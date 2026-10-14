#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
using T=TrafficLogic;
T running(){T t;t.colour(1,true,false,false,-1,100);t.start(100);assert(t.state==T::TRACK);return t;}
int main(){
 {auto t=running();t.tick(1600);assert(t.fault==T::COLOUR_STALE&&t.faultState==T::TRACK);t.fail(T::AI_TIMEOUT,1700);assert(t.fault==T::COLOUR_STALE);t.stop(1800);assert(t.fault==T::NONE);}
 {auto t=running();t.colour(2,false,false,false,-1,200);assert(t.fault==T::COLOUR_INVALID);}
 {auto t=running();t.colour(2,true,true,false,100,200);t.colour(3,true,false,false,-1,901);assert(t.fault==T::RED_LOST&&t.faultState==T::APPROACH);}
 {auto t=running();t.enter(T::VERIFY,200);t.inference(t.request,false,false,300);assert(t.fault==T::AI_INVALID&&t.faultState==T::VERIFY);}
 {auto t=running();t.enter(T::VERIFY,200);t.lastColour=20201;t.tick(20201);assert(t.fault==T::AI_TIMEOUT);}
 {auto t=running();t.enter(T::CROSS,200);t.lastColour=6701;t.tick(6701);assert(t.fault==T::CROSS_TIMEOUT&&t.faultState==T::CROSS);}
 int l=0,r=0;Ex3Tracking x;
 x.step(1000,1000,1000,100,100,0,true,l,r);assert(x.failure==Ex3Tracking::LIFTED);
 x.reset();x.step(0,0,0,100,100,0,false,l,r);assert(x.failure==Ex3Tracking::GYRO_LOST);
 x.reset();x.step(0,0,0,100,100,0,true,l,r);x.step(0,0,0,100,12200,0,true,l,r);assert(x.failure==Ex3Tracking::TIMED_OUT);
 x.reset();x.step(0,0,0,100,100,0,true,l,r);x.step(0,0,0,100,200,16,true,l,r);assert(x.failure==Ex3Tracking::WRONG_YAW);
 x.reset();x.step(0,0,0,100,100,0,true,l,r);x.step(0,0,0,100,200,-90,true,l,r);x.step(0,0,0,100,300,90,true,l,r);assert(x.failure==Ex3Tracking::EXHAUSTED);
 puts("PASS V14: actual fault codes, originating state, first-cause retention and reset; line lift, gyro loss, search timeout, wrong yaw and exhausted sweep.");
}
