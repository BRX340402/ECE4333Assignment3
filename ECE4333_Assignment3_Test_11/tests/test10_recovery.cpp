#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/Ex3Tracking.h"
int main(){Ex3Tracking e;e.routeMode=0;int l,r;
e.step(700,700,100,60,0,0,true,l,r);assert(l==60&&r==60&&!e.searching);
e.step(700,100,100,60,10,0,true,l,r);assert(l==0&&r==0&&e.searching);
e.step(700,100,100,60,40,0,true,l,r);assert(l==30&&r==60);
e.step(0,0,0,60,270,0,true,l,r);assert(l==-100&&r==100&&e.phase==1);
e.step(0,0,0,60,350,0,true,l,r);assert(l==0&&r==0); // pulse rest
// Bound violations are checked before accepting a newly seen line.
e.step(0,700,0,60,360,111,true,l,r);assert(e.failed&&e.failure==Ex3Tracking::ANGLE_LIMIT&&!l&&!r);
e.reset();e.step(0,0,0,60,0,0,true,l,r);e.step(0,0,0,60,300,0,true,l,r);
e.step(0,0,0,60,2801,0,true,l,r);assert(e.failed&&e.failure==Ex3Tracking::STALLED&&!l&&!r);
e.reset();e.step(0,700,0,60,0,0,false,l,r);assert(e.failed&&e.failure==Ex3Tracking::GYRO_LOST);
e.reset();e.step(0,0,0,60,0,0,true,l,r);e.step(0,700,0,60,100,-10,true,l,r);e.step(0,700,0,60,160,-10,true,l,r);assert(e.settling&&!l&&!r);
e.step(0,700,0,60,200,-10,true,l,r);assert(l==45&&r==45&&e.searching);
e.step(0,700,0,60,380,-10,true,l,r);assert(!e.searching);
// Synthetic yaw integration checks staged motion stays bounded and terminates.
e.reset();float yaw=0;for(unsigned t=0;t<=21000&&!e.failed;t+=20){e.step(0,0,0,60,t,yaw,true,l,r);assert(l>=-100&&r<=100);yaw+=(r-l)*0.01f;assert(yaw<=110&&yaw>=-110);}assert(e.failed&&!l&&!r);
e.reset();e.step(0,0,0,0,0,0,true,l,r);assert(!l&&!r&&!e.searching);
e.rightDegrees=e.sweepDegrees=360;e.step(0,0,0,60,0,0,true,l,r);e.step(0,0,0,60,100,112,true,l,r);assert(e.failed); // internal 110 hard cap
puts("PASS gyro required, gentle local correction, PWM100 pulse/rest, hard continuous-yaw bounds, stall timeout, low-speed reacquisition, zero speed");}
