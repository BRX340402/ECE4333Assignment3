#pragma once
#include <stdint.h>
#include <math.h>
// Use unwrapped yaw when available; otherwise scan by bounded motor-on time.
struct Ex3Tracking {
 int blackLow=600,blackHigh=850,rightDegrees=120,sweepDegrees=240;
 enum Failure : uint8_t {OK=0,LIFTED=1,GYRO_LOST=2,TIMED_OUT=3,WRONG_YAW=4,EXHAUSTED=5};
 uint8_t failure=OK;
 bool searching=false,failed=false,timedSearch=false,liftSeen=false;
 uint8_t phase=0,found=0;uint32_t at=0,searchAt=0,liftAt=0;float origin=0,reverseAt=0;
 void reset(){searching=failed=timedSearch=liftSeen=false;failure=OK;phase=found=0;}
 bool black(int v)const{return v>=blackLow&&v<=blackHigh;}
 void step(int l,int m,int r,int speed,uint32_t now,float yaw,bool gyro,int &left,int &right){
  left=right=0;if(failed)return;
  if(l>950&&m>950&&r>950){if(!liftSeen){liftSeen=true;liftAt=now;}if(uint32_t(now-liftAt)>=200){failed=true;failure=LIFTED;}return;}
  liftSeen=false;
  if(speed<=0){reset();return;} // Zero means stopped: no search timer or division by zero.
  int turn=speed>70?70:speed;
  if(!searching){
   if(black(m)||(black(l)&&black(r))){left=right=speed;return;}
   if(black(r)){left=turn;right=-turn;return;}
   if(black(l)){left=-turn;right=turn;return;}
   searching=true;phase=found=0;origin=reverseAt=yaw;at=searchAt=now;timedSearch=!gyro;
  }
  if(uint32_t(now-searchAt)>=30000){failed=true;failure=TIMED_OUT;return;}
  // Side sensors must also be able to recover a left/right bend during search.
  if(black(l)||black(m)||black(r)){
   if(++found>=3){reset();if(black(m)||(black(l)&&black(r)))left=right=speed;
    else if(black(l)){left=-turn;right=turn;}else{left=turn;right=-turn;}return;}
   if(!black(m)){if(black(l)){left=-turn;right=turn;}else{left=turn;right=-turn;}}
   return; // Brake on centre detection; do not sweep past it while confirming.
  }else found=0;
  bool rightTurn=phase==0;
  if(!timedSearch&&(!gyro||(rightTurn?yaw-origin>15:yaw-origin< -15))){timedSearch=true;}
  // Cap search to at most 120 degrees to either side of the starting yaw.
  // phase 0: turn right 'first'; phase 1: reverse left 'sweep' total,
  // so 120 right + 240 left ends 120 degrees LEFT of the starting yaw.
  int first=rightDegrees>120?120:rightDegrees;
  int sweep=sweepDegrees>first+120?first+120:sweepDegrees;
  int degrees=phase==0?first:sweep;
  // Nominal 12 ms/degree at PWM70, scaled for slower scans; safety fallback only.
  // This is NOT a calibrated turning angle. Use verified gyro yaw where available.
  uint32_t duration=(uint32_t)degrees*12*70/(uint32_t)turn;
  // A stalled gyro must not keep the same turn running for the full 30 s timeout.
  bool finished=uint32_t(now-at)>=duration||(!timedSearch&&(rightTurn?origin-yaw>=degrees:yaw-origin>=degrees));
  if(finished){
   if(phase>=1){failed=true;failure=EXHAUSTED;return;}
   phase++;origin=reverseAt=yaw;at=now;return;
  }
  if(rightTurn){left=turn;right=-turn;}else{left=-turn;right=turn;}
 }
};
