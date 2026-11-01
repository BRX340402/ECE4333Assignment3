#pragma once
#include <stdint.h>
#include <stdlib.h>
// Test14 traffic rules for the Figure-8 race (replaces Test13 TrafficLite.h). Pure logic, no Arduino calls.
//
// Vision zones (the sketch computes them from the map and reports them to the ESP32 in N221 "Z", so the
// ESP32 only runs the detector that is needed there; outside the zones colour detection is OFF):
//   zone 1 TRAFFIC LIGHT : segment O-S (driving towards the light) and the start while waiting for green
//   zone 2 STOP SIGN     : segments E-O and C-O (each has a STOP sign before O) until that stop is done
//   zone 0               : everywhere else - red is ignored
//
// Traffic light (zone 1):
//   GO -> WAIT_GREEN : red light (ESP32 level >= 1, red region >= RPL)   [also the race start]
//   WAIT_GREEN -> GO : 2 fresh green frames without red
// STOP sign (zone 2):
//   GO   -> SLOW  : red blob >= RPS pixels (level 1): drive at slowPct % of the race speed
//   SLOW -> HOLD  : red blob >= RP pixels (level 2: the sign is right in front), or the map says the stop line
//                   (stopLineCm before O) is reached although the sign was not seen (never run a STOP sign)
//   HOLD          : stand still. The 3 s timer starts HERE, at the same moment the STOP-sign AI is requested.
//                     ph 0  camera straight: the ESP32 captures the frame for the FOMO model (N219 = captured)
//                     ph 1  pan LEFT  + ultrasonic: cross traffic on the other road?
//                     ph 2  pan RIGHT + ultrasonic
//                     ph 3  back to the centre; leave when >= 3 s since the stop AND both sides were clear.
//                   A side reading closer than clearCm is traffic; the same echo (+-4 cm) on two scans in a
//                   row is a fixed object (the sign post, a wall) and counts as clear. Blocked -> scan again.
//   DONE          : this stop is done; red is ignored until the car leaves the segment.
// The AI answer (N205) does not shorten or lengthen the hold; it is counted for the report (stops / stopsAI).
struct RaceRules{
 enum State:uint8_t{GO=0,SLOW=1,HOLD=2,WAIT_GREEN=3,DONE=4};
 State st=GO;uint8_t ph=0,zone=0,greens=0,segSeen=255;bool enabled=false,changed=false,seqSeen=false,aiAns=false;
 bool blocked[2]={false,false};int16_t lastEcho[2]={-1,-1};
 uint32_t at=0,phAt=0;uint16_t req=0,seq=0;uint8_t stops=0,stopsAI=0;
 uint8_t clearCm=45; // intersection scan: closer than this = traffic
 void reset(){st=GO;ph=0;greens=0;seqSeen=false;segSeen=255;changed=true;}
 void enter(State s,uint32_t now){st=s;at=phAt=now;ph=0;greens=0;changed=true;}
 bool stopped()const{return st==HOLD||st==WAIT_GREEN;}
 bool slow()const{return st==SLOW;}
 // TS code for the ESP32 and the page: 2 track, 3 slowing for STOP, 4 STOP + AI capture wanted, 5 wait for green, 6 STOP hold / scan
 uint8_t ts()const{return st==HOLD?(ph==0?4:6):st==WAIT_GREEN?5:st==SLOW?3:2;}
 // pan request while holding: +1 left, -1 right, 0 straight
 int8_t look()const{return st==HOLD?(ph==1?1:(ph==2?-1:0)):0;}
 // the follower moved to another segment: a finished / pending stop belongs to the old one
 void segment(uint8_t s){if(s!=segSeen){segSeen=s;if(st==DONE||st==SLOW){st=GO;changed=true;}}}
 void setZone(uint8_t z){if(z!=zone){zone=z;changed=true;}}
 void hold(uint32_t now){enter(HOLD,now);req++;aiAns=false;lastEcho[0]=lastEcho[1]=-1;if(stops<255)stops++;}
 // N204 colour frame. level: zone 1 -> 1 = red light; zone 2 -> 1 = red blob >= RPS, 2 = >= RP. allow = line follower on a straight
 void colour(uint16_t s,bool valid,uint8_t level,bool green,bool allow,uint32_t now){
  if(seqSeen&&s==seq)return;seq=s;seqSeen=true;
  if(!valid)return;
  switch(st){
   case GO:case SLOW:
    if(!enabled||!allow)break;
    if(zone==1&&level)enter(WAIT_GREEN,now);
    else if(zone==2){if(level>=2)hold(now);else if(level&&st==GO){st=SLOW;changed=true;}}
    break;
   case WAIT_GREEN:if(level)greens=0;else if(green&&++greens>=2)enter(GO,now);break;
   default:break;
  }
 }
 // map fallback: never pass the stop line of a STOP-sign approach without stopping
 void stopLine(float toO,uint8_t lineCm,uint32_t now){if(enabled&&lineCm&&zone==2&&(st==GO||st==SLOW)&&toO<=lineCm)hold(now);}
 void captured(uint16_t id,uint32_t now){if(st==HOLD&&ph==0&&id==req){ph=1;phAt=now;changed=true;}}
 void ai(uint16_t id,bool ok,bool positive,uint32_t now){
  if(id!=req||!ok||aiAns)return;
  aiAns=true;if(positive&&stopsAI<255)stopsAI++;
  captured(id,now); // an answer also means the frame was taken
 }
 // range: ultrasonic cm (-1 no echo, -2 servo still moving); fresh: a new ping since the last call
 void tick(uint32_t now,int range,bool fresh){
  if(st!=HOLD)return;
  uint32_t t=now-phAt;
  switch(ph){
   case 0:if(t>=1500){ph=1;phAt=now;changed=true;}break; // no capture message: scan anyway (the timer is already running)
   case 1:case 2:
    if(t>=550&&fresh&&range>=-1){
     uint8_t i=ph-1;bool clear=range<0||range>clearCm;
     if(!clear){if(lastEcho[i]>=0&&abs(range-lastEcho[i])<=4)clear=true;lastEcho[i]=range;}else lastEcho[i]=-1;
     blocked[i]=!clear;ph++;phAt=now;
    }
    break;
   default:
    if(t>=350){
     if(blocked[0]||blocked[1]){ph=1;phAt=now;}       // something is crossing: look again
     else if(uint32_t(now-at)>=3000)enter(DONE,now);   // 3 s since the stop (= since the AI request) and clear: go
    }
    break;
  }
 }
};
