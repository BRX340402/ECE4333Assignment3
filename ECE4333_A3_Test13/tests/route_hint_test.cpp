// Replays the sensor pattern from Test10 real run 4 (corner D, 2026-10-14): left sensor on
// tape, then right (CROSS), then all three, then left again, then the line ends.
// Old rule (latest onset) pivots LEFT; the route hint must pivot RIGHT near the end of
// C-O-D but leave the crossing (mid-straight) to the sensor rule.
//   g++ -std=c++14 -I../UNO_Mapping route_hint_test.cpp -o rh && ./rh
#include <cstdio>
#include "LineFollower.h"
static int fails=0;
static void check(bool ok,const char*what){printf("%s %s\n",ok?"PASS":"FAIL",what);if(!ok)fails++;}
// returns the turn direction chosen (0 = none)
static int apex(bool route,float distAtApex,float yaw=4.5f){
  LineFollower f;f.route=route;f.cruise=50;int l,r;uint32_t t=1000;f.distCm=0;f.start(0,t);
  auto run=[&](bool bl,bool bm,bool br,int ms){for(int i=0;i<ms;i+=10){t+=10;f.step(bl,bm,br,t,yaw,true,-1,l,r);if(f.state==LineFollower::TURN)return true;}return false;};
  f.distCm=distAtApex-10;run(0,1,0,3000);            // following, centre only (narrow class)
  f.distCm=distAtApex;
  if(run(1,1,0,300))return f.turnDir;                   // left sensor rides the tape
  if(run(0,1,1,100))return f.turnDir;                   // right sees tape -> CROSS
  if(run(1,1,1,100))return f.turnDir;                   // apex blob, all three
  if(run(1,1,0,100))return f.turnDir;                   // left onset again (latest)
  if(run(0,0,0,200))return f.turnDir;                   // line ends
  return 0;
}
int main(){
  check(apex(false,305)==1,"old rule at corner D: pivots LEFT (the real run-4 failure)");
  check(apex(true,305)==-1,"route hint at corner D (305 cm): pivots RIGHT");
  check(apex(true,150)==1,"same pattern at the crossing (150 cm): hint off, sensor rule");
  // segment bookkeeping: accept a line at -125 deg after the corner -> D-E
  LineFollower f;int l,r;uint32_t t=1000;f.start(0,t);f.distCm=310;
  f.enterTurn(-1,t);for(int i=0;i<10;i++){t+=10;f.step(false,true,false,t,-125,true,-1,l,r);}
  check(f.seg==1&&f.state==LineFollower::FOLLOW,"line accepted at -125 deg -> segment D-E");
  check(f.routeSide()==0,"just after the corner: hint off (start of D-E)");
  f.distCm=310+120;check(f.routeSide()==-1,"end of D-E: next corner E is a RIGHT turn");
  printf(fails?"ROUTE HINT TEST FAILED\n":"ALL ROUTE HINT CHECKS PASSED\n");return fails?1:0;
}
