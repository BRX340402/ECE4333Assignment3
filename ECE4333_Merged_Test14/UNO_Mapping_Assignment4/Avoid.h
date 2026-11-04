#pragma once
#include <stdint.h>
#include <math.h>
#include "HeadingPD.h"
// Test14 obstacle avoidance + overtaking (Bug algorithm), replaces Test13 Overtake.h. Pure logic, no Arduino calls.
//
// Test14c: tight, sensor-guided Bug manoeuvre (the same steps for a static obstacle and for overtaking a robot).
// The track line is the m-line; the car leaves it on the OUTSIDE of the loop and follows the obstacle boundary
// with the ultrasonic turned sideways, then returns sharply.
//   IDLE  line following; ultrasonic straight ahead. 2 readings <= trigCm (a missed echo only counts down) = obstacle.
//         Passing not allowed here / too little straight left -> TAIL, otherwise -> OUT.
//   TAIL  follow behind it (speed scaled by the gap) until passing is allowed.
//   OUT   pivot on the spot to track heading + side*45 deg, then drive straight until latCm beside the line.
//   PASS  servo turns the ultrasonic at the obstacle. No echo yet: turn back to 10 deg outwards (from 45). Echo: hold the side
//         distance keepCm (heading offset 2 deg per cm of error, -25..+30 deg). Echo gone or > keepCm + 20 cm on
//         2 readings = obstacle cleared -> drive tailCm more (car length) -> BACK. Never seen after rg+40 cm, or
//         rg+150 cm in total (a robot as fast as us) -> BACK anyway.
//   BACK  pivot sharply to track heading - side*70 deg, drive until a line sensor sees the line. If the detour went
//         past the next corner, head for the NEXT straight instead (past = true: the map moves on, e.g. B-C -> C-O).
//   JOIN  move the axle over the line, pivot onto it, hand over (lost = line not found: the follower searches).
struct Avoid{
 enum State:uint8_t{IDLE=0,TAIL=1,OUT=2,PASS=3,BACK=4,JOIN=5};
 static const int OUT_DEG=45,BACK_DEG=70,PIVOT_MIN=50,JOIN_V=40,STOP_CM=10;
 // parameters (set by the sketch from the web page)
 bool enabled=false;int trigCm=70,latCm=22,margin=15,tailCm=20,gapCm=18,speed=60,joinAdv=8; // margin = side keep distance (cm, from the sensor)
 HeadingPD pd;
 // state
 State state=IDLE;int8_t side=1,look=0;float H=0,Hn=0,turn=0,cornerCm=999,tgt=0,lat=0,along=0,lastDist=0,startDist=0,d0=0,lostCm=0;
 int rg=0,side_r=-1,lastRange=-1;uint32_t at=0,doneAt=0;uint8_t near=0,far=0,seen=0,gone=0,ok=0,fails=0,scale=100;
 bool handover=false,lost=false,adv=false,cool=false,armed=true,past=false; // past: the detour ended beyond the corner (on the next straight) // armed: passing allowed on this segment (sketch: B-C only)
 // along-track length the manoeuvre needs (detection range + 45 deg out + obstacle + car length + back + join)
 float need()const{return rg+latCm+tailCm+10;}  // Merged: margin 30 -> 10 cm (B-C only)
 bool active()const{return state>=OUT;}
 void reset(){state=IDLE;look=0;near=far=0;handover=lost=cool=false;scale=100;}
 static float norm(float a){a=fmodf(a+180.0f,360.0f);if(a<0)a+=360.0f;return a-180.0f;}
 // turn: heading change at the next corner (deg); corner: straight track left at the start of the detour
 void begin(float lineYaw,int8_t s,float dist,uint32_t now){
  past=false;H=lineYaw;side=s?s:1;tgt=H+side*OUT_DEG;lat=along=0;startDist=lastDist=dist;state=OUT;at=now;look=0;seen=gone=0;adv=false;
 }
 void finish(bool found,uint32_t now){lostCm=(lastDist-startDist)-along;handover=true;lost=!found;if(found){if(ok<99)ok++;}else if(fails<99)fails++;
  state=IDLE;look=0;doneAt=now;cool=true;near=0;}
 void goBack(uint32_t now){state=BACK;look=0;tgt=H-side*BACK_DEG;at=now;}
 // Returns true when Avoid drives the wheels. range: ultrasonic cm for the CURRENT servo direction (-1 no echo,
 // -2 servo still moving); fresh: new ping. following: line follower on a straight; lineYaw: learned track
 // heading; outside: + left / - right; toCorner: straight track left; noPass: STOP-sign segment.
 bool step(float yaw,float rate,float dist,int range,bool fresh,bool following,float lineYaw,int8_t outside,
           float toCorner,bool noPass,bool bl,bool bm,bool br,uint32_t now,int &l,int &r){
  l=r=0;scale=100;
  if(state>=OUT){float ds=dist-lastDist,a=(yaw-H)*0.0174533f;lat+=ds*sinf(a)*side;along+=ds*cosf(a);}
  lastDist=dist;
  bool valid=fresh&&range>=-1;
  if(valid&&look==0){
   lastRange=range;bool hit=range>=0&&range<=trigCm;
   near=hit?(near<9?near+1:9):(near?near-1:0); // a weak echo (plush obstacle) may miss single pingsfar=(range<0||range>trigCm+15)?(far<20?far+1:20):0;
  }
  switch(state){
   case IDLE:
    if(!enabled||!armed||!following||(cool&&uint32_t(now-doneAt)<1500))return false;
    if(near<2||toCorner<10)return false;     // (just before a corner the obstacle is probably beyond it: decide after the turn)
    rg=lastRange;
    if(noPass||toCorner<need()){state=TAIL;at=now;far=0;return false;}
    begin(lineYaw,outside,dist,now);cornerCm=toCorner;break;
   case TAIL:{
    if(!enabled||far>=10){state=IDLE;return false;}
    int u=lastRange;scale=u<0?100:(u<=gapCm?0:(u>=trigCm?100:(uint8_t)((u-gapCm)*100L/(trigCm-gapCm))));
    if(scale)at=now;
    if(following&&!noPass&&near&&(toCorner>=need()||(uint32_t(now-at)>=5000&&toCorner>=0.6f*need()))){rg=u<0?trigCm:u;begin(lineYaw,outside,dist,now);cornerCm=toCorner;break;}
    return false;}                           // the line follower keeps steering, the sketch scales its speed
   default:break;
  }
  switch(state){
   case OUT:
    if(lat>=latCm){state=PASS;look=-side;at=now;seen=gone=0;adv=false;side_r=-1;}
    else if(uint32_t(now-at)>=5000){finish(false,now);return true;}
    break;
   case PASS:{
    if(valid&&look==-side){              // side echo (the sketch passes -2 until the servo has settled)
     if(range>=0&&range<=margin+20){side_r=range;if(seen<9)seen++;gone=0;}
     else if(seen&&gone<9)gone++;
    }
    float off=10;                        // not seen yet: turned most of the way back (still 10 deg outwards)
    if(seen&&!gone){off=2.0f*(margin-side_r);if(off>30)off=30;if(off< -25)off=-25;} // hold keepCm from the obstacle
    tgt=H+side*off;
    if(gone>=2){if(!adv){adv=true;d0=along;}if(along-d0>=tailCm)goBack(now);}   // cleared: car length, then cut back in
    else if((!seen&&along>=rg+40)||along>=rg+150)goBack(now);
    break;}
   case BACK:
    {bool blk=look==0&&range>=0&&range<30; // the robot being overtaken is still in the way: stay parallel a bit longer
     float b=past?H+turn:H;tgt=blk?b:b-side*BACK_DEG;}
    if(!past&&along>=cornerCm-5)past=true; // beyond the corner apex: head for the NEXT straight (it lies on the track side)
    if((bl||bm||br)&&uint32_t(now-at)>=150){state=JOIN;d0=dist;adv=true;at=now;}
    else if((lat< -15&&!past)||uint32_t(now-at)>=6000){finish(false,now);return true;}
    break;
   case JOIN:{
    float ae=fabsf(norm(tgt-yaw));
    if(adv){if(dist-d0>=joinAdv){adv=false;tgt=past?H+turn:H;at=now;}break;}
    if((bm&&ae<20)||((bl||br)&&ae<30)){finish(true,now);return false;} // on the line and roughly aligned: the line follower finishes the turn
    if(uint32_t(now-at)>=3500){finish(bl||bm||br,now);return false;}
    break;}
   default:break;
  }
  float err=norm(tgt-yaw),v=speed;
  if(state!=PASS&&fabsf(err)>20)v=0;     // OUT / BACK / JOIN: pivot sharply on the spot first, then drive straight (no wide curve)
  else if(state==JOIN)v=JOIN_V;
  if(look==0&&range>=0&&range<STOP_CM&&state!=JOIN)v=0; // never drive into something straight ahead
  pd.steer(err,rate,v,PIVOT_MIN,l,r);
  return true;
 }
};
