#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
#include "../ESP32_Assignment2/RedAction.h"
void frame(TrafficLogic &t,int seq,int px,int limit,bool green=false){t.colour(seq,true,redActionTriggered(px>0,px,limit),green,-1,seq*100,false,px>0);}
int main(){
 for(int stop=0;stop<=1;stop++){
  TrafficLogic t;t.stopOnRed=stop;t.start(0);
  frame(t,1,169,170);assert(t.state==TrafficLogic::TRACK&&!t.red);
  frame(t,2,169,170);assert(t.state==TrafficLogic::TRACK);
  frame(t,3,170,170);assert(t.state==TrafficLogic::VERIFY);assert(t.moving()==!stop);
 }
 TrafficLogic t;t.stopOnRed=true;
 frame(t,1,169,170);t.start(150);assert(t.state==TrafficLogic::TRACK); // Cached small red must not stop Start.
 frame(t,2,170,170);assert(!t.moving());
 frame(t,3,10,170,true);frame(t,4,10,170,true);assert(!t.moving()&&t.greens==0); // Small red still blocks release.
 frame(t,5,0,170,true);frame(t,6,0,170,true);assert(t.state==TrafficLogic::TRACK);
 assert(redActionTriggered(true,1,0));assert(!redActionTriggered(false,0,0));
 assert(!redActionTriggered(true,170,171));
 Ex3Tracking e;int l,r;assert(e.rightDegrees==180);e.rightDegrees=e.sweepDegrees=360;
 e.step(0,0,0,70,0,0,true,l,r);e.step(0,0,0,70,2500,-200,true,l,r);assert(e.phase==0&&l>0&&r<0);
 e.step(0,0,0,70,5000,-360,true,l,r);assert(e.phase==1);
 e.step(0,0,0,70,7000,-180,true,l,r);assert(e.phase==1);
 e.step(0,0,0,70,10000,0,true,l,r);assert(e.phase==2);
 e.step(0,0,0,70,10001,0,true,l,r);assert(e.phase==3); // zero return angle
 e.reset();e.step(0,0,0,70,0,0,false,l,r);e.step(0,0,0,70,1440,0,false,l,r);assert(e.phase==0);
 e.step(0,0,0,70,4320,0,false,l,r);assert(e.phase==1);
 puts("PASS threshold boundary 169/170, both traffic modes, cached small red, green release protection, zero threshold, 360-degree gyro/timed right scan");
}
