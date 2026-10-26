#pragma once
#include <stdint.h>
#include <math.h>
#include "HeadingPD.h"

// ECE 4333 Assignment 4 TEST 2: reactive, Bug-2-inspired ultrasonic detour.
// NO preset obstacle width/length or preset rectangular path.
// The ultrasonic sensor and ESP32 camera share a single pan servo.
// Non-echo is weak evidence; require multiple readings AFTER seeing the object.
// Distances from the PWM model are estimates, not wheel encoders.
// IMPORTANT: SIDE and REAR margins are CHASSIS dimensions, NOT obstacle dimensions.
struct A4Bug2 {
 enum State:uint8_t {
  TRACK=0, WAIT=1, TURN_OUT=2,
  SIDE_SCAN=3, SIDE_FRONT=4, SIDE_STEP=5, SIDE_MARGIN=6,
  TURN_ALONG=7, PASS_SCAN=8, PASS_FRONT=9, PASS_STEP=10,
  REAR_MARGIN=11, TURN_IN=12, RETURN_STEP=13,
  TURN_JOIN=14, FAULT=15
 };
 State state=TRACK;
 // Measurements to replace after physically measuring robot. cm.
 static constexpr float ROBOT_WIDTH_CM=15.0f;
 static constexpr float SIDE_SAFETY_CM=5.0f;
 static constexpr float SENSOR_TO_REAR_CM=22.0f;
 static constexpr float REAR_SAFETY_CM=5.0f;
 // Maxima are FAULT guards, NEVER target distances.
 static constexpr float MAX_SIDE_CM=42.0f, MAX_PASS_CM=95.0f;
 static constexpr float STEP_CM=3.0f;
 static constexpr float MAX_RETURN_EXTRA_CM=12.0f;
 static constexpr int TRIGGER_CM=20, FRONT_STOP_CM=12;
 static constexpr int PAN_LEFT=30, PAN_CENTER=90, PAN_RIGHT=150;
 static constexpr int SIDE_OBSTACLE_CM=65;
 static constexpr int PIVOT_PWM=100, DRIVE_PWM=48;
 static constexpr uint32_t TURN_TIMEOUT_MS=4200;
 static constexpr uint32_t PING_TIMEOUT_MS=2200;
 static constexpr uint32_t TOTAL_TIMEOUT_MS=30000;
 HeadingPD pd;
 float H=0, target=0, hitDist=0, legStart=0;
 float sideDistance=0, passDistance=0, rearStart=0, returnStart=0;
 uint32_t entered=0, started=0, lastPing=0, lastGoodPing=0, cooldown=0;
 uint8_t nearCount=0, seen=0, farCount=0, silentCount=0, lineCount=0;
 int8_t side=0; // +1 left of track, -1 right of track.
 bool sawSide=false, sawRear=false;
 uint8_t doneCount=0,failCount=0;
 static float angle(float a){while(a>180)a-=360;while(a< -180)a+=360;return a;}
 bool active()const{return state!=TRACK&&state!=FAULT;}
 int desiredPan()const{
  if(state==SIDE_SCAN||state==PASS_SCAN)return side>0?PAN_RIGHT:PAN_LEFT;
  return PAN_CENTER; // forward ultrasonic before each movement.
 }
 void begin(State s,uint32_t t){
  state=s;entered=t;seen=farCount=silentCount=lineCount=0;
 }
 void reset(){state=TRACK;nearCount=seen=farCount=silentCount=lineCount=0;
  sawSide=sawRear=false;side=0;entered=started=lastPing=lastGoodPing=0;}
 void fault(uint32_t now){
  begin(FAULT,now);if(failCount<99)failCount++;
 }
 // Route hint is only for selecting OUTSIDE; do not use it for odometry replay.
 static int8_t outsideOfRoute(uint8_t segment){
  // segment sequence from Test11: C-O-D, D-E, E-O-A, A-B, B-C.
  static const int16_t headings[]={0,-137,-240,-210,-120};
  int a=headings[segment%5],b=headings[(segment+1)%5];
  float delta=angle((float)(b-a));
  return delta>15?-1:(delta< -15?1:0);
 }
 static bool nearIntersection(uint8_t seg,float progress){
  if(seg==0&&progress>95&&progress<195)return true;
  if(seg==2&&progress>65&&progress<165)return true;
  return false;
 }
 // fresh: NEW ultrasonic ping taken only after the servo settles at desiredPan().
 // range==-1 means timeout, NOT confirmed clear.
 bool step(uint32_t now,float yaw,float rate,float odom,
           bool gyroGood,int range,uint32_t pingAt,
           bool following,bool settled,float roadHeading,
           uint8_t segment,float segProgress,
           bool L,bool M,bool R,int &left,int &right) {
  left=right=0;
  if(state==FAULT)return true;
  if(state!=TRACK&&(!gyroGood||now-started>TOTAL_TIMEOUT_MS)){
   fault(now);return true;
  }
  const bool fresh=range!=-2&&pingAt!=lastPing;
  if(fresh){lastPing=pingAt;if(range>=0)lastGoodPing=now;}
  if(state==TRACK){
   if(!gyroGood||!following||!settled||now<cooldown)return false;
   if(fresh)nearCount=range>=0&&range<=TRIGGER_CM?
         (nearCount<3?nearCount+1:3):0;
   if(nearCount<3)return false;
   // The figure-8 crossing is NOT a safe place for a 90-degree detour.
   if(nearIntersection(segment,segProgress)||
      segProgress>=(segment==0?320.0f:segment==1?151.0f:
        segment==2?220.0f:segment==3?43.0f:125.0f)-60.0f){
     begin(WAIT,now);return true;
   }
   side=outsideOfRoute(segment);
   if(!side){fault(now);return true;}
   H=roadHeading;hitDist=odom;sideDistance=passDistance=0;
   target=H+90.0f*side;started=now;
   begin(TURN_OUT,now);return true;
  }
  if(state==WAIT){
   if(fresh&&range>=TRIGGER_CM+10){
    if(++farCount>=3){begin(TRACK,now);cooldown=now+2500;}
   }else if(fresh)farCount=0;
   if(now-entered>9000)fault(now); // no forced pass at a corner
   return true;
  }
  float err=angle(target-yaw);
  if(state==TURN_OUT||state==TURN_ALONG||state==TURN_IN||state==TURN_JOIN){
   if(fabsf(err)<7.0f&&fabsf(rate)<35.0f){
    if(state==TURN_OUT){legStart=odom;begin(SIDE_SCAN,now);}
    else if(state==TURN_ALONG){legStart=odom;begin(PASS_SCAN,now);}
    else if(state==TURN_IN){returnStart=odom;begin(RETURN_STEP,now);}
    else { // line AND heading must be stable before resuming Test11.
     if(M){if(lineCount<4)lineCount++;}
     else lineCount=0;
     if(lineCount>=3){begin(TRACK,now);cooldown=now+2800;
      if(doneCount<99)doneCount++;return false;}
    }
    return true;
   }
   if(now-entered>TURN_TIMEOUT_MS){fault(now);return true;}
   pd.steer(err,rate,0,PIVOT_PWM,left,right);
   return true;
  }
  if(now-entered>PING_TIMEOUT_MS &&
     (state==SIDE_SCAN||state==PASS_SCAN||state==SIDE_FRONT||state==PASS_FRONT) &&
     now-lastPing>PING_TIMEOUT_MS){fault(now);return true;}
  float travel=odom-legStart;
  // Measurement phases are stationary. A confirmed "far" observation may be
  // either valid >65 cm OR consecutive no-echo AFTER near detections.
  // Non-echo is never enough by itself to start a detour.
  if(state==SIDE_SCAN||state==PASS_SCAN){
   if(!fresh)return true;
   if(range>=0&&range<=SIDE_OBSTACLE_CM){
    if(range<8){fault(now);return true;}
    if(seen<6)seen++;farCount=silentCount=0;
    if(state==SIDE_SCAN)sawSide=true;else sawRear=true;
   }else if(range>SIDE_OBSTACLE_CM){
    if(farCount<6)farCount++;silentCount=0;
   }else if(range<0){
    if(silentCount<8)silentCount++;
   }
   bool edge=(farCount>=3||silentCount>=5);
   if(state==SIDE_SCAN){
    if(sawSide&&travel>=3&&edge){
     legStart=odom;begin(SIDE_MARGIN,now);return true;
    }
    if(travel>MAX_SIDE_CM){fault(now);return true;}
    begin(SIDE_FRONT,now);
   }else {
    if(sawRear&&passDistance>=3&&edge){
     rearStart=odom;begin(REAR_MARGIN,now);return true;
    }
    if(passDistance>MAX_PASS_CM){fault(now);return true;}
    begin(PASS_FRONT,now);
   }
   return true;
  }
  if(state==SIDE_FRONT||state==PASS_FRONT){
   if(!fresh)return true;
   if(range<0){ // no forward echo: retry, but never drive on unknown.
    if(++silentCount>=4)fault(now);
    return true;
   }
   if(range<=FRONT_STOP_CM+5){fault(now);return true;}
   legStart=odom;
   begin(state==SIDE_FRONT?SIDE_STEP:PASS_STEP,now);
   return true;
  }
  if(state==SIDE_STEP){
   if(travel>=STEP_CM){
    sideDistance+=travel;
    legStart=odom;
    begin(SIDE_SCAN,now);return true;
   }
   if(sideDistance+travel>=MAX_SIDE_CM){fault(now);return true;}
  }
  if(state==PASS_STEP){
   if(travel>=STEP_CM){
    passDistance+=travel;
    legStart=odom;
    begin(PASS_SCAN,now);return true;
   }
   if(passDistance+travel>=MAX_PASS_CM){fault(now);return true;}
  }
  if(state==SIDE_MARGIN){
   // Offset after detecting lateral edge = robot half-width + safety.
   if(travel>=ROBOT_WIDTH_CM*0.5f+SIDE_SAFETY_CM){
    sideDistance+=travel;target=H;
    begin(TURN_ALONG,now);return true;
   }
  }
  if(state==REAR_MARGIN){
   // Advance rear-overhang distance AFTER obstacle's observed end.
   if(travel>=SENSOR_TO_REAR_CM+REAR_SAFETY_CM){
    passDistance+=travel;
    target=H-90.0f*side;begin(TURN_IN,now);return true;
   }
  }
  if(state==RETURN_STEP){
   // Line is truth; estimated lateral displacement is only a guard.
   float minTravel=sideDistance*0.55f;
   if(minTravel<5)minTravel=5;
   if(odom-returnStart>=minTravel&&(L||M||R)){
    if(++lineCount>=3){target=H;begin(TURN_JOIN,now);return true;}
   }else lineCount=0;
   if(odom-returnStart>sideDistance+MAX_RETURN_EXTRA_CM){
    fault(now);return true;
   }
  }
  if(state==SIDE_STEP||state==SIDE_MARGIN||state==PASS_STEP||
     state==REAR_MARGIN||state==RETURN_STEP){
   // When scanning is needed, stop before turning the servo.
   pd.steer(angle(target-yaw),rate,DRIVE_PWM,PIVOT_PWM,left,right);
  }
  return true;
 }
};
