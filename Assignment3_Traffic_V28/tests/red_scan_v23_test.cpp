#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/TrafficLogic.h"
int main(){
 TrafficLogic t;t.stopOnRed=true;t.start(0);t.colour(1,true,true,false,-1,100);auto id=t.request;
 t.inference(id,true,true,200);assert(t.positives==1&&!t.moving());
 t.inference(id,false,false,300);t.inference(id,true,false,400);t.inference(id,true,false,500);assert(t.state==t.WAIT_RED&&t.positives==1);
 t.tick(60000);t.inference(id,true,false,60001);assert(t.positives==1&&!t.moving());
 t.inference(id+1,true,true,60002);assert(!t.moving());t.inference(id,true,true,60003);assert(t.state==t.CROSS&&t.moving());
 t.stop(60004);assert(t.positives==0);t.start(60005);t.colour(2,true,true,false,-1,60006);id=t.request;
 t.inference(id,true,false,60007);t.inference(id,true,false,60008);assert(t.state==t.WAIT_RED&&t.positives==0);
 t.colour(3,true,false,true,-1,60010);t.inference(id,true,false,60011);t.colour(4,true,false,true,-1,60012);t.inference(id,true,false,60013);t.colour(5,true,false,true,-1,60014);assert(t.state==t.TRACK);
 t.cruiseSpeed=80;t.slowSpeed=40;assert(t.slowPWM()==40);t.cruiseSpeed=50;t.slowSpeed=60;assert(t.slowPWM()==50);
 puts("PASS V23: nonconsecutive STOP positives retained across bad frames, negatives and WAIT_RED; long wait keeps scanning eligibility; wrong request ignored; green sequence not reset by AI; speed cap.");
}
