#pragma once
#include <stdint.h>
// Test16 Assignment 5 build: the race-only modules (RaceRules.h STOP/light rules, Avoid.h overtaking) are replaced by
// these empty stand-ins with the same names, so the sketch compiles unchanged and the compiler drops their code.
struct RaceRules{
 enum State:uint8_t{GO=0,SLOW=1,HOLD=2,WAIT_GREEN=3,DONE=4};
 static constexpr State st=GO;static constexpr uint8_t ph=0,zone=0;
 bool enabled=false,changed=false;uint8_t clearCm=45,stops=0,stopsAI=0;uint16_t req=0;
 void reset(){}void hold(uint32_t){}void enter(State,uint32_t){}bool stopped()const{return false;}bool slow()const{return false;}uint8_t ts()const{return 2;}
 int8_t look()const{return 0;}void segment(uint8_t){}void setZone(uint8_t){}void colour(uint16_t,bool,uint8_t,bool,bool,uint32_t){}
 void stopLine(float,uint8_t,uint32_t){}void captured(uint16_t,uint32_t){}void ai(uint16_t,bool,bool,uint32_t){}void tick(uint32_t,int,bool){}
};
struct Avoid{
 enum State:uint8_t{IDLE=0,TAIL=1,OUT=2,PASS=3,BACK=4,JOIN=5};
 static const int STOP_CM=10;static constexpr State state=IDLE;static constexpr bool past=false,lost=false;bool handover=false;
 static constexpr int8_t look=0;static constexpr uint8_t scale=100;static constexpr float H=0,lostCm=0,along=0,cornerCm=0;
 bool enabled=false,armed=false,mapDone=false,force=false;int obsAt=0;float prog=0;int trigCm=0,latCm=0,margin=0,tailCm=0,gapCm=0,speed=0,joinAdv=0;float turn=0,lastDist=0;uint8_t ok=0,fails=0;
 void reset(){}bool active()const{return false;}
 bool step(float,float,float,int,bool,bool,float,int8_t,float,bool,bool,bool,bool,uint32_t,int&l,int&r){l=r=0;return false;}
};
