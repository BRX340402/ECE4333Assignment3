#include <cassert>
#include <iostream>
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
#include "../UNO_Assignment2/CrossingControl.h"
int main(){
 TrafficLogic t;uint16_t seq=0;uint32_t now=0;
 auto frame=[&](bool r,bool g,int d){now+=200;t.colour(++seq,true,r,g,d,now);t.tick(now);};
 frame(false,false,-1);t.start(now);assert(t.state==t.TRACK&&t.moving());frame(false,false,-1);assert(t.state==t.TRACK);
 t.stopCm=40;frame(true,false,60);assert(t.state==t.APPROACH);frame(true,false,39);assert(t.state==t.VERIFY&&!t.moving());
 auto request=t.request;t.inference(request-1,true,true,now);assert(t.positives==0);
 t.inference(request,true,true,now);assert(t.state==t.VERIFY);t.inference(request,true,true,now);assert(t.state==t.STOP_HOLD);
 for(int i=0;i<14;i++){frame(true,false,20);}assert(t.state==t.STOP_HOLD);frame(true,false,20);assert(t.state==t.CROSS);
 // No early rearm just because the red sign disappears.
 for(int i=0;i<10;i++){frame(false,false,-1);}assert(t.state==t.CROSS);t.crossReady=true;frame(false,false,-1);assert(t.state==t.TRACK);
 frame(true,false,20);t.inference(t.request,true,false,now);t.inference(t.request,true,false,now);assert(t.state==t.WAIT_RED);
 for(int i=0;i<8;i++){frame(false,false,-1);}assert(t.state==t.WAIT_RED);frame(true,true,20);assert(t.state==t.WAIT_RED);
 frame(false,true,-1);frame(false,true,-1);frame(false,true,-1);assert(t.state==t.TRACK);
 // Repeat two more circuits with both STOP and a red light. No lap-complete state.
 for(int lap=0;lap<2;lap++){
  frame(true,false,20);assert(t.state==t.VERIFY);
  t.inference(t.request,true,true,now);t.inference(t.request,true,true,now);
  auto holdAt=t.at;t.start(now);assert(t.state==t.STOP_HOLD&&t.at==holdAt);
  for(int i=0;i<15;i++)frame(true,false,20);
  assert(t.state==t.CROSS);t.crossReady=true;
  for(int i=0;i<10;i++)frame(false,false,-1);
  assert(t.state==t.TRACK);
  frame(true,false,20);t.inference(t.request,true,false,now);t.inference(t.request,true,false,now);
  t.start(now);assert(t.state==t.WAIT_RED);
  frame(false,true,-1);frame(false,true,-1);assert(t.state==t.WAIT_RED);
  frame(false,true,-1);assert(t.state==t.TRACK);
 }
 t.stop(now);for(int i=0;i<4;i++)frame(false,true,-1);assert(t.state==t.OFF);
 frame(true,false,20);t.start(now);assert(t.state==t.VERIFY); // red already present on Start
 t.stop(now);frame(false,false,-1);t.start(now);
 frame(true,false,-1);assert(t.state==t.VERIFY);t.inference(t.request,false,false,now);assert(t.state==t.FAULT);
 t.stop(now);t.start(now);now+=1600;t.tick(now);assert(t.state==t.FAULT);
 TrafficLogic q;q.colour(1,true,false,true,-1,100);q.start(100);q.colour(2,true,false,true,-1,200);q.colour(2,true,false,true,-1,300);assert(q.greens==1);q.colour(1,true,false,true,-1,400);assert(q.greens==1);
 Ex3Tracking e;int l,r;
 assert(!e.black(450)&&e.black(700)&&!e.black(900));e.blackLow=650;assert(!e.black(625));
 e.step(700,700,700,100,0,0,true,l,r);assert(l==100&&r==100);
 e.step(700,100,700,100,10,0,true,l,r);assert(l==100&&r==100); // symmetric intersection = straight
 e.step(100,100,700,100,20,0,true,l,r);assert(l==100&&r==-100);
 e.step(700,100,100,100,30,0,true,l,r);assert(l==-100&&r==100);
 // Unwrapped yaw deliberately crosses the -180 boundary.
 e.reset();e.step(100,100,100,60,100,-170,true,l,r);assert(l==60&&r==-60);
 e.step(100,100,100,60,200,-259,true,l,r);assert(e.phase==0&&r==-60);
 e.step(100,100,100,60,300,-260,true,l,r);assert(e.phase==1&&!l&&!r);
 e.step(100,100,100,60,400,-100,true,l,r);assert(l==-60&&r==60);
 e.step(100,100,100,60,500,-80,true,l,r);assert(e.failed&&!l&&!r);
 e.reset();e.step(100,100,100,60,100,0,true,l,r);e.step(100,700,100,60,200,-20,true,l,r);e.step(100,700,100,60,220,-22,true,l,r);assert(e.searching);e.step(100,700,100,60,240,-24,true,l,r);assert(!e.searching&&l==60&&r==60);
 e.reset();e.step(100,100,100,60,100,0,false,l,r);assert(e.failed&&!l&&!r);
 e.reset();e.step(100,100,100,60,100,0,true,l,r);e.step(100,100,100,60,200,16,true,l,r);assert(e.failed);
 CrossingControl c;c.arm(170);
 // Even at long elapsed time, a straight approach before the junction is not an exit.
 assert(c.step(170,true,false,true,false,2000,2000,1500,l,r));assert(l==r&&l>0&&!c.cleared(2500));
 for(int i=0;i<3;i++){assert(c.step(170,true,true,true,true,2600+i*20,2600+i*20,1500,l,r));assert(l==r&&l>0);}assert(c.wideSeen);
 c.step(170,true,true,false,false,2700,2700,1500,l,r);assert(l==r); // side-only branch cannot turn car
 c.step(170,true,false,true,false,2800,2800,1500,l,r);assert(!c.cleared(2900));c.step(170,true,false,true,false,3010,3010,1500,l,r);assert(c.cleared(3010));
 c.arm(0);c.step(-5,true,true,false,false,100,100,1500,l,r);assert(l<r&&l>0&&r>0); // left correction, never pivot
 assert(!c.step(45,true,false,true,false,200,200,1500,l,r));assert(!l&&!r);
 assert(!c.step(0,false,false,true,false,200,200,1500,l,r));
 std::cout<<"PASS V4: immediate start without green; repeated circuits; repeated Start cannot bypass stops; editable stop threshold; green/red/STOP sequence; no early crossing release; 90R/180L using unwrapped yaw; reacquisition; wrong gyro sign/loss; narrow black band; symmetric straight priority; straight lock requires wide junction then centre; correction polarity.\n";
}
