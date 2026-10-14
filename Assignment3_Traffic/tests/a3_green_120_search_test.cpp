#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
int main(){
 // Stop-on-red: red stops immediately. 3 green-only unique frames can release
 // straight from VERIFY without waiting 20 seconds.
 TrafficLogic t;t.stopOnRed=true;t.start(0);t.colour(1,true,true,false,-1,100);
 assert(t.state==TrafficLogic::VERIFY&&!t.moving());
 t.colour(2,true,false,true,-1,200);assert(!t.moving()&&t.greens==1);
 t.colour(2,true,false,true,-1,250);assert(t.greens==1); // duplicate
 t.colour(3,true,false,true,-1,300);assert(!t.moving());
 t.colour(4,true,false,true,-1,400);assert(t.state==TrafficLogic::TRACK&&t.moving());
 // Red/green simultaneously should not release an active stop.
 t.stop(500);t.start(600);t.colour(5,true,true,false,-1,700);
 for(int i=6;i<12;i++)t.colour(i,true,true,true,-1,700+i*100);
 assert(t.state==TrafficLogic::VERIFY&&!t.moving());
 // Existing WAIT_RED behavior remains, old mode remains unaffected.
 t.inference(t.request,true,false,2000);t.inference(t.request,true,false,2100);
 assert(t.state==TrafficLogic::WAIT_RED);
 t.colour(12,true,false,true,-1,2200);t.colour(13,true,false,true,-1,2300);
 assert(!t.moving());t.colour(14,true,false,true,-1,2400);assert(t.moving());
 // STOP AI positives still release to CROSS with the same two-frame logic.
 t.stop(3000);t.start(3001);t.colour(15,true,true,false,-1,3100);
 auto request=t.request;t.inference(request,true,true,3200);assert(!t.moving());
 t.inference(request,true,true,3300);assert(t.state==TrafficLogic::CROSS);
 // Old/legacy opt-out behavior doesn't auto-release VERIFY on green.
 TrafficLogic legacy;legacy.stopOnRed=false;legacy.start(0);
 legacy.colour(1,true,true,false,20,100,true);assert(legacy.state==TrafficLogic::VERIFY);
 legacy.colour(2,true,false,true,-1,200);legacy.colour(3,true,false,true,-1,300);legacy.colour(4,true,false,true,-1,400);
 assert(legacy.state==TrafficLogic::VERIFY);
 // New right120, reverse-left240 search using measured gyro yaw.
 Ex3Tracking e;assert(e.rightDegrees==120&&e.sweepDegrees==240);
 int l=0,r=0;e.step(100,100,100,70,0,0,true,l,r);
 assert(e.searching&&l>0&&r<0);
 e.step(100,100,100,70,100,-119,true,l,r);assert(e.phase==0);
 e.step(100,100,100,70,200,-120,true,l,r);assert(e.phase==1);
 e.step(100,100,100,70,300,-10,true,l,r);assert(e.phase==1&&l<0&&r>0);
 e.step(100,100,100,70,400,120,true,l,r);assert(e.failed&&e.failure==Ex3Tracking::EXHAUSTED&&!l&&!r);
 // Timed fallback: 120*12ms right, then 240*12ms left, at PWM70.
 e.reset();e.step(100,100,100,70,0,0,false,l,r);
 e.step(100,100,100,70,1439,0,false,l,r);assert(e.phase==0);
 e.step(100,100,100,70,1440,0,false,l,r);assert(e.phase==1);
 e.step(100,100,100,70,4320,0,false,l,r);assert(e.failed&&!l&&!r);
 // Reacquire a visible line before the full sweep and never keep scanning.
 e.reset();e.step(100,100,100,70,0,0,true,l,r);
 e.step(100,700,100,70,50,-20,true,l,r);
 e.step(100,700,100,70,70,-21,true,l,r);
 e.step(100,700,100,70,90,-22,true,l,r);
 assert(!e.searching&&!e.failed&&l==70&&r==70);
 puts("PASS A3-V27 red->green release, red priority, STOP-AI crossing, legacy mode, 120/240 gyro and timed fallback, line reacquisition");
}
