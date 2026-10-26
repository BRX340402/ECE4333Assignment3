// Host-only smoke tests for the independent A4 Test2 logic.
// They do not validate physical obstacle geometry, ultrasound, or motor torque.
#include <cassert>
#include <cstdio>
#include "../UNO_Test2/A4Bug2.h"

static void tick(A4Bug2 &v,uint32_t t,float yaw,float odom,int cm,uint32_t stamp,
                 bool gyro=true,uint8_t seg=1,float progress=20) {
 int L=0,R=0;
 bool owns=v.step(t,yaw,0,odom,gyro,cm,stamp,
     true,true,0.0f,seg,progress,false,true,false,L,R);
 if(v.active())assert(owns);
}

int main(){
 assert(A4Bug2::outsideOfRoute(1)==1);
 assert(A4Bug2::outsideOfRoute(0)==1);
 assert(A4Bug2::nearIntersection(0,150));
 assert(!A4Bug2::nearIntersection(1,20));
 A4Bug2 v;
 assert(v.state==A4Bug2::TRACK);
 tick(v,100,0,0,18,100);assert(v.state==A4Bug2::TRACK);
 tick(v,200,0,0,18,200);assert(v.state==A4Bug2::TRACK);
 tick(v,300,0,0,18,300);assert(v.state==A4Bug2::TURN_OUT);
 assert(v.desiredPan()==90);
 tick(v,310,90,0,-2,300);assert(v.state==A4Bug2::SIDE_SCAN);
 assert(v.desiredPan()==150); // left detour => look right toward obstacle
 tick(v,320,90,0,-2,300);assert(v.state==A4Bug2::SIDE_SCAN);
 // Losing a working IMU while detouring must cause a stopped fault.
 tick(v,330,90,0,-2,300,false);assert(v.state==A4Bug2::FAULT);
 v.reset();assert(v.state==A4Bug2::TRACK);
 A4Bug2 w;
 tick(w,100,0,0,15,100,true,0,140);
 tick(w,200,0,0,15,200,true,0,140);
 tick(w,300,0,0,15,300,true,0,140);
 assert(w.state==A4Bug2::WAIT);
 tick(w,9500,0,0,15,9500,true,0,140);
 assert(w.state==A4Bug2::FAULT); // never force a pass by WAIT expiry
 std::puts("A4 Test2 smoke tests passed.");
 return 0;
}
