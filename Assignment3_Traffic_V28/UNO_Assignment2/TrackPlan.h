#pragma once
#include <stdint.h>
#include <math.h>
#include "Track.h"
// Kinematic track follower (replaces the black-line SEARCH of V15-V27).
// The robot knows the track: a list of (turn, length) segments.  It drives each
// segment straight with a gyro heading hold, and when its estimated position has
// travelled the segment length it pivots to the next heading with the gyro.
// There is no sweep/search.  The floor sensors only act as an OUTER loop:
//   * side sensor sees the line -> small heading-setpoint trim (+-TRIM_MAX_DEG)
//   * line ends near a planned corner -> that corner is a LANDMARK: the fused
//     position along the segment is corrected (one nudge per segment)
// and the inner loop is always the gyro PD.  If the line is not seen nothing
// happens: the plan simply continues from the kinematic estimate.
struct PlanIn {float x,y,yaw,rate;bool gyro;};     // fused pose (cm, deg, deg/s)
struct TrackPlan {
 enum Phase:uint8_t{TURN=0,DRIVE=1};
 enum Fault:uint8_t{F_NONE=0,F_LIFTED=1,F_GYRO=2,F_YAW=3};
 int blackLow=600,blackHigh=850;bool lineEvent=LINE_EVENT_ENABLED,lineTrim=LINE_TRIM_ENABLED;
 uint8_t seg=0,lap=0,phase=TURN,fault=F_NONE;bool done=false,lapMarked=false,evDone=false,liftSeen=false,gapOn=false;
 float px=0,py=0,hd=0,ulx=0,uly=0,ulh=0,ux=1,uy=0,len=0,s=0,trim=0,turnYaw=0,evErr=0;
 float sPrev=0,lineCm=0;bool sInit=false;uint8_t evCount=0;bool evFlag=false;float evRatio=0;
 uint32_t turnMs=0,lastAt=0,settleAt=0,liftAt=0,gapAt=0,gyroBadAt=0;
 float window()const{float w=EVENT_WINDOW_FRAC*len;return w>EVENT_WINDOW_CM?w:EVENT_WINDOW_CM;}
 static float wrap180(float a){while(a>180)a-=360;while(a< -180)a+=360;return a;}
 bool black(int v)const{return v>=blackLow&&v<=blackHigh;}
 void load(uint8_t k,uint32_t now){
  hd+=a3_rd16(&PLAN_TURN10[k])*0.1f;len=a3_rd16(&PLAN_MM[k])*0.1f;
  ux=cosf(hd*(float)(M_PI/180.0));uy=sinf(hd*(float)(M_PI/180.0));
  seg=k;turnMs=0;settleAt=0;evDone=lapMarked=false;lineCm=0;sInit=false;gapOn=false;
  float t=a3_rd16(&PLAN_TURN10[k])*0.1f;phase=(t>0.5f||t< -0.5f)?TURN:DRIVE;
 }
 void reset(uint32_t now=0){
  px=py=hd=0;evFlag=false;fault=F_NONE;done=false;lap=0;trim=0;liftSeen=false;gyroBadAt=0;evCount=0;evErr=0;
  load(0,now);ulx=len*ux;uly=len*uy;ulh=hd;
 }
 void advance(uint32_t now){
  px+=len*ux;py+=len*uy;
  uint8_t k=seg+1;
  if(k>=PLAN_SEGMENTS){k=LOOP_FIRST_SEG;px=ulx;py=uly;hd=ulh;}   // exact wrap, no accumulated float drift
  load(k,now);
 }
 // Not moving (red light, STOP hold...): keep state, send nothing.
 void hold(){lastAt=0;}
 // s = progress along the segment, derived from the fused position.
 void step(const PlanIn &in,int l,int m,int r,uint32_t now,int speed,int &left,int &right,float &nudgeX,float &nudgeY){
  left=right=0;nudgeX=nudgeY=0;if(done||fault)return;
  if(l>950&&m>950&&r>950){if(!liftSeen){liftSeen=true;liftAt=now;}if(now-liftAt>=200){fault=F_LIFTED;}return;}
  liftSeen=false;
  if(!in.gyro){if(!gyroBadAt)gyroBadAt=now;if(now-gyroBadAt>1500)fault=F_GYRO;return;}   // plan needs the gyro
  gyroBadAt=0;
  if(speed<=0)return;
  uint32_t dtm=lastAt?now-lastAt:0;if(dtm>100)dtm=0;lastAt=now;     // time spent actually running
  if(phase==TURN){
   turnMs+=dtm;
   float e=wrap180(hd-in.yaw),ae=e<0?-e:e,rt=in.rate<0?-in.rate:in.rate,coast=TURN_COAST_S*rt;if(coast>15)coast=15;
   if(ae<=TURN_TOL_DEG+coast){
    // on (or coasting onto) the target heading: cut power and let it settle
    if(ae<=TURN_TOL_DEG&&rt<15){if(!settleAt)settleAt=now;if(now-settleAt>=TURN_SETTLE_MS){settleAt=0;phase=DRIVE;lineCm=0;sInit=false;trim=0;}}
    else settleAt=0;
    return;
   }
   settleAt=0;
   if(turnMs>TURN_TIMEOUT_MS){phase=DRIVE;trim=0;return;}              // never spin forever
   int lo=TURN_MIN_PWM<speed?TURN_MIN_PWM:speed,hi=TURN_MAX_PWM<speed?TURN_MAX_PWM:speed;
   int mag=(int)(TURN_KP*ae);if(mag<lo)mag=lo;if(mag>hi)mag=hi;
   int sign=e>=0?1:-1;left=-sign*mag;right=sign*mag;
   // wrong gyro sign: commanded turn but heading moves the other way
   if(turnMs<200)turnYaw=in.yaw;
   else if(turnMs>600&&(in.yaw-turnYaw)*sign< -25.0f)fault=F_YAW;
   return;
  }
  // ---- DRIVE ----
  s=(in.x-px)*ux+(in.y-py)*uy;
  if(seg==PLAN_SEGMENTS-1&&!lapMarked&&s>=len-START_TO_UL_MM*0.1f){   // passing the start point again
   lapMarked=true;lap++;if(LAPS_TO_RUN&&lap>=LAPS_TO_RUN){done=true;return;}
  }
  bool b=black(l)||black(m)||black(r);
  if(!sInit){sInit=true;sPrev=s;}
  float ds=s-sPrev;sPrev=s;if(ds>0&&ds<5&&b)lineCm+=ds;      // distance driven while on the line
  if(lineEvent&&!evDone){
   if(b)gapOn=false;
   else if(lineCm>=EVENT_MIN_LINE_CM){                          // only after really following the line (not just crossing it)
    if(!gapOn){gapOn=true;gapAt=now;}
    float corner=len-SENSOR_AHEAD_CM,err=s-corner,win=window();
    if(now-gapAt>=EVENT_MIN_MS&&err>-win&&err<win){
     evDone=true;evErr=err;if(evCount<255)evCount++;
     if(len>100&&s>50){float q=corner/s;if(q>1-SCALE_MAX_STEP&&q<1+SCALE_MAX_STEP){evFlag=true;evRatio=q;}}   // how far the car really went vs the estimate
     nudgeX=-err*ux;nudgeY=-err*uy;s=corner;sPrev=s;               // the corner is where the sensors say it is
    }
   }
  }
  // Slower than modelled: the estimate reaches the corner while the line still runs on ahead.
  // Keep driving straight (still no search) until the line ends, up to the event window.
  if(s>=len-CORNER_LEAD_CM&&!(lineEvent&&!evDone&&(b||gapOn)&&s<len+window())){advance(now);return;}
  if(lineTrim){
   if(black(l)&&!black(r))trim+=TRIM_STEP_DEG;else if(black(r)&&!black(l))trim-=TRIM_STEP_DEG;
   else trim*=black(m)?0.9f:0.995f;
   if(trim>TRIM_MAX_DEG)trim=TRIM_MAX_DEG;if(trim< -TRIM_MAX_DEG)trim=-TRIM_MAX_DEG;
  }
  float e=wrap180(hd+trim-in.yaw),u=HOLD_KP*e-HOLD_KD*in.rate;
  if(u>HOLD_UMAX)u=HOLD_UMAX;if(u< -HOLD_UMAX)u=-HOLD_UMAX;
  int v=speed;if(len-s<CORNER_SLOW_CM&&v>CORNER_PWM)v=CORNER_PWM;
  left=v-(int)u;right=v+(int)u;
 }
};
