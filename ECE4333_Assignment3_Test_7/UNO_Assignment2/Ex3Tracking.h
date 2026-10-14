#pragma once
#include <stdint.h>
#include <math.h>
// Assignment 3 Test 1: conservative, bounded recovery inspired by ELEGOO Demo 2.
// Sequence (with RA=120, SA=360): right 120 -> left 360 -> right 240
// -> 100 ms *forward* nudge if ultrasonic clearance is CONFIRMED; repeat <=5.
// Unlike the previous version, bright-white sensor readings do not imply the
// car has been lifted; the three reflectance sensors alone cannot tell.
struct Ex3Tracking {
 int blackLow=600,blackHigh=1023,rightDegrees=120,sweepDegrees=360;
 enum Failure : uint8_t {OK=0,LIFTED=1,GYRO_LOST=2,TIMED_OUT=3,WRONG_YAW=4,EXHAUSTED=5,BLOCKED=6};
 uint8_t failure=OK;
 bool searching=false,failed=false,timedSearch=false;
 uint8_t phase=0,found=0,attempt=0;
 uint32_t at=0,searchAt=0;
 float origin=0,entryYaw=0,trackYaw=0;
 uint32_t candidateAt=0,wideAt=0;bool candidate=false,wide=false,headingKnown=false;
 static const uint8_t MAX_ATTEMPTS=5;
 static const uint32_t MAX_SEARCH_MS=120000UL;
 static const uint16_t NUDGE_MS=100;  // initial TEST value; physical distance is NOT guaranteed
 static const int SAFE_NUDGE_CM=22;  // forward ultrasonic must return a *valid* safe reading
 void reset(){searching=failed=timedSearch=false;failure=OK;phase=found=attempt=0;at=searchAt=0;candidate=wide=headingKnown=false;}
 bool black(int v)const{return v>=blackLow&&v<=blackHigh;}
 void nextPhase(uint32_t now,float yaw,bool gyro){phase++;origin=yaw;at=now;timedSearch=!gyro;}
 void step(int l,int m,int r,int speed,uint32_t now,float yaw,bool gyro,int &left,int &right,int rangeCm=-1){
  left=right=0;
  if(failed)return;
  if(speed<=0){reset();return;}
  const int turn=speed>70?70:speed;
  const bool bl=black(l),bm=black(m),br=black(r);
  if(!searching){
   // Uniform/wide dark region is ambiguous. Preserve the last line heading
   // briefly instead of declaring it a centred line indefinitely.
   if(bl&&bm&&br){
    if(!wide){wide=true;wideAt=now;if(!headingKnown)trackYaw=yaw;}
    if(uint32_t(now-wideAt)<600){
     int base=speed>50?50:speed;
     int correction=gyro?(int)((trackYaw-yaw)*2):0;
     if(correction>20)correction=20;if(correction< -20)correction=-20;
     left=base-correction;right=base+correction;return;
    }
   }else{
    wide=false;
    if(bm||(bl&&br)){trackYaw=yaw;headingKnown=true;left=right=speed;return;}
    if(br){left=turn;right=-turn;return;}
    if(bl){left=-turn;right=turn;return;}
   }
   searching=true;phase=found=attempt=0;entryYaw=origin=yaw;
   at=searchAt=now;timedSearch=!gyro;candidate=false;
  }
  if(uint32_t(now-searchAt)>=MAX_SEARCH_MS){failed=true;failure=TIMED_OUT;return;}
  // Reacquire a narrow centre line for >=60 ms, not three CPU iterations.
  // A line encountered backwards is not the forward continuation of this route.
  float angle=yaw-entryYaw;while(angle>180)angle-=360;while(angle< -180)angle+=360;
  bool forward=!gyro||fabs(angle)<150;
  if(((bm&&!(bl&&br))||(bl&&br&&!bm))&&forward){
   if(!candidate){candidate=true;candidateAt=now;}
   if(uint32_t(now-candidateAt)>=60){reset();trackYaw=yaw;headingKnown=true;left=right=speed;}
   return;
  }
  candidate=false;
  const int first=rightDegrees>120?120:rightDegrees;
  int sweep=sweepDegrees>360?360:sweepDegrees;
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
  if(!timedSearch && !gyro){timedSearch=true;at=now;}
  // A reversed gyro sign must be corrected rather than hidden by timed fallback.
  if(!timedSearch && (rightTurn?(yaw-origin)>15:(yaw-origin)<-15)){failed=true;failure=WRONG_YAW;return;}
  uint32_t duration=(uint32_t)degrees*12*70/(uint32_t)turn;
  // With a healthy gyro, nominal motor time must NOT truncate the angle.
  if(!timedSearch&&uint32_t(now-at)>duration*4+3000){failed=true;failure=TIMED_OUT;return;}
  bool finished=timedSearch?uint32_t(now-at)>=duration:
      (rightTurn?(origin-yaw)>=degrees:(yaw-origin)>=degrees);
  if(finished){nextPhase(now,yaw,gyro);return;}
  if(rightTurn){left=turn;right=-turn;}else{left=-turn;right=turn;}
 }
};
