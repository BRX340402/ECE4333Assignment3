#pragma once
#include <stdint.h>
#include <math.h>
struct CrossingControl {
 float target=0;uint32_t centerSince=0;bool timingCenter=false,wideSeen=false;uint8_t wideCount=0;
 void arm(float yaw){target=yaw;centerSince=0;timingCenter=false;wideSeen=false;wideCount=0;}
 bool step(float yaw,bool gyro,bool l,bool m,bool r,uint32_t now,uint32_t elapsed,uint32_t minimum,int &left,int &right,int speed=60){
  left=right=0;
  float correction=gyro&&fabs(target-yaw)<=40?(target-yaw)*2.0f:0;
  if(correction>25)correction=25;if(correction< -25)correction=-25;
  left=speed-(int)correction;right=speed+(int)correction;
  if(l&&r){if(wideCount<3)wideCount++;if(wideCount>=3)wideSeen=true;}else wideCount=0;
  if(elapsed>=minimum&&m&&!l&&!r){if(!timingCenter){centerSince=now;timingCenter=true;}}else timingCenter=false;
  return true;
 }
 bool cleared(uint32_t now)const{return timingCenter&&uint32_t(now-centerSince)>=200;}
};
