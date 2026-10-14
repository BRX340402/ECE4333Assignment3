#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/Ex3Tracking.h"
int main(){Ex3Tracking e;int l,r;
// Dark saturation no longer lies outside the default range.
e.step(100,999,100,60,0,0,true,l,r);assert(l==60&&r==60&&!e.searching);
// High contrast polarity may be reversed by calibration.
e.blackLow=0;e.blackHigh=400;e.step(900,100,900,60,20,0,true,l,r);assert(l==60&&r==60);e.blackLow=600;e.blackHigh=1023;
// Actual measured angle, not nominal time, completes phases.
e.reset();e.step(100,100,100,70,0,0,true,l,r);
e.step(100,100,100,70,1440,-50,true,l,r);assert(e.phase==0&&!e.failed);
e.step(100,100,100,70,2500,-120,true,l,r);assert(e.phase==1);
e.step(100,100,100,70,6800,120,true,l,r);assert(e.phase==1);
e.step(100,100,100,70,8500,240,true,l,r);assert(e.phase==2);
e.step(100,100,100,70,11000,0,true,l,r);assert(e.phase==3);
// Confirmation requires actual elapsed time; reject opposite heading.
e.reset();e.step(100,100,100,70,0,0,true,l,r);
e.step(100,700,100,70,20,-180,true,l,r);assert(e.searching);
e.step(100,700,100,70,80,-180,true,l,r);assert(e.searching);
e.step(100,700,100,70,100,-90,true,l,r);assert(e.searching&&!l&&!r);
e.step(100,700,100,70,159,-90,true,l,r);assert(e.searching);
e.step(100,700,100,70,160,-90,true,l,r);assert(!e.searching&&l==70&&r==70);
// Broad black patches cannot end recovery.
e.reset();e.step(0,0,0,60,0,0,true,l,r);e.step(900,900,900,60,100,0,true,l,r);assert(e.searching);
// Normal wide patch follows entry heading only for a bounded interval.
e.reset();e.step(0,700,0,60,0,0,true,l,r);e.step(900,900,900,60,20,5,true,l,r);assert(l>r&&!e.searching);
e.step(900,900,900,60,620,5,true,l,r);assert(e.searching);
// Wrong gyro sign fails explicitly rather than silently changing scan geometry.
e.reset();e.step(0,0,0,60,0,0,true,l,r);e.step(0,0,0,60,500,20,true,l,r);assert(e.failed&&e.failure==Ex3Tracking::WRONG_YAW);
// Timed fallback and unknown range never cause an unverified forward nudge.
e.reset();e.step(0,0,0,70,0,0,false,l,r);e.step(0,0,0,70,1440,0,false,l,r);assert(e.phase==1);
e.step(0,0,0,70,5760,0,false,l,r);assert(e.phase==2);e.step(0,0,0,70,8640,0,false,l,r);assert(e.phase==3);
e.step(0,0,0,70,8650,0,false,l,r);assert(e.phase==0&&!l&&!r);
puts("PASS search: saturation, inverted threshold, full measured angles, direction gate, 60ms confirmation, wide patches, gyro sign, nudge gate");}
