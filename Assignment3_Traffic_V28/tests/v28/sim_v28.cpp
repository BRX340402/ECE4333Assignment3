// Host simulation of the V28 UNO control code (PoseEstimate.h + TrackPlan.h are the
// REAL firmware headers).  Plant = simple differential-drive robot with errors.
// Build: g++ -O2 -std=c++17 -DA3_HOST_SIM -I../../UNO_Assignment2 sim_v28.cpp -o sim_v28
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <random>
#include "PoseEstimate.h"
#include "TrackPlan.h"
using namespace std;
static double g_rawGz=0;static bool g_i2cFail=false;
bool PoseEstimate::raw(int16_t &v){if(g_i2cFail)return false;double r=g_rawGz;if(r>32767)r=32767;if(r<-32768)r=-32768;v=(int16_t)lround(r);return true;}
struct V{double x,y;};
// true taped track (scaled/rotated copy of the nominal one)
static vector<V> trackVerts(double scale){
 vector<V> v;double x=0,y=0,h=0;const double d=M_PI/180;
 for(int i=0;i<PLAN_SEGMENTS;i++){h+=PLAN_TURN10[i]*0.1;x+=scale*PLAN_MM[i]*0.1*cos(h*d);y+=scale*PLAN_MM[i]*0.1*sin(h*d);v.push_back({x,y});}
 return v; // end of seg0=UL, UR, A, L, R, UL
}
static double distSeg(V p,V a,V b){double dx=b.x-a.x,dy=b.y-a.y,t=((p.x-a.x)*dx+(p.y-a.y)*dy)/(dx*dx+dy*dy);t=t<0?0:t>1?1:t;double ex=a.x+t*dx-p.x,ey=a.y+t*dy-p.y;return sqrt(ex*ex+ey*ey);}
int main(int argc,char**argv){
 double speedScale=1.0,trackScale=1.0,sensorAhead=7.0,biasDps=0.4,noiseDps=0.15,widthScale=1.0,biasDrift=0.0;int events=1,trim=1,cruise=80;int failI2C=0;unsigned seed=1;double tmax=240;const char*out="sim.csv";
 for(int i=1;i<argc;i++){
  if(!strncmp(argv[i],"--speed=",8))speedScale=atof(argv[i]+8);
  else if(!strncmp(argv[i],"--track=",8))trackScale=atof(argv[i]+8);
  else if(!strncmp(argv[i],"--ahead=",8))sensorAhead=atof(argv[i]+8);
  else if(!strncmp(argv[i],"--bias=",7))biasDps=atof(argv[i]+7);
  else if(!strncmp(argv[i],"--width=",8))widthScale=atof(argv[i]+8);
  else if(!strncmp(argv[i],"--events=",9))events=atoi(argv[i]+9);
  else if(!strncmp(argv[i],"--trim=",7))trim=atoi(argv[i]+7);
  else if(!strncmp(argv[i],"--pwm=",6))cruise=atoi(argv[i]+6);
  else if(!strncmp(argv[i],"--i2cfail=",10))failI2C=atoi(argv[i]+10);
  else if(!strncmp(argv[i],"--seed=",7))seed=atoi(argv[i]+7);
  else if(!strncmp(argv[i],"--out=",6))out=argv[i]+6;
 }
 mt19937 rng(seed);normal_distribution<double> nd(0,1);
 auto TV=trackVerts(trackScale);
 vector<pair<V,V>> lines={{TV[0],TV[1]},{TV[1],TV[2]},{TV[2],TV[3]},{TV[3],TV[4]},{TV[4],TV[0]}}; // UL-UR, UR-A, A-L, L-R, R-UL
 // robot truth.  Start point S is START_TO_UL before UL on line R-UL, pointing at UL.
 V UL=TV[0];(void)UL;
 double tx=-0.0,ty=0,th=0;                       // S = origin, heading 0 (UL is at +x)
 // true plant parameters (differ from the firmware's model on purpose)
 const double kTrue=0.45*speedScale,deadTrue=40,Wtrue=14.0*widthScale;
 double vlT=0,vrT=0;const double tau=0.10;
 PoseEstimate pose;TrackPlan plan;
 g_rawGz=biasDps*GYRO_LSB_PER_DPS/pose.gyroSign;   // raw = rate*LSB/sign + bias  (bias added below in raw units)
 double biasRaw=biasDps*GYRO_LSB_PER_DPS;
 g_rawGz=biasRaw;pose.begin();
 pose.reset();plan.reset(0);plan.lineEvent=events;plan.lineTrim=trim;
 int lPWM=0,rPWM=0;double t=0;const double dt=0.010;uint32_t nowMs=0;
 FILE*f=fopen(out,"w");fprintf(f,"t_s,true_x,true_y,true_h,fus_x,fus_y,fus_h,kin_x,kin_y,kin_h,yaw,seg,lap,phase,ev_err,ev_cnt,trimdeg,lat_err_cm,lPWM,rPWM\n");
 double maxLat=0,sumLat=0;long nLat=0;double nextLog=0;int finishedAt=-1;
 while(t<tmax){
  t+=dt;nowMs=(uint32_t)(t*1000+0.5);
  // sensors
  double c=cos(th),s=sin(th);double sp[3]; // L,M,R at +ahead forward, +-2cm lateral (left = +y local)
  int rd[3];double lat[3]={+2.0,0,-2.0};
  for(int k=0;k<3;k++){V p={tx+sensorAhead*c-lat[k]*s*(-1)*(-1)*(1),ty+sensorAhead*s+lat[k]*c};
   p.x=tx+sensorAhead*c-lat[k]*s;p.y=ty+sensorAhead*s+lat[k]*c;
   double dm=1e9;for(auto&L:lines){double d=distSeg(p,L.first,L.second);if(d<dm)dm=d;}
   sp[k]=dm;rd[k]=dm<1.2?700:300;}
  // gyro raw (+ drift, noise).  true yaw rate omega (rad/s) -> deg/s
  double omega=((vrT-vlT)/Wtrue);double rateDeg=omega*180/M_PI;
  biasRaw+=biasDrift*dt*GYRO_LSB_PER_DPS;
  g_rawGz=rateDeg*GYRO_LSB_PER_DPS/pose.gyroSign+biasRaw+nd(rng)*noiseDps*GYRO_LSB_PER_DPS;
  g_i2cFail=failI2C&&t>40&&t<40+failI2C*0.001;
  // ---- firmware loop body (mirrors UNO_Assignment2.ino mode 6, traffic state TRACK) ----
  pose.step(lPWM,rPWM,(uint32_t)(t*1e6));
  PlanIn in={pose.x,pose.y,pose.yawDegrees,pose.rateDps,pose.gyro};
  int L=0,R=0;float nx,ny;
  plan.step(in,rd[0],rd[1],rd[2],nowMs,cruise,L,R,nx,ny);
  if(nx||ny)pose.nudge(nx,ny);
  if(plan.evFlag){plan.evFlag=false;pose.learn(plan.evRatio);}
  if(plan.fault){printf("FAULT %d at t=%.1f seg %d\n",plan.fault,t,plan.seg);break;}
  if(plan.done){finishedAt=(int)t;L=R=0;}
  lPWM=L;rPWM=R;
  // ---- plant ----
  auto wheel=[&](int pwm){int a=abs(pwm)-(int)deadTrue;if(a<=0)return 0.0;return (pwm<0?-1.0:1.0)*kTrue*a;};
  vlT+=(wheel(lPWM)-vlT)*dt/tau;vrT+=(wheel(rPWM)-vrT)*dt/tau;
  double v=(vlT+vrT)/2;th+=omega*dt;tx+=v*cos(th)*dt;ty+=v*sin(th)*dt;
  double lat0=1e9;for(auto&Lh:lines){double d=distSeg({tx,ty},Lh.first,Lh.second);if(d<lat0)lat0=d;}
  if(plan.done&&fabs(v)<0.5){/*stopped*/}
  if(fabs(v)>1){maxLat=max(maxLat,lat0);sumLat+=lat0;nLat++;}
  if(t>=nextLog){nextLog+=0.4;fprintf(f,"%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%d,%d,%d,%.1f,%d,%.1f,%.1f,%d,%d\n",t,tx,ty,th*180/M_PI,pose.x,pose.y,pose.heading*180/M_PI,pose.kinX,pose.kinY,pose.kinHeading*180/M_PI,pose.yawDegrees,plan.seg,plan.lap,plan.phase,plan.evErr,plan.evCount,plan.trim,lat0,lPWM,rPWM);}
  if(plan.done&&fabs(vlT)<0.5&&fabs(vrT)<0.5)break;
 }
 fclose(f);
 printf("vScale=%.3f events=%d | end t=%.1fs laps=%d done=%d  true end (%.1f,%.1f) heading %.1f | max off-line %.1f cm, mean %.1f cm | fused (%.1f,%.1f) kin (%.1f,%.1f)\n",pose.vScale,plan.evCount,t,plan.lap,plan.done,tx,ty,th*180/M_PI,maxLat,nLat?sumLat/nLat:0,pose.x,pose.y,pose.kinX,pose.kinY);
 return 0;
}
