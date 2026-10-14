#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
#include "../ESP32_Assignment2/RedDetector.h"
int main(){
  // Traffic workflow: green from OFF / TRACK is not a start instruction.
  TrafficLogic t; t.stopOnRed=true;t.cruiseSpeed=50;t.slowSpeed=30;
  t.colour(1,true,false,true,-1,100); assert(t.state==TrafficLogic::OFF);
  t.start(200);assert(t.state==TrafficLogic::TRACK);
  t.colour(2,true,false,true,-1,300);assert(t.state==TrafficLogic::TRACK);
  // Red -> immediate stationary VERIFY -> two AI positives -> three-second STOP_HOLD
  t.colour(3,true,true,false,-1,500);assert(t.state==TrafficLogic::VERIFY&&!t.moving());
  uint16_t req=t.request;t.inference(req,true,true,700);assert(t.state==TrafficLogic::VERIFY);
  t.inference(req,true,true,800);assert(t.state==TrafficLogic::STOP_HOLD&&!t.moving());
  t.colour(4,true,false,true,-1,900);t.colour(5,true,false,true,-1,1000);t.colour(6,true,false,true,-1,1100);
  assert(t.state==TrafficLogic::STOP_HOLD);t.tick(3799);assert(t.state==TrafficLogic::STOP_HOLD);
  t.tick(3800);assert(t.state==TrafficLogic::CROSS&&t.moving());
  // The same STOP sign still in view after the crossing must not immediately rearm.
  t.crossReady=true;t.tick(5400);assert(t.state==TrafficLogic::TRACK);
  t.colour(7,true,true,false,-1,5450);assert(t.state==TrafficLogic::TRACK);
  t.colour(8,true,false,false,-1,5600);t.colour(9,true,false,false,-1,5750);t.colour(10,true,false,false,-1,5900);
  t.colour(11,true,true,false,-1,6100);assert(t.state==TrafficLogic::VERIFY);
  // Red-only traffic light -> 2 negative AI -> wait until two green-only observations
  req=t.request;t.inference(req,true,false,6250);t.inference(req,true,false,6350);
  assert(t.state==TrafficLogic::WAIT_RED&&!t.moving());
  t.colour(12,true,true,true,-1,6400);assert(t.greens==0&&t.state==TrafficLogic::WAIT_RED);
  t.colour(13,true,false,true,-1,6600);t.colour(14,true,false,true,-1,6800);
  assert(t.state==TrafficLogic::TRACK&&t.moving());t.colour(15,true,false,true,-1,7000);
  assert(t.state==TrafficLogic::TRACK&&t.moving());
  // Green pixel classification: dark green yes; black, gray, cyan, blue no.
  assert(greenPixel(10,55,18));assert(greenPixel(50,100,49));
  assert(!greenPixel(10,10,10));assert(!greenPixel(40,45,40));
  assert(!greenPixel(55,55,55));assert(!greenPixel(10,55,55));
  assert(!greenPixel(80,20,20));

 puts("PASS unchanged red/STOP/green decisions");
}
