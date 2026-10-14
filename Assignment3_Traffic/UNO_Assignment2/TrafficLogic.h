#pragma once
#include <stdint.h>
// V15: line motion does not depend on colour calibration or camera freshness.
struct TrafficLogic {
 enum State {OFF,RESERVED_1,TRACK,APPROACH,VERIFY,WAIT_RED,STOP_HOLD,CROSS,FAULT};
 enum Fault : uint8_t {NONE=0,COLOUR_STALE=1,COLOUR_INVALID=2,RED_LOST=3,AI_INVALID=4,AI_TIMEOUT=5,CROSS_TIMEOUT=6,LINE_LIFTED=7,GYRO_UNAVAILABLE=8,SEARCH_TIMEOUT=9,YAW_WRONG=10,SEARCH_EXHAUSTED=11,CROSS_HEADING=12,UNKNOWN_FAULT=13};
 uint8_t fault=NONE,faultState=OFF;
 State state=OFF;uint32_t at=0,lastColour=0,lastRed=0,clearAt=0;uint16_t seq=0,request=0;
 bool seen=false,red=false,green=false,lightCalibrated=false;int distance=-1;uint8_t greens=0,positives=0,negatives=0;
 uint32_t stopHoldMs=3000,crossMinimumMs=1500;bool crossReady=false;
 bool stopOnRed=false;int cruiseSpeed=100,slowSpeed=60;
 int cruisePWM()const{return cruiseSpeed;}
 int slowPWM()const{return cruiseSpeed<slowSpeed?cruiseSpeed:slowSpeed;}
 // A stopped car must restart at cruise PWM after STOP confirmation.
 // Reduced PWM remains the moving approach speed in the legacy mode.
 int crossingPWM()const{return stopOnRed||slowPWM()==0?cruisePWM():slowPWM();}
 int stopCm=25;bool pendingInference=false;
 void enter(State s,uint32_t now){if(s==FAULT&&state!=FAULT&&fault==NONE){fault=UNKNOWN_FAULT;faultState=state;}if(s==OFF){fault=NONE;faultState=OFF;}state=s;at=now;crossReady=false;greens=0;positives=negatives=0;if(s==VERIFY){request++;pendingInference=true;}else pendingInference=false;}
 void fail(Fault why,uint32_t now){if(state==FAULT)return;fault=why;faultState=state;enter(FAULT,now);}
 bool fresh(uint32_t now)const{return seen&&uint32_t(now-lastColour)<1500;}
 bool atRedLight()const{return lightCalibrated&&red&&distance>=0&&distance<=stopCm;}
 // VERIFY remains a moving, slow state; only calibrated nearby red can hold it.
 bool moving()const{return state==TRACK||state==CROSS||((state==APPROACH||state==VERIFY)&&!stopOnRed&&!atRedLight());}
 void start(uint32_t now){if(state!=OFF)return;clearAt=0;enter(TRACK,now);if(fresh(now)&&red)enter(VERIFY,now);}
 void stop(uint32_t now){enter(OFF,now);}
 void colour(uint16_t s,bool valid,bool r,bool g,int d,uint32_t now,bool calibrated=false){
  if(seen&&uint32_t(now-lastColour)<3000){uint16_t delta=s-seq;if(!delta||delta>=32768)return;}
  seq=s;if(!valid){greens=0;return;} // Ignore a bad frame; keep following the floor line.
  seen=true;lastColour=now;red=r;green=g;distance=d;lightCalibrated=calibrated;
  if(r)lastRed=now;
  if(g&&!r){if(greens<3)greens++;}else greens=0;
  // In Stop-on-red mode VERIFY is *already stopped*. Accept three distinct
  // green-only colour frames there as well as in WAIT_RED. Previously green
  // could not release VERIFY until its 20-second timeout had elapsed.
  // A red region in the same frame resets greens (see condition above).
  if(state==WAIT_RED || (stopOnRed && state==VERIFY)){
   if(greens>=3)enter(TRACK,now);
   return;
  }
  if(state==TRACK&&r)enter(VERIFY,now);
  else if(state==APPROACH&&atRedLight())enter(VERIFY,now);
  else if(!stopOnRed&&(state==APPROACH||state==VERIFY)&&!r&&uint32_t(now-lastRed)>700)enter(TRACK,now);
 }
 void waitForRed(uint32_t now){if(state==WAIT_RED)return;uint8_t hits=positives;enter(WAIT_RED,now);if(stopOnRed)positives=hits;}
 void inference(uint16_t id,bool ok,bool positive,uint32_t now,int stopDistance=-1){
  if((state!=VERIFY&&!(stopOnRed&&state==WAIT_RED))||id!=request)return;
  if(!ok){if(!stopOnRed)positives=0;negatives=0;return;}
  if(positive){if(positives<2)positives++;negatives=0;}else{if(negatives<2)negatives++;if(!stopOnRed)positives=0;}
  if(positives>=2){
   if(stopOnRed){clearAt=0;enter(CROSS,now);}
   else if(stopDistance<0||stopDistance<=stopCm)enter(STOP_HOLD,now);
   else positives=0; // Confirmed but far away: keep approaching and remeasure.
  }else if(negatives>=2){if(stopOnRed||atRedLight())waitForRed(now);else enter(APPROACH,now);}
 }
 void tick(uint32_t now){
  if(state==OFF||state==FAULT)return;
  if(state==STOP_HOLD){if(uint32_t(now-at)>=stopHoldMs){clearAt=0;enter(CROSS,now);}return;}
  if(state==WAIT_RED)return;
  if(state==CROSS){if((crossReady&&uint32_t(now-at)>=crossMinimumMs)||uint32_t(now-at)>=crossMinimumMs+5000)enter(TRACK,now);return;}
  if(stopOnRed&&state==VERIFY){if(uint32_t(now-at)>20000)waitForRed(now);return;}
  if(!fresh(now)){red=green=false;distance=-1;if(state!=TRACK)enter(TRACK,now);return;}
  if(state==VERIFY&&uint32_t(now-at)>20000)enter((stopOnRed||atRedLight())?WAIT_RED:APPROACH,now);
  if(state==APPROACH&&red&&uint32_t(now-at)>=2000)enter(VERIFY,now);
 }
};
