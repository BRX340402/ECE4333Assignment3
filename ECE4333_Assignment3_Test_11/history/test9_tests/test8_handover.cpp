#include <cassert>
#include <cstdio>
#include "../UNO_Assignment2/Ex3Tracking.h"
int main(){Ex3Tracking e;e.rightDegrees=120;int l,r;
// A left corner touching the centre must still steer LEFT.
e.step(700,700,100,60,0,0,false,l,r);assert(l<r&&!e.searching);
e.step(100,100,100,60,100,0,false,l,r);assert(l<0&&r>0&&!e.searching);
e.step(100,100,100,60,181,0,false,l,r);assert(e.searching&&l>0&&r<0);
// Reach the leftward phase, detect line, then lose it in the handover.
e.reset();e.step(100,100,100,70,0,0,false,l,r);
e.step(100,100,100,70,1440,0,false,l,r);assert(e.phase==1);
e.step(100,700,100,70,1500,0,false,l,r);
e.step(100,700,100,70,1560,0,false,l,r);assert(e.settling&&e.searching);
e.step(700,700,100,70,1570,0,false,l,r);assert(l<r&&r==45);
e.step(100,100,100,70,1580,0,false,l,r);assert(e.searching&&e.phase==1&&!e.settling&&l<0&&r>0);
// Successful low-speed traversal, rather than a stationary hit, ends recovery.
e.step(100,700,100,70,1600,0,false,l,r);e.step(100,700,100,70,1660,0,false,l,r);
e.step(100,700,100,70,1800,0,false,l,r);assert(e.searching&&l==45&&r==45);
e.step(100,700,100,70,1880,0,false,l,r);assert(!e.searching&&l==45&&r==45);
// Both side sensors with a white centre are NOT a centred line.
e.reset();e.step(100,100,100,60,0,0,false,l,r);
e.step(700,100,700,60,100,0,false,l,r);e.step(700,100,700,60,500,0,false,l,r);assert(e.searching&&!e.settling);
puts("PASS left-corner steering, local loss correction, low-speed handover, preserved left sweep, false centre rejection");}
