#include <cassert>
#include <cstdio>
#include "../ESP32_Assignment2/RedDetector.h"
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
int main(){
 assert(greenPixel(180,230,180));assert(greenPixel(10,40,15));
 assert(greenPixel(120,200,45));assert(greenPixel(35,130,100));
 assert(!greenPixel(200,200,200));assert(!greenPixel(255,255,255));
 assert(!greenPixel(1,8,1));assert(!greenPixel(200,200,0));
 assert(!greenPixel(0,200,200));assert(!greenPixel(220,20,20));
 assert(redPixel(220,20,20));
 TrafficLogic t;t.stopOnRed=true;t.start(0);t.colour(1,true,true,false,-1,10);
 assert(t.state==t.VERIFY);t.colour(2,true,false,true,-1,20);
 t.colour(2,true,false,true,-1,21);assert(t.greens==1);
 t.colour(3,true,false,true,-1,30);assert(!t.moving());
 t.colour(4,true,false,true,-1,40);assert(t.state==t.TRACK&&t.moving());
 Ex3Tracking e;e.sweepDegrees=360;int l,r;
 e.step(100,100,100,60,0,0,true,l,r);assert(l>0&&r<0);
 e.step(100,100,100,60,100,-90,true,l,r);assert(e.phase==1);
 e.step(100,100,100,60,200,90,true,l,r);assert(!e.failed&&l<0&&r>0);
 e.step(100,100,100,60,300,269,true,l,r);assert(!e.failed);
 e.step(100,100,100,60,400,270,true,l,r);assert(e.failed&&e.failure==e.EXHAUSTED);
 e.reset();e.step(100,100,100,30,0,0,false,l,r);
 e.step(100,100,100,30,2520,0,false,l,r);assert(e.phase==1);
 e.step(100,100,100,30,7560,0,false,l,r);assert(!e.failed&&l<0&&r>0);
 for(int i=0;i<3;i++)e.step(700,100,100,30,7600+i,0,false,l,r);
 assert(!e.searching&&!e.failed&&l<0&&r>0);
 puts("PASS V27 pale/dark green, neutral/yellow/cyan rejection, three-frame green release during VERIFY, full360 left yaw and timed search/reacquisition.");
}
