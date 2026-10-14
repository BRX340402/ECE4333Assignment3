#include <cassert>
#include <iostream>
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
#include "../UNO_Assignment2/CrossingControl.h"
int main(){
 TrafficLogic t;t.start(0);t.tick(5000);assert(t.state==t.TRACK&&t.moving());
 t.colour(1,true,true,false,10,5100,false);assert(t.state==t.VERIFY&&t.moving());
 t.inference(t.request,false,false,5200);assert(t.moving());
 t.inference(t.request,true,false,5300);t.inference(t.request,true,false,5400);assert(t.state==t.APPROACH&&t.moving());
 t.colour(2,false,false,false,-1,5500);t.tick(7000);assert(t.state==t.TRACK&&t.moving());
 t.colour(3,true,true,false,50,7100,true);assert(t.moving());
 t.inference(t.request,true,false,7200);t.inference(t.request,true,false,7300);assert(t.state==t.APPROACH&&t.moving());
 t.colour(4,true,true,false,24,7400,true);assert(t.state==t.VERIFY&&!t.moving());
 t.inference(t.request,true,false,7500);t.inference(t.request,true,false,7600);assert(t.state==t.WAIT_RED&&!t.moving());
 t.tick(20000);assert(t.state==t.WAIT_RED);t.start(20000);assert(t.state==t.WAIT_RED);
 t.colour(5,true,false,true,-1,20100,false);t.colour(6,true,false,true,-1,20200,false);assert(t.state==t.WAIT_RED);
 t.colour(6,true,false,true,-1,20250,false);assert(t.state==t.WAIT_RED); // duplicate is not another green frame
 t.colour(7,true,false,true,-1,20300,false);assert(t.state==t.TRACK&&t.moving());
 t.colour(8,true,true,false,-1,20400,false);auto id=t.request;
 t.inference(id-1,true,true,20500,10);assert(t.positives==0);
 t.inference(id,true,true,20500,50);t.inference(id,true,true,20600,50);assert(t.state==t.VERIFY&&t.moving());
 t.inference(id,true,true,20700,25);t.inference(id,true,true,20800,25);assert(t.state==t.STOP_HOLD&&!t.moving());
 t.start(21000);assert(t.at==20800);t.tick(23799);assert(t.state==t.STOP_HOLD);t.tick(23800);assert(t.state==t.CROSS&&t.moving());
 t.crossReady=true;t.tick(25299);assert(t.state==t.CROSS);t.tick(25300);assert(t.state==t.TRACK);
 // Camera/AI unavailable cannot fault a moving car.
 TrafficLogic q;q.start(0);q.colour(1,true,true,false,-1,100,false);
 for(int i=2;i<=110;i++){q.colour(i,true,true,false,-1,i*200,false);q.inference(q.request,false,false,i*200);q.tick(i*200);assert(q.moving()&&q.state!=q.FAULT);}
 Ex3Tracking e;int l,r;
 // Historical tests below exercise the old 90/180 preset explicitly.
 e.rightDegrees=90;e.sweepDegrees=180;
 e.step(252,224,215,100,0,0,false,l,r);assert(e.searching&&e.timedSearch&&!e.failed&&l==70&&r==-70);
 e.step(252,224,215,100,1079,0,false,l,r);assert(e.phase==0&&!e.failed);
 e.step(252,224,215,100,1080,0,false,l,r);assert(e.phase==1&&!e.failed);
 e.step(252,224,215,100,1081,0,false,l,r);assert(l==-70&&r==70);
 e.step(252,224,215,100,3239,0,false,l,r);assert(!e.failed);
 e.step(252,224,215,100,3240,0,false,l,r);assert(e.failed&&e.failure==e.EXHAUSTED&&!l&&!r);
 e.reset();e.step(100,100,100,60,0,0,true,l,r);e.step(100,100,100,60,100,-89,true,l,r);assert(e.phase==0);
 e.step(100,100,100,60,120,-90,true,l,r);assert(e.phase==1);
 e.step(100,100,100,60,150,0,true,l,r);assert(e.phase==1&&l<0&&r>0);
 e.step(100,100,100,60,200,89,true,l,r);assert(!e.failed);e.step(100,100,100,60,220,90,true,l,r);assert(e.failed&&!l&&!r);
 e.reset();e.step(100,100,100,100,0,0,true,l,r);e.step(100,100,100,100,1080,0,true,l,r);assert(e.phase==1); // stuck gyro still reverses
 // Left-only track recovery must reverse a right scan immediately, then leave search.
 e.reset();e.step(100,100,100,100,0,0,true,l,r);
 for(int i=0;i<3;i++){e.step(700,100,100,100,20+i*20,-10,true,l,r);assert(l<0&&r>0);}assert(!e.searching);
 e.reset();e.step(700,100,100,100,0,0,true,l,r);assert(!e.searching&&l==-70&&r==70);
 // Legacy broad settings are bounded even before a settings update.
 e.reset();e.rightDegrees=150;e.sweepDegrees=300;e.step(100,100,100,60,0,0,true,l,r);
 e.step(100,100,100,60,100,-120,true,l,r);assert(e.phase==1);e.step(100,100,100,60,200,120,true,l,r);assert(e.failed);
 e.rightDegrees=90;e.sweepDegrees=180;
 e.reset();e.step(100,100,100,60,0,0,true,l,r);e.step(100,100,100,60,100,16,true,l,r);assert(e.timedSearch&&!e.failed);
 for(int i=0;i<3;i++)e.step(100,700,100,60,200+i*20,0,false,l,r);assert(!e.searching&&!e.failed&&l==60&&r==60);
 e.reset();e.step(100,100,100,60,0,0,true,l,r);e.step(100,100,100,60,30000,0,true,l,r);assert(e.failed&&e.failure==e.TIMED_OUT);
 e.reset();e.step(1000,1000,1000,60,0,0,false,l,r);assert(!e.failed&&!l&&!r);e.step(1000,1000,1000,60,199,0,false,l,r);assert(!e.failed);e.step(1000,1000,1000,60,200,0,false,l,r);assert(e.failed&&e.failure==e.LIFTED);
 CrossingControl c;c.arm(0);assert(c.step(0,false,false,true,false,1600,1600,1500,l,r));assert(l==60&&r==60);assert(c.cleared(1800));
 std::cout<<"PASS V15: uncalibrated motion; calibrated red distance stop and three-green release; stale/invalid vision tolerance; STOP range/hold/restart protection; V18 bounded right90/reverse-left180 searches and side-line recovery, reacquisition, timeout, lift debounce; crossing without gyro.\n";
}
