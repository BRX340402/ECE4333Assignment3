// Closed-loop simulation of TargetFollow (camera outer loop + gyro PD inner loop) on the PC.
//   g++ -std=c++14 -I../UNO_Mapping follow_test.cpp -o ft && ./ft
// Plant: differential drive, 14 cm track, 0.33 cm/s per PWM, dead band 30 PWM, 60 ms motor lag.
// Camera: 5 frames/s, each frame is 250 ms old when it reaches the controller (ESP32 decode + UART).
#include <cstdio>
#include <cmath>
#include <deque>
#include <cstdint>
#include "TargetFollow.h"
static int fails=0;
static void check(bool ok,const char*w){printf("%s %s\n",ok?"PASS":"FAIL",w);if(!ok)fails++;}
static const double DEG=M_PI/180;
struct Pose{double x,y,th;};
struct World{
 Pose car{0,0,0};double vl=0,vr=0,gyroBias=0.3;double tx=200,ty=40,size=10;bool visible=true;
 std::deque<Pose> hist;double yawDeg=0,rate=0;uint32_t now=1000;
 double bearing(const Pose&p)const{double b=atan2(ty-p.y,tx-p.x)-p.th;while(b>M_PI)b-=2*M_PI;while(b<-M_PI)b+=2*M_PI;return b;}
 double dist(const Pose&p)const{return hypot(tx-p.x,ty-p.y);}
 void plant(int L,int R,double dt){
  auto spd=[&](int pwm){double a=fabs(pwm);if(a<30)return 0.0;return (pwm>0?1:-1)*0.33*(a-18);};
  double k=dt/0.06;if(k>1)k=1;vl+=(spd(L)-vl)*k;vr+=(spd(R)-vr)*k;
  double v=(vl+vr)/2,w=(vr-vl)/14.0;
  car.x+=v*cos(car.th+w*dt/2)*dt;car.y+=v*sin(car.th+w*dt/2)*dt;car.th+=w*dt;
  rate=w/DEG+gyroBias;yawDeg+=rate*dt; // gyro: yaw in degrees (+ left), small bias
 }
};
struct Result{bool arrived;double tArrive,finalDist,maxAbsBear,tLost;int state;};
// mirror: camera image is flipped (negative FOV tells the controller)
Result run(World&w,TargetFollow&f,int fovTrue,bool mirror,double seconds,bool moveTarget=false,double hideFrom=-1,double hideTo=-1){
 uint16_t seq=0;uint32_t lastFrame=w.now;f.start((float)w.yawDeg,w.now);Result r{false,0,0,0,0,0};
 uint32_t t0=w.now;int L=0,R=0;
 for(;w.now-t0<seconds*1000;){
  w.now+=5;double t=(w.now-t0)/1000.0;
  w.hist.push_back(w.car);if(w.hist.size()>200)w.hist.pop_front();
  if(moveTarget){w.tx=200+40*sin(t*0.2);w.ty=40+30*cos(t*0.17);}
  bool hidden=hideFrom>=0&&t>=hideFrom&&t<hideTo;
  if(w.now-lastFrame>=200){lastFrame=w.now; // a frame captured 250 ms ago arrives now
   size_t back=std::min<size_t>(50,w.hist.size()-1);Pose c=w.hist[w.hist.size()-1-back];
   double b=w.bearing(c),d=w.dist(c);bool seen=!hidden&&fabs(b)<fovTrue/2.0*DEG&&d<400;
   int x=500,wp=0;if(seen){x=(int)(500-(mirror?-1:1)*b/(fovTrue*DEG)*1000);wp=(int)(1000*(2*atan(w.size/2/d))/(fovTrue*DEG));}
   f.measure(++seq,true,seen,x,wp,250);}
  f.step((float)w.yawDeg,(float)w.rate,w.now,L,R);
  w.plant(std::max(-180,std::min(180,L)),std::max(-180,std::min(180,R)),0.005);
  double bn=fabs(w.bearing(w.car))/DEG;if(t>3&&bn>r.maxAbsBear)r.maxAbsBear=bn;
  if(!r.arrived&&f.state==TargetFollow::ARRIVED){r.arrived=true;r.tArrive=t;}
 }
 r.finalDist=w.dist(w.car);r.state=f.state;return r;
}
int main(){
 { World w;TargetFollow f;f.speed=70;f.fov=62;Result r=run(w,f,62,false,25);
   check(r.arrived,"static target 40 cm to the left of the line of travel: car reaches the stop width");
   check(r.finalDist>20&&r.finalDist<60,"it stops in front of the target, not on top of it");
   check(fabs(w.bearing(w.car))/DEG<8,"and faces it (heading within 8 deg)");}
 { World w;w.tx=150;w.ty=-120; // target behind-right: outside the camera view at the start -> search first
   TargetFollow f;f.speed=70;Result r=run(w,f,62,false,60);
   check(r.arrived,"target outside the field of view: sweep finds it, then the car drives to it");}
 { World w;TargetFollow f;f.speed=70;f.fov=-62;Result r=run(w,f,62,true,25);
   check(r.arrived&&r.finalDist<60,"mirrored image with negative FOV works");}
 { World w;TargetFollow f;f.speed=70;f.fov=62;Result r=run(w,f,62,true,12);
   check(!r.arrived||r.finalDist>60,"mirrored image with positive FOV does NOT work (the web page has a mirror box for this)");}
 { World w;TargetFollow f;f.speed=70;Result r=run(w,f,62,false,40,true);
   check(r.finalDist<120&&r.maxAbsBear<40,"moving target: car keeps it in view and stays close (bearing < 40 deg)");}
 { World w;TargetFollow f;f.speed=70;Result r=run(w,f,62,false,12,false,3,12);
   check(r.state==TargetFollow::HOLD||r.state==TargetFollow::SWEEP,"target hidden for 9 s: car stops (HOLD) then searches");
   check(fabs(w.vl)+fabs(w.vr)<30,"no runaway while the target is lost");}
 { // outer loop really runs at 0.5 s: a measurement is only used when the period has elapsed
   TargetFollow f;f.start(0,1000);int L,R;f.measure(1,true,true,300,100,0);f.step(0,0,1100,L,R);
   check(f.state==TargetFollow::HOLD&&fabs(f.setYaw)<0.01f,"measurement is not used before the 0.5 s outer period");
   f.step(0,0,1510,L,R);check(f.state==TargetFollow::TRACK&&fabs(f.setYaw-12.4f)<0.2f,"at 0.5 s the outer loop sets the heading target = 12.4 deg left and starts tracking");}
 { // latency compensation: the frame is 250 ms old; the car yawed 20 deg since -> target heading uses the OLD yaw
   TargetFollow f;f.start(0,1000);int L,R;float yaw=0;
   for(uint32_t t=1000;t<1700;t+=10){yaw=(t<1400?0:(t-1400)*0.0667f);f.step(yaw,0,t,L,R);} // 20 deg in the last 300 ms
   f.measure(5,true,true,500,100,250);f.step(yaw,0,1720,L,R);
   check(fabsf(f.setYaw-0.0f)<4.0f,"latency compensation: set point = heading at capture time + bearing (not the current heading)");}
 { HeadingPD pd;check(fabsf(pd.out(10,0)-15)<0.01f&&fabsf(pd.out(10,50)-10)<0.01f,"PD: Kp 1.5 PWM/deg, Kd 0.10 PWM per deg/s");
   check(pd.out(1000,0)==70&&pd.out(-1000,0)==-70,"PD output limited to +/-70");}
 printf(fails?"FOLLOW TEST FAILED\n":"ALL FOLLOW CHECKS PASSED\n");return fails?1:0;}
