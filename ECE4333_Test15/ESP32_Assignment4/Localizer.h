#pragma once
#include <math.h>
#include <stdio.h>
#include <string.h>
// Test15 track localizer (runs on the ESP32; the UNO flash is full).
//
// Everything on the Figure-8 is fixed, so the robot's position is ONE number: the distance P (cm) along the
// track from the start S (unwrapped: lap n is P in [n*lap, (n+1)*lap)). A 2-state extended Kalman filter
// estimates x = [P, k] (k = odometer scale) from the UNO's 10 Hz samples (N230):
//   predict : P += k * dRaw          (dRaw = raw wheel-model distance; along-track part only during a detour)
//   update  : at every fixed landmark the UNO's state machine reports:
//             corner pivot starts (state 3)  -> corners D, E, A, B, C   (axle CORNER_OFS before the apex)
//             crossing pattern (state 2)     -> intersection O, twice a lap (axle SENSOR_AHEAD before O)
//             the landmark is the nearest one of that type to the predicted P (gate 60 cm, 3-sigma test).
// The UNO gets the result as N218 {D1 segment, D2 progress mm, D3 raw odometer mm, D4 k x1000}: it then knows
// exactly where it is on its segment (distance to O for the STOP line, distance to the next corner for passing).
// The heading drift (gyro vs the known straight heading) is estimated on straights and logged (H).
// Log lines pushed into the map log for the web page:
//   N240 {q, P mm, K x1000, S = 1-sigma of P in mm, E = landmark id (-1 none), I = innovation mm, H = heading drift x10}
//   N241 {q, O = 1 STOP sign / 2 traffic light / 3 obstacle, P mm, R = range cm ahead (-1 unknown)}
struct TrackEKF{
 static constexpr float CORNER_OFS=5.0f,SENSOR_AHEAD=8.0f,GATE=60.0f;
 float len[8],start[9],lap=854;
 float P=0,k=1,p00=1,p01=0,p11=0.0025f,hOff=0,yawRef=0,lastRaw=0,lastInnov=0;
 int prevSm=-1,prevMode=0,lastEvent=-1,seg=0;uint16_t lastQ=0,events=0,rejected=0,mismatch=0,n=0;bool have=false;
 static constexpr float H8[8]={0,-137,-240,-240,-210,-120,0,0}; // map headings, same as UNO ROUTE_HEADING
 void setup(int so){
  static const int16_t L[8]={0,148,118,99,47,125,150,0};
  for(int i=0;i<8;i++)len[i]=L[i];len[0]=167-so;len[7]=so;
  start[0]=0;for(int i=0;i<8;i++)start[i+1]=start[i]+len[i];lap=start[8];
  P=0;k=1;p00=1;p01=0;p11=0.0025f;hOff=0;have=false;prevSm=-1;lastEvent=-1;events=rejected=mismatch=n=0;
 }
 // landmarks: 0 D, 1 E, 2 O (end of E-O), 3 A, 4 B, 5 C, 6 O (end of C-O) = start of segments 1..7
 float lmPos(int id)const{return start[id+1];}
 static bool isTurn(int id){return id!=2&&id!=6;}
 static float norm180(float a){a=fmodf(a+180.0f,360.0f);if(a<0)a+=360.0f;return a-180.0f;}
 float lapPos()const{float p=fmodf(P,lap);return p<0?p+lap:p;}
 void predict(float d,bool detour,float yaw){
  if(detour)d*=cosf((yaw-yawRef)*0.0174533f);              // only the along-track part of a detour counts
  float a=fabsf(d);
  float n00=p00+2*d*p01+d*d*p11+0.02f*a+(detour?0.10f*a:0),n01=p01+d*p11,n11=p11+1e-6f*a;
  P+=k*d;p00=n00;p01=n01;p11=n11;
 }
 bool update(float z,float R){
  float y=z-P,S=p00+R;
  if(fabsf(y)>15&&y*y>9*S){rejected++;return false;}      // 3-sigma test: not this landmark
  float K0=p00/S,K1=p01/S;P+=K0*y;k+=K1*y;if(k<0.7f)k=0.7f;if(k>1.4f)k=1.4f;
  float n00=(1-K0)*p00,n01=(1-K0)*p01,n11=p11-K1*p01;p00=n00;p01=n01;p11=n11>1e-6f?n11:1e-6f;
  lastInnov=y;events++;return true;
 }
 int landmark(bool turn){
  float lp=lapPos(),base=P-lp,best=1e9,z=0;int id=-1;
  for(int i=0;i<7;i++){if(isTurn(i)!=turn)continue;
   for(int w=-1;w<=1;w++){float q=lmPos(i)+w*lap,dd=fabsf(q-lp);if(dd<best){best=dd;id=i;z=base+q;}}}
  if(best>GATE){rejected++;return -1;}
  z-=turn?CORNER_OFS:SENSOR_AHEAD;
  return update(z,turn?9.0f:2.25f)?id:-1;
 }
 // segment progress for the UNO's segment (cm); false if the two disagree by more than 40 cm
 bool progress(int s,float &prog)const{
  prog=lapPos()-start[s];if(prog<-lap/2)prog+=lap;if(prog>lap/2)prog-=lap;
  return prog>-40&&prog<len[s]+40;
 }
};
constexpr float TrackEKF::H8[8];
