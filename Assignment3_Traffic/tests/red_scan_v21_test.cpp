#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/TrafficLogic.h"
int main(){
 TrafficLogic t;t.stopOnRed=true;t.cruiseSpeed=50;assert(t.cruisePWM()==50&&t.slowPWM()==50);t.cruiseSpeed=150;assert(t.cruisePWM()==150&&t.slowPWM()==60);
 t.start(0);assert(t.moving());t.colour(1,true,true,false,-1,100,false);assert(t.state==t.VERIFY&&!t.moving());auto id=t.request;
 t.colour(2,true,false,false,-1,1000,false);t.tick(3000);assert(t.state==t.VERIFY&&!t.moving()); // no runaway on lost red/stale frame
 t.inference(id,false,false,3100);assert(!t.moving());t.inference(id,true,true,3200,200);assert(!t.moving());t.inference(id,true,true,3300,200);assert(t.state==t.CROSS&&t.moving()); // independent of range
 t.stop(3400);t.start(3500);t.colour(3,true,true,false,-1,3600,false);id=t.request;t.inference(id,true,false,3700);t.inference(id,true,false,3800);assert(t.state==t.WAIT_RED&&!t.moving());t.tick(100000);assert(!t.moving());
 for(int i=4;i<=6;i++)t.colour(i,true,false,true,-1,100000+i*100,false);assert(t.state==t.TRACK&&t.moving());
 t.colour(7,true,true,false,100,101000,true);t.tick(121001);assert(t.state==t.WAIT_RED&&!t.moving());
 t.stop(121010);t.stopOnRed=false;t.start(121020);t.colour(8,true,true,false,-1,121030,false);assert(t.moving());id=t.request;t.inference(id,true,false,121040);t.inference(id,true,false,121050);assert(t.state==t.APPROACH&&t.moving());
 puts("PASS V21 red-stop latch, no-calibration detection, stale/lost-red hold, two-positive release even at distance, plain-red wait, green release, timeout hold, disabled legacy behaviour, speed limits.");
}
