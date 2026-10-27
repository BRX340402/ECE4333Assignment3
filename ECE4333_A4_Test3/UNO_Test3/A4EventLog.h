#pragma once
#include <stdint.h>
// Fixed-memory FIFO for obstacle state transitions. No dynamic allocation.
// Drops oldest event if ESP32/serial cannot keep up; exported "lost" tracks this.
struct A4EventLog {
 static const uint8_t CAPACITY=3;
 struct Event{
  uint32_t t;
  int32_t yaw10,dist10;
  int16_t x,y,range,side10,pass10;
  uint16_t run,dropped;
  uint8_t before,after,pan,seg;
  int8_t side;
 };
 Event queue[CAPACITY];
 uint8_t head=0,count=0;
 uint16_t lost=0;
 void push(const Event &e){
  if(count==CAPACITY){head=(head+1)%CAPACITY;count--;lost++;}
  queue[(head+count)%CAPACITY]=e;count++;
 }
 bool peek(Event &e)const{if(!count)return false;e=queue[head];return true;}
 void remove(){if(count){head=(head+1)%CAPACITY;count--;}}
};
