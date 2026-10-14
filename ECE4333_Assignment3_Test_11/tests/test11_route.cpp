#include <cassert>
#include <cstdlib>
#include <cstdio>
#include "../UNO_Assignment2/Ex3Tracking.h"
void confirm(Ex3Tracking &e,float yaw,unsigned t){int l,r;e.step(0,0,0,60,t,e.routeBase,true,l,r);e.step(0,700,0,60,t+300,yaw,true,l,r);e.step(0,700,0,60,t+360,yaw,true,l,r);e.step(0,700,0,60,t+580,yaw,true,l,r);assert(!e.searching&&!e.failed);}
int main(){Ex3Tracking e;int l,r;assert(e.routeMode==1);e.beginRoute();
// Crossings are straight; a brief loss/recovery on the same heading is not a corner.
e.step(700,700,700,60,0,0,true,l,r);assert(l==60&&r==60&&e.routeIndex==0);
confirm(e,0,100);assert(e.routeIndex==0);
// Wrong-direction line cannot advance route, and the absolute bound wins over line detection.
e.step(0,0,0,60,800,0,true,l,r);e.step(0,700,0,60,1100,80,true,l,r);assert(!e.candidate&&e.routeIndex==0);
e.step(0,700,0,60,1200,171,true,l,r);assert(e.failed&&l==0&&r==0);e.reset();
// All five turns: two right then three left; 120-degree exit is permitted.
float headings[]={-150,-240,-210,-120,0};for(unsigned i=0;i<5;i++){confirm(e,headings[i],2000+i*1000);assert(e.routeIndex==(i+1)%5);}
assert(e.laps==1);e.beginRoute();assert(e.laps==0&&e.routeIndex==0&&e.routeBase==0);
// Missing line exhausts finite mapped search, rather than spinning indefinitely.
float yaw=0;for(unsigned t=0;t<22000&&!e.failed;t+=20){e.step(0,0,0,60,t,yaw,true,l,r);yaw+=(r-l)*0.008f;}assert(e.failed&&l==0&&r==0);
puts("PASS ordered map turns, crossing straight, no false local progress, wrong-exit rejection, 120-degree turn, bounded missing-line search");}
