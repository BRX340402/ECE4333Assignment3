#pragma once
#include <stdint.h>
#include <math.h>
// Test10 mapping-run line follower. Pure logic (no Arduino calls) so it can be
// simulated on a PC (tests/sim_track.cpp).
//
// Track assumptions (figure 8): straight segments, sharp corners, one 60-degree crossing.
//  FOLLOW : centre on line -> drive; one side + centre -> gentle correction;
//           side only -> arc toward it, pivot after EDGE_MS.
//  CROSS  : both sides seen within PAIR_MS (crossing signature) -> ignore the
//           sensors and hold the line's heading for crossHold ms.
//  TURN   : line ran out on one side (corner) -> pivot toward that side until
//           the centre sensor finds a line. A line ~180 deg from the incoming
//           heading is the track we came from and is rejected (gyro only).
//  SEARCH : bounded sweep +-45, +-90 (with short forward creeps), +-wide, then FAIL.
// Yaw convention: degrees, + = left (counter-clockwise).
struct LineFollower {
 enum State:uint8_t{IDLE=0,FOLLOW=1,CROSS=2,TURN=3,SEARCH=4,FAIL=5};
 enum Fault:uint8_t{NONE=0,YAW_WRONG=10,SEARCH_EXHAUSTED=11,BLOCKED=14};
 enum Event:uint8_t{EV_CROSS=16,EV_TURN=32,EV_SEARCH=64};
 static const uint16_t PAIR_MS=600,EDGE_MS=150,GAP_MS=30,SIDE_MEMORY_MS=400,CONFIRM_MS=20,CREEP_MS=250,SETTLE_MS=200;
 static const int REVERSE_DEG=150,SEARCH_ACCEPT_DEG=150;
 static const uint16_t CENTRE_LOST_MS=40;
 // parameters (set by the sketch)
 int cruise=60,pivot=100,maxTurn=200,wideSweep=170,blockCm=15;uint16_t crossHold=400;float degPerMs=0.12f;
 // state
 State state=IDLE;uint8_t fault=NONE,faultState=0,events=0;
 float refYaw=0,yawNow=0,origin=0;
 uint32_t at=0,lastMs=0,leftAt=0,rightAt=0,centerAt=0,lineAt=0,edgeAt=0,confirmAt=0,followAt=0;
 bool leftSeen=false,rightSeen=false,edgeActive=false,confirming=false;
 int8_t lastSide=0,edgeSide=0,turnDir=0,searchPref=1,rot=0;uint8_t phase=0;int lastL=0,lastR=0;
 bool gyroOK=false,cwActive=false,flipped=false;
 // Gyro sign check: commanded pivot left but measured yaw went right by >25 deg.
 // The first time this happens the follower asks its owner to flip the gyro sign
 // (requestSignFlip) and keeps turning; a second time is a real fault.
 bool requestSignFlip=false,signChecked=false;
 void negateAngles(){yawNow=-yawNow;refYaw=-refYaw;origin=-origin;signChecked=true;}uint16_t dtMs=0;uint32_t cwAt=0;
 // tape-width class learned while following: narrow = only the centre sees a centred line,
 // wide = all three do. Wide tape cannot show the crossing signature (and needs none).
 uint32_t tNarrow=0,tWide=0;bool wide=false;
 uint32_t leftOnAt=0,rightOnAt=0;bool prevL=false,prevR=false,leftEver=false,rightEver=false; // rising edges; *Ever never cleared mid-run
 bool classified()const{return tNarrow+tWide>=1000;}
 // Route hint. The track is fixed: start at C facing D with yaw and distance zeroed at
 // Start, so every straight has a known heading (unwrapped; a figure-8 lap ends back at 0)
 // and length, and the next corner's direction is known. Only used when the sensors cannot
 // tell - both sides saw tape just before the line ended (the tape blob at a sharp corner's
 // apex: Test10 run 4 took the left branch at D this way) - and only in the last 30 % of a
 // straight, so the crossing and mid-straight wobbles keep the sensor rule.
 // seg advances when the learned line heading matches the next straight and at least half
 // of the current straight has been driven (a short corner overshoot cannot skip a straight).
 bool route=true;uint8_t seg=0;float distCm=0,segStart=0; // distCm: odometer, set by the owner every step
 static float routeHeading(uint8_t k){switch(k){case 1:return -136.6f;case 2:return -240.0f;case 3:return -210.0f;case 4:return -120.0f;default:return 0.0f;}}
 static float routeLength(uint8_t k){switch(k){case 1:return 151.0f;case 2:return 220.0f;case 3:return 43.0f;case 4:return 125.0f;default:return 320.0f;}}
 uint8_t nextSeg()const{return seg>=4?0:seg+1;}
 int8_t routeSide()const{
  if(!route||!gyroOK||distCm-segStart<0.7f*routeLength(seg))return 0;
  float d=norm180(routeHeading(nextSeg())-yawNow);
  return d>20?1:(d< -20?-1:0);
 }
 void routeUpdate(){
  if(!route||distCm-segStart<0.5f*routeLength(seg))return;
  uint8_t n=nextSeg();float dn=fabsf(norm180(refYaw-routeHeading(n)));
  if(dn<25&&dn<fabsf(norm180(refYaw-routeHeading(seg)))){seg=n;segStart=distCm;}
 }

 static float norm180(float a){a=fmodf(a+180.0f,360.0f);if(a<0)a+=360.0f;return a-180.0f;}
 void start(float yaw,uint32_t now){requestSignFlip=false;seg=0;segStart=distCm;tNarrow=tWide=0;wide=false;leftEver=rightEver=prevL=prevR=false;state=FOLLOW;fault=NONE;yawNow=yaw;refYaw=yaw;lastMs=now;rot=0;lastSide=0;enterFollow(now);centerAt=lineAt=now;}
 void stop(){state=IDLE;rot=0;}
 void fail(uint8_t why){if(state==FAIL)return;faultState=state;fault=why;state=FAIL;}
 void enterFollow(uint32_t now,bool keepSides=false){state=FOLLOW;at=followAt=now;if(!keepSides)leftSeen=rightSeen=false;edgeActive=false;confirming=false;}
 void enterCross(uint32_t now){state=CROSS;at=now;cwActive=false;events|=EV_CROSS;}
 // Corner direction: among sides that saw tape within SIDE_MEMORY_MS, the one whose
 // tape APPEARED most recently (rising edge). Riding offset keeps one side black for a
 // long time; the outgoing line of a corner shows up as a fresh onset on the other side.
 int8_t recentSide(uint32_t now)const{
  bool l=leftEver&&uint32_t(now-leftAt)<SIDE_MEMORY_MS,r=rightEver&&uint32_t(now-rightAt)<SIDE_MEMORY_MS;
  if(l&&r){int8_t h=routeSide();if(h)return h;return int32_t(leftOnAt-rightOnAt)>=0?1:-1;}
  return l?1:(r?-1:0);
 }
 void enterTurn(int8_t dir,uint32_t now){state=TURN;turnDir=dir;origin=yawNow;at=now;confirming=false;flipped=false;events|=EV_TURN;}
 void enterSearch(uint32_t now){state=SEARCH;origin=yawNow;phase=0;at=now;confirming=false;searchPref=recentSide(now);if(!searchPref)searchPref=routeSide();if(!searchPref)searchPref=lastSide?lastSide:1;events|=EV_SEARCH;}
 void pivotOut(int8_t dir,int &l,int &r)const{l=-dir*pivot;r=dir*pivot;}
 // centre found a line candidate: confirm for CONFIRM_MS, then follow it
 bool accept(bool ok,uint32_t now){
  if(!ok){confirming=false;return false;}
  if(!confirming){confirming=true;confirmAt=now;}
  if(uint32_t(now-confirmAt)>=CONFIRM_MS){refYaw=yawNow;routeUpdate();enterFollow(now);return true;}
  return false;
 }

 void step(bool bl,bool bm,bool br,uint32_t now,float gyroYaw,bool gyro,int rangeCm,int &left,int &right){
  uint32_t dt=now-lastMs;lastMs=now;dtMs=dt>1000?1000:dt;gyroOK=gyro;
  if(gyro)yawNow=gyroYaw;else yawNow+=rot*(float)dt*degPerMs; // timed estimate without gyro
  left=right=0;
  if(state==IDLE||state==FAIL){rot=0;return;}
  if(bl){if(!prevL)leftOnAt=now;leftAt=now;leftSeen=leftEver=true;}
  if(br){if(!prevR)rightOnAt=now;rightAt=now;rightSeen=rightEver=true;}
  prevL=bl;prevR=br;
  if(bm)centerAt=now;
  if(bl||bm||br)lineAt=now;
  switch(state){
   case FOLLOW:follow(bl,bm,br,now,left,right);break;
   case CROSS:cross(bl,bm,br,now,left,right);break;
   case TURN:turn(bl,bm,br,now,gyro,left,right);break;
   case SEARCH:search(bl,bm,br,now,gyro,rangeCm,left,right);break;
   default:break;
  }
  if(state==FAIL)left=right=0;
  rot=(left<0&&right>0)?1:((left>0&&right<0)?-1:0);
  lastL=left;lastR=right;
 }

 void follow(bool bl,bool bm,bool br,uint32_t now,int &left,int &right){
  bool recentL=leftSeen&&uint32_t(now-leftAt)<PAIR_MS,recentR=rightSeen&&uint32_t(now-rightAt)<PAIR_MS;
  bool recentC=uint32_t(now-centerAt)<PAIR_MS;
  // Crossing: the other line sweeps across the whole sensor row while the centre stays on ours.
  if(bm){if(!bl&&!br)tNarrow+=dtMs;else if(bl&&br)tWide+=dtMs;wide=tWide>tNarrow;}
  if(classified()&&!wide&&uint32_t(now-followAt)>=SETTLE_MS&&((bl&&br)||(recentC&&((bl&&recentR)||(br&&recentL))))){
   enterCross(now);cross(bl,bm,br,now,left,right);return;
  }
  if(bm){
   edgeActive=false;
   int k=cruise/3;
   if(bl&&!br){left=cruise-k;right=cruise;lastSide=1;}
   else if(br&&!bl){left=cruise;right=cruise-k;lastSide=-1;}
   else{
    left=right=cruise;
    // learn the line direction while centred (time constant ~0.25 s)
    bool centred=wide?(bl&&br):(!bl&&!br);
    if(centred&&uint32_t(now-followAt)>=SETTLE_MS){float a=dtMs/250.0f;if(a>1)a=1;refYaw+=norm180(yawNow-refYaw)*a;routeUpdate();}
   }
   return;
  }
  if(bl!=br){
   int8_t side=bl?1:-1;lastSide=side;
   if(!edgeActive||edgeSide!=side){edgeActive=true;edgeSide=side;edgeAt=now;}
   if(uint32_t(now-edgeAt)>=EDGE_MS){enterTurn(side,now);turn(bl,bm,br,now,gyroOK,left,right);return;}
   left=side>0?0:cruise;right=side>0?cruise:0;return;
  }
  edgeActive=false;
  if(bl&&br){left=right=cruise;return;} // both sides, centre white: keep straight
  // all white
  if(uint32_t(now-lineAt)<GAP_MS){left=lastL;right=lastR;return;}
  int8_t side=recentSide(now);
  if(side){lastSide=side;enterTurn(side,now);pivotOut(side,left,right);return;}
  enterSearch(now);search(bl,bm,br,now,gyroOK,-1,left,right);
 }

 void cross(bool bl,bool bm,bool br,uint32_t now,int &left,int &right){
  float err=norm180(refYaw-yawNow);
  int corr=(int)(err*2.0f);if(corr>20)corr=20;if(corr< -20)corr=-20;
  left=cruise-corr;right=cruise+corr;
  // All three white for CENTRE_LOST_MS: our line ended, so this 'crossing' was a
  // sharp corner. Pivot toward the side that saw tape last. (A side sensor alone
  // being black is ambiguous here - it may be the crossing line - so keep holding.)
  if(bl||bm||br)cwActive=false;else if(!cwActive){cwActive=true;cwAt=now;}
  if(cwActive&&uint32_t(now-cwAt)>=CENTRE_LOST_MS){
   int8_t side=recentSide(now);
   if(side){lastSide=side;enterTurn(side,now);}else enterSearch(now);
   return;
  }
  // Fixed-length hold, then normal following resumes (a crossing still under the
  // sensors simply re-triggers CROSS). Longer holds only accumulate heading drift.
  if(uint32_t(now-at)>=crossHold&&(bl||bm||br))enterFollow(now);
 }

 void turn(bool bl,bool bm,bool br,uint32_t now,bool gyro,int &left,int &right){
  float turned=(yawNow-origin)*turnDir;
  if(gyro&&turned< -25){if(!signChecked){requestSignFlip=true;pivotOut(turnDir,left,right);return;}fail(YAW_WRONG);return;}
  if(gyro&&turned>25)signChecked=true; // sign proven correct
  bool reverse=gyro&&fabs(norm180(yawNow-refYaw))>REVERSE_DEG;
  if(accept(bm&&!(bl&&br)&&!reverse,now))return;
  // Nothing acceptable that way (wrong guess, e.g. a zigzag blip on the other side
  // was the latest sighting): sweep back the other way once, up to a full turn.
  if(turned>(flipped?360:maxTurn)){
   if(flipped){enterSearch(now);return;}
   flipped=true;turnDir=-turnDir;origin=yawNow;confirming=false;
  }
  pivotOut(turnDir,left,right);
 }

 void search(bool bl,bool bm,bool br,uint32_t now,bool gyro,int rangeCm,int &left,int &right){
  bool forward=!gyro||fabs(norm180(yawNow-refYaw))<SEARCH_ACCEPT_DEG;
  if(accept(bm&&!(bl&&br)&&forward,now))return;
  // pattern (relative to where the search began, preferred side first):
  // +45,-45,0, +wide,-wide,0, creep, +45,-45,0, +wide,-wide,0, fail
  // Wide sweeps come before any creep: a sharp corner (up to ~140 deg) is reachable
  // by rotating, and creeping first would only move further past it.
  static const int16_t C=1000,F=2000;const int16_t W=(int16_t)wideSweep;
  const int16_t pattern[14]={45,-45,0,W,(int16_t)-W,0,C,45,-45,0,W,(int16_t)-W,0,F};
  if(phase>=14){fail(SEARCH_EXHAUSTED);return;}
  int16_t p=pattern[phase];
  if(p==F){fail(SEARCH_EXHAUSTED);return;}
  if(p==C){
   if(rangeCm>=0&&rangeCm<blockCm){fail(BLOCKED);return;}
   if(uint32_t(now-at)<CREEP_MS){
    float err=norm180(origin-yawNow);int corr=(int)(err*2.0f);if(corr>20)corr=20;if(corr< -20)corr=-20;
    left=cruise-corr;right=cruise+corr;return;
   }
   phase++;at=now;return;
  }
  float target=p*searchPref,rel=yawNow-origin;
  if(target>rel+2){pivotOut(1,left,right);return;}
  if(target<rel-2){pivotOut(-1,left,right);return;}
  phase++;at=now;
 }
};
