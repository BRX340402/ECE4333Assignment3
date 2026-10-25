#pragma once
#include <stdint.h>
#include <math.h>
#include "HeadingPD.h"
#include "MapConfig.h"
// Overtaking state machine (Assignment 4): Bug-inspired, preset-distance detour around an obstacle / slower robot on the
// OUTSIDE of the track, then back onto the line (the m-line) and normal figure-8 tracking.
//
//   IDLE      following the line; the forward ultrasonic is watched (3 readings <= trigCm = obstacle)
//   WAIT      not enough free track ahead (corner / intersection) -> stand behind it until it clears
//   OUT_TURN  pivot to (track heading + side*90 deg)          [side: + left, - right = outside]
//   OUT_DRIVE drive outCm sideways, away from the line (clearance = obstacle half width + robot half width + margin)
//   ALIGN     pivot back to the track heading
//   PASS      drive parCm parallel to the line, beside the obstacle
//   RET_TURN  pivot towards the line (90 deg)
//   RET_DRIVE drive until a line sensor sees the line again (Bug leave condition: m-line re-encountered)
//   JOIN      turn onto the line direction, then hand back to the line follower
//   FAIL      detour impossible (line not found, blocked again): stop, fault code 15
// The only range sensor looks forward, so the boundary of the obstacle is not followed with a sensor:
// the detour is dead-reckoned with the gyro heading (HeadingPD) and the odometer. parCm and outCm
// are tuning values (obstacle length / width + robot size).
// Pure logic (no Arduino calls): tests/ovt_sim.cpp simulates it on the figure-8 on a PC.
struct Overtake{
 enum State:uint8_t{IDLE=0,WAIT=1,OUT_TURN=2,OUT_DRIVE=3,ALIGN=4,PASS=5,RET_TURN=6,RET_DRIVE=7,JOIN=8,FAIL=9};
 static const int OUT_DEG=90,RET_DEG=90,RET_MAX_CM=90,PIVOT_MIN=50,JOIN_V=35; // 90 deg side-steps: the detour needs only parCm of straight track (a 45 deg detour needs ~125 cm, more than this track has between the intersection and the corners)
 static const uint16_t WAIT_MAX_MS=6000,COOL_MS=2500,TURN_TIMEOUT_MS=3500;
 // parameters (set by the sketch / web page)
 static const int SENSOR_FORWARD_CM=15; // provisional: 22 cm body minus 7 cm rear overhang; measure from drive axle
 bool enabled=false;int8_t sideOpt=1;int trigCm=10,parCm=26,outCm=12,speed=55,joinAdv=15,rg=0; // parCm = obstacle length + clearance behind it (a moving robot: multiply by speed/(speed - its speed))
 HeadingPD pd;
 // state
 State state=IDLE;int8_t side=1;float H=0,tgt=0,d0=0,path=0,along=0,lostCm=0;bool adv=false;
 uint32_t at=0,rdAt=0,doneAt=0;uint8_t near=0,clear=0,ok=0,fails=0;bool handover=false,cool=false,paused=false;uint32_t pauseAt=0,joinAt=0;bool joinSeen=false;
 float need()const{return rg+SENSOR_FORWARD_CM+parCm+40;} // estimated pass travel plus 40 cm before a corner/intersection
 bool active()const{return state!=IDLE&&state!=FAIL;}
 void reset(){state=IDLE;near=clear=0;handover=false;cool=false;paused=false;joinSeen=false;}
 void done(uint32_t now){doneAt=now;cool=true;near=0;}
 void go(State s,uint32_t now){state=s;at=now;joinSeen=false;}
 void pause(bool stop,uint32_t now){
  if(stop&&!paused){paused=true;pauseAt=now;}
  if(!stop&&paused){uint32_t dt=now-pauseAt;at+=dt;rdAt=now;if(cool)doneAt+=dt;if(joinSeen)joinAt+=dt;paused=false;}
 }
 // Called every loop pass while the line follower owns the car. Returns true when Overtake drives the wheels.
 //  following: line follower in FOLLOW and no traffic stop; ready: it has been following steadily (LineFollower::settled); lineYaw: learned track heading; outside: + left / - right (0 unknown);
 //  freeCm: straight track left before a corner / the intersection window; toCorner: track left before the corner; range: ultrasonic cm (-1 none, also -1 while the camera is panned)
 bool step(float yaw,float rate,float dist,int range,bool following,bool ready,float lineYaw,int8_t outside,float freeCm,float toCorner,bool bl,bool bm,bool br,uint32_t now,int &l,int &r){
  l=r=0;
  bool fresh=uint32_t(now-rdAt)>=100;if(fresh)rdAt=now;
  bool hit=range>=0&&range<=trigCm;
  if(fresh){near=hit?(near<9?near+1:9):0;clear=(range<0||range>trigCm+8)?(clear<9?clear+1:9):0;}
  float err=0;
  switch(state){
   case IDLE:
    if(!enabled||!following||(cool&&uint32_t(now-doneAt)<COOL_MS))return false;
    if(near<3||toCorner<25)return false; // (a corner is about to be taken: the obstacle is probably beyond it - decide after the turn)
    rg=range;path=along=lostCm=0;adv=false;side=sideOpt?sideOpt:(outside?outside:1);H=lineYaw;
    if(freeCm<need()){go(WAIT,now);return true;} // not enough straight track ahead: wait behind it
    if(!ready&&range>22)return false; // just after a corner the track heading is not reliable yet: keep following (the car is still far enough)
    tgt=H+side*OUT_DEG;go(OUT_TURN,now);return true;
   case WAIT:
    if(clear>=5){state=IDLE;done(now);return false;}
    if(uint32_t(now-at)>=WAIT_MAX_MS&&freeCm>=0.85f*need()){tgt=H+side*OUT_DEG;go(OUT_TURN,now);return true;} // it is not moving and there is nearly room: pass anyway
    return true;
   case FAIL:return true;
   default:break;
  }
  // ---- maneuver ----
  err=norm(tgt-yaw);float ae=err<0?-err:err;
  bool turning=state==OUT_TURN||state==ALIGN||state==RET_TURN;
  if(turning){
   pd.steer(err,rate,0,PIVOT_MIN,l,r);
   if(ae<6&&rate<30&&rate>-30){
    if(state==OUT_TURN){d0=dist;go(OUT_DRIVE,now);}
    else if(state==ALIGN){d0=dist;go(PASS,now);}
    else{d0=dist;go(RET_DRIVE,now);}
   }else if(uint32_t(now-at)>=TURN_TIMEOUT_MS){fail(now);l=r=0;}
   return true;
  }
  float run=dist-d0;
  switch(state){
   case OUT_DRIVE:
    if(run>=outCm){path+=run;tgt=H;go(ALIGN,now);}
    break;
   case PASS:
    if(range>=0&&range<=FORWARD_ESTOP_CM){l=r=0;if(uint32_t(now-at)>6000)fail(now);return true;} // something else ahead in the pass lane
    if(run>=rg+SENSOR_FORWARD_CM+parCm){path+=run;along+=run;tgt=H-side*RET_DEG;go(RET_TURN,now);}
    break;
   case RET_DRIVE:
    if((bl||bm||br)&&run>=8){path+=run;d0=dist;adv=true;go(JOIN,now);break;} // keep the return heading until the wheel axle is over the line
    if(run>=RET_MAX_CM)fail(now);
    break;
   case JOIN:
    if(adv){if(run>=joinAdv){path+=run;adv=false;tgt=H;at=now;}break;}
    if(bm&&ae<20){if(!joinSeen){joinSeen=true;joinAt=now;}if(uint32_t(now-joinAt)>=150){lostCm=path-along;ok++;handover=true;state=IDLE;done(now);return false;}}else joinSeen=false;
    if(uint32_t(now-at)>=TURN_TIMEOUT_MS)fail(now);
    break;
   default:break;
  }
  if(state==FAIL)return true;
  float v=speed;if(state==JOIN)v=adv?JOIN_V:(ae>25?0:JOIN_V);
  if(range>=0&&range<=FORWARD_ESTOP_CM&&state!=PASS&&state!=JOIN)v=0; // never drive into something ahead
  pd.steer(err,rate,v,PIVOT_MIN,l,r);
  return true;
 }
 void fail(uint32_t now){state=FAIL;at=now;if(fails<99)fails++;}
 static float norm(float a){a=fmodf(a+180.0f,360.0f);if(a<0)a+=360.0f;return a-180.0f;}
};
