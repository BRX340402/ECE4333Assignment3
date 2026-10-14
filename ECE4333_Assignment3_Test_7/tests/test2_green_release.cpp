#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
int main(){
  TrafficLogic t; t.stopOnRed=true;
  t.start(100);assert(t.state==TrafficLogic::TRACK);
  t.colour(1,true,false,true,-1,200);assert(t.state==TrafficLogic::TRACK && t.greens==0);
  t.colour(2,true,true,true,-1,300);assert(t.state==TrafficLogic::VERIFY && t.greens==0);
  t.inference(t.request,true,false,350);t.inference(t.request,true,false,400);
  assert(t.state==TrafficLogic::WAIT_RED);
  t.colour(3,true,false,true,-1,600);assert(t.greens==1);
  t.colour(4,false,false,false,-1,1400);assert(t.greens==1);
  t.colour(5,true,false,false,-1,1700);assert(t.greens==1);
  t.colour(6,true,false,true,-1,2600);assert(t.state==TrafficLogic::TRACK && t.greens==0);
  // Red priority: a red paper may also create a false green region.
  t.stop(3000);t.start(3100);t.colour(7,true,true,true,-1,3200);
  t.inference(t.request,true,false,3250);t.inference(t.request,true,false,3300);
  assert(t.state==TrafficLogic::WAIT_RED);
  t.colour(8,true,false,true,-1,3400);assert(t.greens==1);
  t.colour(9,true,true,true,-1,3500);assert(t.greens==0);
  t.colour(10,true,false,true,-1,4000);assert(t.greens==1);
  t.colour(11,true,false,true,-1,4400);assert(t.state==TrafficLogic::TRACK);
  // Timeout: two observations separated by > 6 sec don't qualify.
  t.stop(5000);t.start(5100);t.colour(12,true,true,false,-1,5200);
  t.inference(t.request,true,false,5250);t.inference(t.request,true,false,5300);
  t.colour(13,true,false,true,-1,5400);assert(t.greens==1);
  t.colour(14,false,false,false,-1,12500);assert(t.greens==0);
  t.colour(15,true,false,true,-1,12600);assert(t.state==TrafficLogic::WAIT_RED && t.greens==1);
  t.colour(16,true,false,true,-1,12900);assert(t.state==TrafficLogic::TRACK);
  // Confirmed STOP sign still holds 3 seconds independently of green release.
  t.stop(14000);t.start(14100);t.colour(17,true,true,false,-1,14200);
  t.inference(t.request,true,true,14300);t.inference(t.request,true,true,14400);
  assert(t.state==TrafficLogic::STOP_HOLD && !t.moving());
  t.colour(18,true,false,true,-1,14500);t.colour(19,true,false,true,-1,15000);
  assert(t.state==TrafficLogic::STOP_HOLD);
  t.tick(17400);assert(t.state==TrafficLogic::CROSS);
  // Requested search limits; traffic decisions above remain unchanged.
  Ex3Tracking e;assert(e.rightDegrees==120 && e.sweepDegrees==360);
  puts("PASS Test 2: green confirmations, dropouts, timeout, red priority, STOP hold, 120/360 search configuration");
}
