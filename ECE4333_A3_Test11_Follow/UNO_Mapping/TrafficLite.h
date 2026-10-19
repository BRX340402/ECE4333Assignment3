#pragma once
#include <stdint.h>
// "Stop on red" for the mapping build (small replacement for Test9 TrafficLogic).
// Enabled by the web setting "Stop on red" (RS=1). Red = the ESP32's N204.R, i.e. the red
// region reached the Red-pixel threshold RP. Then:
//   VERIFY     : stopped, ESP32 runs the STOP-sign AI (UNO reports TS=4 + request ID)
//                 2 positive AI results -> HOLD 3 s;  2 negative -> WAIT_GREEN
//   HOLD       : STOP sign: stand still 3 s, then go on (CLEARING)
//   WAIT_GREEN : red light: wait for 2 fresh green frames with no red, then go on
//   CLEARING   : driving; red is ignored until 3 frames without red (no re-stop on the same sign)
// VERIFY gives up after 20 s and waits for green (no AI answer).
struct TrafficLite {
 enum State:uint8_t{GO=0,VERIFY=1,HOLD=2,WAIT_GREEN=3,CLEARING=4};
 State st=GO;bool enabled=false,changed=false;uint32_t at=0;uint16_t req=0,seq=0;
 uint8_t pos=0,neg=0,greens=0,clear=0;bool seqSeen=false;
 void reset(){st=GO;pos=neg=greens=clear=0;seqSeen=false;changed=true;}
 void enter(State s,uint32_t now){st=s;at=now;pos=neg=greens=clear=0;changed=true;if(s==VERIFY)req++;}
 bool stopped()const{return st==VERIFY||st==HOLD||st==WAIT_GREEN;}
 uint8_t ts()const{return st==VERIFY?4:st==WAIT_GREEN?5:st==HOLD?6:2;} // Test9 TS codes for the page/ESP32
 // N204 colour frame. allow = the line follower is on a straight (not pivoting/searching).
 void colour(uint16_t s,bool valid,bool red,bool green,bool allow,uint32_t now){
  if(seqSeen&&s==seq)return;seq=s;seqSeen=true;
  if(!valid)return;
  switch(st){
   case GO:if(enabled&&red&&allow)enter(VERIFY,now);break;
   case CLEARING:if(red)clear=0;else if(++clear>=3)enter(GO,now);break;
   case VERIFY:case WAIT_GREEN:
    if(red)greens=0;else if(green&&++greens>=2)enter(CLEARING,now);
    break;
   default:break;
  }
 }
 // N205 STOP-sign AI result for request id
 void ai(uint16_t id,bool ok,bool positive,uint32_t now){
  if((st!=VERIFY&&st!=WAIT_GREEN)||id!=req||!ok)return;
  if(positive){neg=0;if(++pos>=2)enter(HOLD,now);}
  else if(st==VERIFY&&++neg>=2){uint16_t r=req;enter(WAIT_GREEN,now);req=r;}
 }
 void tick(uint32_t now){
  if(st==HOLD&&uint32_t(now-at)>=3000)enter(CLEARING,now);
  else if(st==VERIFY&&uint32_t(now-at)>=20000){uint16_t r=req;enter(WAIT_GREEN,now);req=r;}
 }
};
