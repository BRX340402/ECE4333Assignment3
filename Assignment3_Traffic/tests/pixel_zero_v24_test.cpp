#include <cassert>
#include <cstdio>
#include "../ESP32_Assignment2/RedAction.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/CrossingControl.h"
int main(){
 assert(!redActionTriggered(true,499,500));assert(redActionTriggered(true,500,500));assert(redActionTriggered(true,600,500));assert(!redActionTriggered(false,0,0));assert(redActionTriggered(true,100,0));
 Ex3Tracking e;int l=100,r=100;e.step(100,100,100,0,0,0,false,l,r);assert(!l&&!r&&!e.searching);e.step(100,100,100,0,60000,0,false,l,r);assert(!e.failed&&!l&&!r);
 e.step(700,100,100,1,60001,0,false,l,r);assert(l==-1&&r==1);
 TrafficLogic t;t.cruiseSpeed=80;t.slowSpeed=0;assert(t.slowPWM()==0&&t.crossingPWM()==80);CrossingControl c;c.arm(0);c.step(0,false,false,true,false,0,0,1500,l,r,t.crossingPWM());assert(l==80&&r==80);
 puts("PASS V24 pixel threshold boundaries, zero means no extra gate, zero-speed no movement/search timeout/division, low PWM respected, STOP crossing resumes at cruise when reduced speed is zero.");
}
