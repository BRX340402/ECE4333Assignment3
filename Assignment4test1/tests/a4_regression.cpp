#include <cassert>
#include <iostream>
#include "Overtake.h"
int main(){
 Overtake o;int l=0,r=0;
 assert(o.trigCm==10&&o.parCm==26&&o.joinAdv==15&&o.outCm==12&&o.sideOpt==1);
 o.rg=10;assert(o.need()==91);
 o.state=Overtake::PASS;o.d0=0;o.tgt=0;
 o.step(0,0,50.9f,100,true,true,0,1,300,300,false,false,false,100,l,r);
 assert(o.state==Overtake::PASS);
 o.step(0,0,51.0f,100,true,true,0,1,300,300,false,false,false,200,l,r);
 assert(o.state==Overtake::RET_TURN);
 o.reset();o.state=Overtake::PASS;o.d0=0;o.tgt=0;
 o.step(0,0,0,6,true,true,0,1,300,300,false,false,false,100,l,r);assert(l>0&&r>0);
 o.step(0,0,0,5,true,true,0,1,300,300,false,false,false,200,l,r);assert(l==0&&r==0);
 o.reset();o.enabled=true;
 for(unsigned t=100;t<=300;t+=100)o.step(0,0,0,10,true,true,0,-1,300,300,false,false,false,t,l,r);
 assert(o.state==Overtake::OUT_TURN&&o.tgt==90&&o.side==1);
 o.state=Overtake::OUT_DRIVE;o.d0=0;o.tgt=90;
 o.step(90,0,11.9f,100,true,true,0,1,300,300,false,false,false,400,l,r);assert(o.state==Overtake::OUT_DRIVE);
 o.step(90,0,12.0f,100,true,true,0,1,300,300,false,false,false,500,l,r);assert(o.state==Overtake::ALIGN);
 o.reset();
 auto step=[&](unsigned t,bool mid){return o.step(0,0,0,100,true,true,0,1,300,300,false,mid,false,t,l,r);};
 o.state=Overtake::OUT_TURN;o.at=100;o.tgt=90;
 o.pause(true,200);o.pause(false,5200);assert(o.at==5100);
 step(5300,false);assert(o.state==Overtake::OUT_TURN);
 step(8700,false);assert(o.state==Overtake::FAIL&&l==0&&r==0);
 o.reset();o.state=Overtake::JOIN;o.at=0;o.tgt=0;o.adv=false;
 step(100,true);assert(!o.handover);step(200,false);assert(!o.joinSeen);
 step(300,true);step(450,true);assert(o.handover&&o.ok==1);
 o.reset();o.state=Overtake::JOIN;o.at=0;o.tgt=0;o.adv=false;
 step(3600,false);assert(o.state==Overtake::FAIL&&!o.handover);
 std::cout<<"PASS A4 pause, timeout output, stable rejoin, failed rejoin\n";
}
