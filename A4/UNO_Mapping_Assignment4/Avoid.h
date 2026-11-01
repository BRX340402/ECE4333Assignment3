#pragma once
#include <stdint.h>
#include <math.h>
#include "HeadingPD.h"
// Test14 obstacle avoidance + overtaking (Bug algorithm), replaces Test13 Overtake.h. Pure logic, no Arduino calls.
//
// Bug idea: the line is the m-line. When the forward ultrasonic hits something, leave the m-line on the
// OUTSIDE of the loop, follow the obstacle boundary with the (panned) ultrasonic until it is behind, then
// head back to the m-line and continue the race. A static obstacle (the moose) and a slower robot (overtaking)
// use the same manoeuvre: the side scan simply lasts longer for a moving robot.
//
//   IDLE  line following at race speed; ultrasonic straight ahead. 2 fresh readings <= trigCm = obstacle.
//         -> TAIL if passing is not allowed here (STOP-sign segment) or the straight left before the next
//            corner is shorter than need(); otherwise -> OUT. (No E-stop: the car never stops for it.)
//   TAIL  keep following the line behind it, speed scaled by the gap (stand at gapCm, full speed at trigCm).
//         As soon as passing is allowed and there is room -> OUT. It pulled away (1 s nothing) -> IDLE.
//         Parked for 5 s near a corner with >= 60 % of the room -> OUT anyway.
//   OUT   curve out at race speed to (track heading + side*45 deg) until the car is latCm beside the line.
//   PASS  drive parallel to the line; the servo turns the ultrasonic SIDEWAYS at the obstacle ("turn around
//         and check it"). The closest side echo is recorded; when the echo is gone or > recorded + margin
//         (default 6 cm) on 2 readings, the obstacle is behind the sensor; drive tailCm more (car length),
//         or give up waiting after rg+30 cm (never seen) / rg+150 cm (a robot as fast as us).
//   BACK  servo straight again; curve back at (track heading - side*45 deg) until a line sensor sees the line.
//   JOIN  move the axle over the line, turn onto it, hand over to the line follower (handover; lostCm = extra
//         odometer distance so the map progress stays right). Line not found -> handover with lost=true
//         (the line follower searches).
struct Avoid{
 enum State:uint8_t{IDLE=0,TAIL=1,OUT=2,PASS=3,BACK=4,JOIN=5};
 static const int OUT_DEG=45,PIVOT_MIN=50,JOIN_V=40,STOP_CM=10;
 // parameters (set by the sketch from the web page)
 bool enabled=false;int trigCm=35,latCm=20,margin=6,tailCm=15,gapCm=18,speed=60,joinAdv=8;
 HeadingPD pd;
 // state
 State state=IDLE;int8_t side=1,look=0;float H=0,tgt=0,lat=0,along=0,lastDist=0,startDist=0,d0=0,lostCm=0;
 int rg=0,rec=0,lastRange=-1;uint32_t at=0,doneAt=0;uint8_t near=0,far=0,seen=0,gone=0,ok=0,fails=0,scale=100;
 bool handover=false,lost=false,adv=false,cool=false;
 // along-track length the manoeuvre needs (detection range + 45 deg out + obstacle + car length + back + join)
 float need()const{return rg+latCm+tailCm+30;}
 bool active()const{return state>=OUT;}
 void reset(){state=IDLE;look=0;near=far=0;handover=lost=cool=false;scale=100;}
 static float norm(float a){a=fmodf(a+180.0f,360.0f);if(a<0)a+=360.0f;return a-180.0f;}
 void begin(float lineYaw,int8_t s,float dist,uint32_t now){
  H=lineYaw;side=s?s:1;tgt=H+side*OUT_DEG;lat=along=0;startDist=lastDist=dist;state=OUT;at=now;look=0;seen=gone=0;adv=false;
 }
 void finish(bool found,uint32_t now){lostCm=(lastDist-startDist)-along;handover=true;lost=!found;if(found){if(ok<99)ok++;}else if(fails<99)fails++;
  state=IDLE;look=0;doneAt=now;cool=true;near=0;}
 void goBack(uint32_t now){state=BACK;look=0;tgt=H-side*OUT_DEG;at=now;}
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
   near=hit?(near<9?near+1:9):0;far=(range<0||range>trigCm+15)?(far<20?far+1:20):0;
  }
  switch(state){
   case IDLE:
    if(!enabled||!following||(cool&&uint32_t(now-doneAt)<1500))return false;
    if(near<2||toCorner<25)return false;     // (just before a corner the obstacle is probably beyond it: decide after the turn)
    rg=lastRange;
    if(noPass||toCorner<need()){state=TAIL;at=now;far=0;return false;}
    begin(lineYaw,outside,dist,now);break;
   case TAIL:{
    if(!enabled||far>=10){state=IDLE;return false;}
    int u=lastRange;scale=u<0?100:(u<=gapCm?0:(u>=trigCm?100:(uint8_t)((u-gapCm)*100L/(trigCm-gapCm))));
    if(scale)at=now;
    if(following&&!noPass&&near&&(toCorner>=need()||(uint32_t(now-at)>=5000&&toCorner>=0.6f*need()))){rg=u<0?trigCm:u;begin(lineYaw,outside,dist,now);break;}
    return false;}                           // the line follower keeps steering, the sketch scales its speed
   default:break;
  }
  switch(state){
   case OUT:
    if(lat>=latCm){state=PASS;tgt=H;look=-side;at=now;rec=999;}
    else if(uint32_t(now-at)>=5000){finish(false,now);return true;}
    break;
   case PASS:
    if(valid&&look==-side){              // side echo (the sketch passes -2 until the servo has settled)
     if(range>=0&&range<=latCm+25){if(range<rec)rec=range;if(seen<9)seen++;gone=0;}
     else if(seen&&(range<0||range>rec+margin)){if(gone<9)gone++;}
    }
    if(gone>=2){if(!adv){adv=true;d0=along;}if(along-d0>=tailCm)goBack(now);}
    else if((!seen&&along>=rg+30)||along>=rg+150)goBack(now);
    break;
   case BACK:
    if((bl||bm||br)&&uint32_t(now-at)>=150){state=JOIN;d0=dist;adv=true;at=now;}
    else if(lat< -15||uint32_t(now-at)>=6000){finish(false,now);return true;}
    break;
   case JOIN:{
    float ae=fabsf(norm(tgt-yaw));
    if(adv){if(dist-d0>=joinAdv){adv=false;tgt=H;at=now;}break;}
    if(bm&&ae<20){finish(true,now);return false;}
    if(uint32_t(now-at)>=3500){finish(bl||bm||br,now);return false;}
    break;}
   default:break;
  }
  float err=norm(tgt-yaw),v=speed;
  if(state==JOIN)v=adv?JOIN_V:(fabsf(err)>25?0:JOIN_V);
  if(look==0&&range>=0&&range<STOP_CM&&state!=JOIN)v=0; // never drive into something straight ahead
  pd.steer(err,rate,v,PIVOT_MIN,l,r);
  return true;
 }
};
