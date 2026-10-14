#pragma once
#include <stdint.h>
#include <math.h>
// Adapted from teammate Control.cpp: weighted position, filtered derivative,
// yaw-rate damping and speed scheduling. Outputs only; Test2 owns the motors.
struct TeamControl {
 int16_t previous=0,filtered=0;uint32_t last=0;bool ready=false;
 void reset(){ready=false;previous=filtered=0;}
 static float clamp(float x,float a,float b){return x<a?a:(x>b?b:x);}
 static int limit(int x,int a,int b){return x<a?a:(x>b?b:x);}
 bool line(int l,int m,int r,int low,int high,int speed,uint32_t now,float rate,int &left,int &right){
  if(speed<=0){left=right=0;reset();return true;}
  bool bl=l>=low&&l<=high,bm=m>=low&&m<=high,br=r>=low&&r<=high;
  if(!(bl||bm||br)){reset();return false;}
  if((bm&&!bl&&!br)||(bl&&br)){left=right=speed;reset();return true;}
  int wl=bl?(low==0?high-l+1:l-low+1):0,wm=bm?(low==0?high-m+1:m-low+1):0,wr=br?(low==0?high-r+1:r-low+1):0;
  int err=(long)(wr-wl)*1000/(wl+wm+wr);
  uint32_t dt=now-last;
  if(!ready||dt>250){previous=err;filtered=0;ready=true;}
  else {if(dt<10)dt=10;int d=(long)3*(err-previous)/dt;filtered+=(long)35*(d-filtered)/100;}
  last=now;previous=err;
  int steer=limit((long)60*err/1000+filtered+(int)rate*8/100,-speed,speed);
  int base=limit(speed-(long)40*abs(err)/1000,speed<30?speed:30,speed);
  left=limit(base+steer,-speed,speed);right=limit(base-steer,-speed,speed);return true;
 }
 static void heading(float target,float yaw,float rate,int speed,int &left,int &right){
  int err=(int)(target-yaw);while(err>180)err-=360;while(err< -180)err+=360;
  int u=limit(err*16/10-(int)rate*12/100,-25,25);
  left=limit(speed-u,0,speed);right=limit(speed+u,0,speed);
 }
};
