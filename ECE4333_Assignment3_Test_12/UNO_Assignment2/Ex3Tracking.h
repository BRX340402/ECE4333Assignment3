#pragma once
#include <stdint.h>
#include <math.h>
// Test11: optional ordered-route assistance; all steering still requires line evidence.
struct Ex3Tracking {
 // Signed vehicle turns: positive left. Unmeasured corners are editable estimates.
 uint8_t routeMode=1,routeIndex=0,laps=0,routeRevision=0;
 int16_t turns[5]={-150,-90,30,90,120};
 int16_t routeBase=0;
 void beginRoute(){routeIndex=laps=0;routeBase=0;}
 int16_t routeTarget()const{return routeBase+turns[routeIndex];}
 __attribute__((noinline)) bool nearTurn(float yaw)const{return abs((int16_t)yaw-routeTarget())< (abs(turns[routeIndex])<50?8:18);}
 __attribute__((noinline)) void acceptLine(float yaw){
  if(routeMode&&nearTurn(yaw)){
   routeBase=yaw;if(++routeIndex==5){routeIndex=0;if(laps<255)laps++;}
  }
  reset();trackYaw=yaw;headingKnown=true;
 }
 int blackLow=600,blackHigh=1023,rightDegrees=110,sweepDegrees=110;
 enum Failure:uint8_t {OK,LIFTED,GYRO_LOST,TIMED_OUT,WRONG_YAW,EXHAUSTED,BLOCKED,ANGLE_LIMIT,STALLED};
 uint8_t failure=OK,phase=0;
 bool searching=false,failed=false,settling=false,candidate=false,wide=false,headingKnown=false;
 int8_t lastSide=0,preferred=-1;
 uint32_t at=0,searchAt=0,candidateAt=0,wideAt=0,progressAt=0;
 float entryYaw=0,trackYaw=0,progressYaw=0;
 static const uint32_t MAX_SEARCH_MS=20000;
 __attribute__((noinline)) void reset(){searching=failed=settling=candidate=wide=headingKnown=false;failure=OK;phase=0;lastSide=0;}
 bool black(int v)const{return v>=blackLow&&v<=blackHigh;}
 void fail(uint8_t f){failed=true;failure=f;}
 __attribute__((noinline)) void step(int l,int m,int r,int speed,uint32_t now,float yaw,bool gyro,int &left,int &right,int rangeCm=-1,float rate=0){
  left=right=0;if(failed)return;if(speed<=0){reset();return;}
  if(!gyro){fail(GYRO_LOST);return;}
  bool bl=black(l),bm=black(m),br=black(r);
  if(!searching){
   if(bm){
    if(bl!=br)lastSide=bl?1:-1;
    if(!headingKnown){trackYaw=yaw;headingKnown=true;}
    if(bl&&br){if(!wide){wide=true;wideAt=now;}}else wide=false;
    if(!wide||uint32_t(now-wideAt)<900){
     // Centre still on line: heading/rate damping, never a side-triggered pivot.
     int u=(int)((yaw-trackYaw)*2+rate*0.3f);if(u>15)u=15;if(u< -15)u=-15;
     left=speed+u;right=speed-u;return;
    }
   }
   searching=true;phase=0;settling=candidate=false;entryYaw=progressYaw=yaw;
   preferred=routeMode?(turns[routeIndex]>0?1:-1):(bl!=br?(bl?1:-1):(lastSide?lastSide:-1));
   at=searchAt=progressAt=now;
  }
  float relative=yaw-entryYaw;
  // Continuous yaw, not wrapped heading: a full revolution cannot hide here.
  int rightLimit=rightDegrees>110?110:rightDegrees;
  int leftLimit=sweepDegrees>110?110:sweepDegrees;
  int16_t mapDelta=(int16_t)yaw-routeBase;
  if(routeMode){rightLimit=170;leftLimit=170;}
  if((routeMode&&abs(mapDelta)>170)||relative>leftLimit||relative< -rightLimit){fail(ANGLE_LIMIT);return;}
  if(uint32_t(now-searchAt)>=MAX_SEARCH_MS){fail(TIMED_OUT);return;}
  bool acceptable=bm&&!(bl&&br)&&(!routeMode||abs(mapDelta)<12||nearTurn(yaw));
  if(settling){
   if(acceptable){
    int creep=speed>45?45:speed;left=bl?creep/2:creep;right=br?creep/2:creep;
    if(uint32_t(now-candidateAt)>=220){acceptLine(yaw);}
    return;
   }
   settling=candidate=false;progressYaw=yaw;progressAt=now;
  }
  if(acceptable){
   if(!candidate){candidate=true;candidateAt=now;}
   if(uint32_t(now-candidateAt)>=60){settling=true;candidateAt=now;}
   return;
  }
  candidate=false;
  if(phase==0){
   // Observe for 30 ms, then local forward correction only while a side sees line.
   if(uint32_t(now-at)<250){
    if(uint32_t(now-at)>=30&&bl!=br){left=bl?speed/2:speed;right=br?speed/2:speed;}
    return;
   }
   phase=1;at=progressAt=now;progressYaw=yaw;
  }
  if(phase>6){fail(EXHAUSTED);return;}
  float error;
  if(routeMode&&phase>=3){
   if(phase>5){fail(EXHAUSTED);return;}
   int16_t target=routeTarget();
   if(phase==4)target+=preferred*12;
   if(phase==5)target-=preferred*12;
   error=target-yaw;
  }else{
   int8_t sign=(phase&1)?preferred:-preferred;
   int bound=sign>0?leftLimit:rightLimit;
   int amplitude=phase<=2?25:(phase<=4?60:bound-8);
   if(amplitude>bound-8)amplitude=bound-8;
   error=sign*amplitude-relative;
  }
  if(fabs(error)<4){phase++;at=progressAt=now;progressYaw=yaw;return;}
  if(fabs(yaw-progressYaw)>=2){progressYaw=yaw;progressAt=now;}
  if(uint32_t(now-progressAt)>2500){fail(STALLED);return;}
  // PWM100 breaks static friction; short duty near target leaves braking margin.
  uint16_t on=fabs(error)<15?35:80;
  if(uint32_t(now-at)%180>=on)return;
  if(fabs(error)<15&&fabs(rate)>60)return;
  if(error>0){left=-100;right=100;}else{left=100;right=-100;}
 }
};
