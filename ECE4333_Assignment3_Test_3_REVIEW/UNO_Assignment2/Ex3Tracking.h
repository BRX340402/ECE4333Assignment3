#pragma once
#include <stdint.h>
#include <math.h>
// Assignment 3 Test 1: conservative, bounded recovery inspired by ELEGOO Demo 2.
// Sequence (with RA=120, SA=240): right 120 -> left 240 -> right 120
// -> 100 ms *forward* nudge if ultrasonic clearance is CONFIRMED; repeat <=5.
// Unlike the previous version, bright-white sensor readings do not imply the
// car has been lifted; the three reflectance sensors alone cannot tell.
struct Ex3Tracking {
 int blackLow=600,blackHigh=850,rightDegrees=120,sweepDegrees=240;
 enum Failure : uint8_t {OK=0,LIFTED=1,GYRO_LOST=2,TIMED_OUT=3,WRONG_YAW=4,EXHAUSTED=5,BLOCKED=6};
 uint8_t failure=OK;
 bool searching=false,failed=false,timedSearch=false;
 uint8_t phase=0,found=0,attempt=0;
 uint32_t at=0,searchAt=0;
 float origin=0;
 static const uint8_t MAX_ATTEMPTS=5;
 static const uint32_t MAX_SEARCH_MS=55000UL;
 static const uint16_t NUDGE_MS=100;  // initial TEST value; physical distance is NOT guaranteed
 static const int SAFE_NUDGE_CM=22;  // forward ultrasonic must return a *valid* safe reading
 void reset(){searching=failed=timedSearch=false;failure=OK;phase=found=attempt=0;at=searchAt=0;}
 bool black(int v)const{return v>=blackLow&&v<=blackHigh;}
 void nextPhase(uint32_t now,float yaw,bool gyro){phase++;origin=yaw;at=now;timedSearch=!gyro;}
 void step(int l,int m,int r,int speed,uint32_t now,float yaw,bool gyro,int &left,int &right,int rangeCm=-1){
  left=right=0;
  if(failed)return;
  if(speed<=0){reset();return;}
  const int turn=speed>70?70:speed;
  if(!searching){
   if(black(m)||(black(l)&&black(r))){left=right=speed;return;}
   if(black(r)){left=turn;right=-turn;return;}
   if(black(l)){left=-turn;right=turn;return;}
   searching=true;phase=found=attempt=0;origin=yaw;at=searchAt=now;timedSearch=!gyro;
  }
  if(uint32_t(now-searchAt)>=MAX_SEARCH_MS){failed=true;failure=TIMED_OUT;return;}
  // Require three observations of a visible line before leaving recovery.
  if(black(l)||black(m)||black(r)){
   if(++found>=3){
    reset();
    if(black(m)||(black(l)&&black(r))) {left=right=speed;}
    else if(black(l)){left=-turn;right=turn;}
    else{left=turn;right=-turn;}
    return;
   }
   if(!black(m)){if(black(l)){left=-turn;right=turn;}else{left=turn;right=-turn;}}
   return;
  }
  found=0;
  // All sensors above 950 mean 'no black' in this model, NOT a proven lift.
  // Blind repositioning near edges remains unsafe; see Test 1 guide.
  const int first=rightDegrees>120?120:rightDegrees;
  int sweep=sweepDegrees>first+120?first+120:sweepDegrees;
  if(sweep<first)sweep=first;
  if(phase==3){
   // One small forward move at most per attempt; never reverse blindly.
   if(rangeCm>=0 && rangeCm<SAFE_NUDGE_CM){failed=true;failure=BLOCKED;return;}
   // No valid ultrasonic reading? Skip the nudge; still allow bounded scanning.
   if(rangeCm>=SAFE_NUDGE_CM && uint32_t(now-at)<NUDGE_MS){
    int creep=speed>50?50:speed;left=right=creep;return;
   }
   if(++attempt>=MAX_ATTEMPTS){failed=true;failure=EXHAUSTED;return;}
   phase=0;origin=yaw;at=now;timedSearch=!gyro;return;
  }
  // Phase 0: clockwise first; phase 1: anti-clockwise full sweep;
  // phase 2: clockwise return toward the ORIGINAL heading.
  const int degrees=phase==0?first:(phase==1?sweep:(sweep-first));
  const bool rightTurn=phase!=1;
  if(degrees<=0){nextPhase(now,yaw,gyro);return;}
  if(!timedSearch && !gyro)timedSearch=true;
  // If yaw changes in the wrong direction, use bounded timed fallback.
  if(!timedSearch && (rightTurn?(yaw-origin)>15:(yaw-origin)<-15))timedSearch=true;
  uint32_t duration=(uint32_t)degrees*12*70/(uint32_t)turn;
  bool finished=uint32_t(now-at)>=duration ||
      (!timedSearch && (rightTurn?(origin-yaw)>=degrees:(yaw-origin)>=degrees));
  if(finished){nextPhase(now,yaw,gyro);return;}
  if(rightTurn){left=turn;right=-turn;}else{left=-turn;right=turn;}
 }
};
