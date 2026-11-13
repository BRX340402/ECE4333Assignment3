#pragma once
#include <stdint.h>
#include <math.h>
#include "MapConfig.h"
// Test14 (Test10 line follower + Test14 race map: start at S in front of the traffic light).  Pure logic (no Arduino calls) so it can be
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
 enum Fault:uint8_t{NONE=0,YAW_WRONG=10,SEARCH_EXHAUSTED=11,BLOCKED=14,OVT_FAIL=15};
 enum Event:uint8_t{EV_CROSS=16,EV_TURN=32,EV_SEARCH=64};
 static const uint16_t PAIR_MS=600,EDGE_MS=150,GAP_MS=30,SIDE_MEMORY_MS=400,CONFIRM_MS=20,CREEP_MS=250,SETTLE_MS=200;
 static const int REVERSE_DEG=150,SEARCH_ACCEPT_DEG=150;
 static const uint16_t CENTRE_LOST_MS=40;
 // parameters (set by the sketch)
 int cruise=60,pivot=PIVOT_PWM,maxTurn=MAX_CORNER_DEG,wideSweep=WIDE_SEARCH_DEG,blockCm=CREEP_BLOCK_CM;uint16_t crossHold=CROSS_HOLD_MS;
 // odometer scale (learned from O-to-O laps), odometer at leaving O, last raw odometer, map heading steer weight (deg -> PWM)
 float odoK=1.0f,rawLast=0;
 // state
 State state=IDLE;uint8_t fault=NONE,faultState=0,events=0;
 float refYaw=0,yawNow=0,origin=0;
 uint32_t at=0,lastMs=0,leftAt=0,rightAt=0,centerAt=0,lineAt=0,edgeAt=0,confirmAt=0,followAt=0;
 bool leftSeen=false,rightSeen=false,edgeActive=false,confirming=false;
 int8_t lastSide=0,edgeSide=0,turnDir=0,searchPref=1,rot=0;uint8_t phase=0;int lastL=0,lastR=0;
 bool gyroOK=false,cwActive=false,flipped=false,prevAll=false;
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
 // ---- Test14 route map (MapConfig.h: ROUTE_*) ----
 // The track is fixed. Start S is on O-D in front of the traffic light, startOff cm after O, facing D, with
 // yaw and distance zeroed there, so every straight has a known heading (unwrapped; a lap ends back at 0)
 // and length. seg advances:
 //  * at a corner (D, E, A, B, C): the learned line heading matches the next straight and at least half of
 //    the current straight has been driven (a short corner overshoot cannot skip a straight);
 //  * at the intersection O (end of E-O and C-O): the crossing signature / all three sensors black near the
 //    expected place snaps the position exactly; odometry alone moves on 30 cm past it if the crossing is missed;
 //  * at S (end of O-S): by distance; this counts a lap.
 // The hint for an ambiguous corner (both sides saw tape just before the line ended) is only used in the
 // last 30 % of a straight.
 bool route=ROUTE_HINT;uint8_t seg=0,lapsDone=0;float distCm=0,segStart=0;int16_t startOff=START_FROM_O_CM; // distCm: odometer, set by the owner every step
 static float routeHeading(uint8_t k){return (int16_t)pgm_read_word(&ROUTE_HEADING[k&7]);}
 float routeLength(uint8_t k)const{k&=7;if(!k)return ROUTE_OD_CM-startOff;if(k==7)return startOff;return (int16_t)pgm_read_word(&ROUTE_LEN[k]);}
 static uint8_t nextOf(uint8_t k){return (k+1)&7;}
 uint8_t nextSeg()const{return nextOf(seg);}
 int8_t routeSide()const{
  if(!route||!gyroOK||segProg()<0.7f*routeLength(seg))return 0;
  float d=norm180(routeHeading(nextSeg())-yawNow);
  return d>20?1:(d< -20?-1:0);
 }
 void setSeg(uint8_t k,float start){seg=k&7;segStart=start;}
 // heading-based advance at the corners (called whenever refYaw is learned)
 void routeUpdate(){
  if(!route||seg==2||seg==6)return;          // these end at O: see atO() / routeTick()
  if(segProg()<0.5f*routeLength(seg))return;
  float dc=fabsf(norm180(refYaw-routeHeading(seg)));
  // try the next straight, then the one after it (the 47 cm A-B kink or the lap line S can be passed unnoticed)
  for(uint8_t k=nextSeg(),i=0;i<2;i++,k=nextOf(k)){
   float dn=fabsf(norm180(refYaw-routeHeading(k)));
   if(dn<25&&dn<dc){if(k<seg&&lapsDone<255)lapsDone++;setSeg(k,distCm);return;} // wrapped past S: a lap
   if(seg!=3&&seg!=7)break;
  }
 }
 // called every step: distance-based moves (lap line S, missed crossing)
 void routeTick(){
  if(!route)return;
  float p=segProg(),L=routeLength(seg);
  if((seg==2||seg==6)&&p>=L+30)setSeg(seg+1,distCm-(p-L));
  else if(seg==7&&p>=L){if(lapsDone<255)lapsDone++;setSeg(0,distCm-(p-L));}
 }
 // crossing signature at the intersection: exact position of O
 uint8_t snaps=0;
 void atO(){if(route&&(seg==2||seg==6)&&segProg()>=routeLength(seg)-60){
  // Test15: the odometer scale odoK now comes from the ESP32 track localizer (N218)
  setSeg(seg+1,distCm);if(snaps<255)snaps++;}}
 float segProg()const{return distCm-segStart;}
 // odometer: raw distance from the pose model -> true cm; the owner calls it once per step
 void odo(float raw){distCm+=(raw-rawLast)*odoK;rawLast=raw;}
 float toO()const{return routeLength(seg)-segProg();}   // distance left to O on E-O / C-O
 // following the line steadily (not just after a corner / crossing): the track heading is reliable
 bool settled(uint32_t now)const{return state==FOLLOW&&uint32_t(now-followAt)>=400&&fabsf(norm180(yawNow-refYaw))<12.0f;}
 // straight track left before the next corner (O and S are not corners: E-O-A and C-O-S-D are single straights)
 float toCorner()const{
  if(!route||!gyroOK)return 999;
  float e=routeLength(seg)-segProg();
  if(seg==2)e+=routeLength(3);else if(seg==6)e+=routeLength(7)+routeLength(0);else if(seg==7)e+=routeLength(0);
  return e<0?0:e;
 }
 // side of the TRACK that is outside of the loop: + left, - right
 int8_t outside()const{return routeOutside(seg);}

 static float norm180(float a){a=fmodf(a+180.0f,360.0f);if(a<0)a+=360.0f;return a-180.0f;}
 void start(float yaw,uint32_t now){requestSignFlip=false;rawLast=0;odoK=1.0f;seg=0;lapsDone=0;segStart=distCm;snaps=0;prevAll=false;tNarrow=tWide=0;wide=false;leftEver=rightEver=prevL=prevR=false;state=FOLLOW;fault=NONE;yawNow=yaw;refYaw=yaw;lastMs=now;rot=0;lastSide=0;enterFollow(now);centerAt=lineAt=now;}
 void stop(){state=IDLE;rot=0;}
 // back on the line after an overtake (Overtake.h): keep following with the track heading
 void resume(float yaw,float line,uint32_t now){yawNow=yaw;refYaw=line;lastMs=now;rot=0;enterFollow(now);centerAt=lineAt=now;}
 void fail(uint8_t why){if(state==FAIL)return;faultState=state;fault=why;state=FAIL;}
 void enterFollow(uint32_t now,bool keepSides=false){state=FOLLOW;at=followAt=now;if(!keepSides)leftSeen=rightSeen=false;edgeActive=false;confirming=false;}
 void enterCross(uint32_t now){state=CROSS;at=now;cwActive=false;events|=EV_CROSS;atO();}
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
  yawNow=gyroYaw; // Test14: no timed no-gyro mode (the race map needs the gyro; Start is refused without it)
  left=right=0;
  if(state==IDLE||state==FAIL){rot=0;return;}
  if(bl){if(!prevL)leftOnAt=now;leftAt=now;leftSeen=leftEver=true;}
  if(br){if(!prevR)rightOnAt=now;rightAt=now;rightSeen=rightEver=true;}
  prevL=bl;prevR=br;
  {bool all=bl&&bm&&br;if(all&&!prevAll&&(state==FOLLOW||state==CROSS))atO();prevAll=all;}
  routeTick();
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
  // pattern in flash: 45 deg, 99 = +wide sweep, -99 = -wide sweep, 100 = creep, 101 = fail
  static const int8_t PAT[14] PROGMEM={45,-45,0,99,-99,0,100,45,-45,0,99,-99,0,101};
  static const int16_t C=100,F=101;
  if(phase>=14){fail(SEARCH_EXHAUSTED);return;}
  int16_t p=(int8_t)pgm_read_byte(&PAT[phase]);if(p==99)p=wideSweep;else if(p==-99)p=-wideSweep;
  if(p==F){fail(SEARCH_EXHAUSTED);return;}
  if(p==C){
   if(rangeCm>=0&&rangeCm<blockCm){fail(BLOCKED);return;}
   if(uint32_t(now-at)<CREEP_MS){
    left=right=cruise;return; // straight creep (heading hold removed to save flash)
   }
   phase++;at=now;return;
  }
  float target=p*searchPref,rel=yawNow-origin;
  if(target>rel+2){pivotOut(1,left,right);return;}
  if(target<rel-2){pivotOut(-1,left,right);return;}
  phase++;at=now;
 }
};