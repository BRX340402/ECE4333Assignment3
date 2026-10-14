#include <cassert>
#include <iostream>
#include "../ESP32_Assignment2/RedDetector.h"
int main(){assert(redPixel(255,0,0));assert(!redPixel(0,255,0));assert(redPixel(255,20,20));assert(!redPixel(255,255,255));assert(!redPixel(20,0,0));assert(redPixel(255,0,0));
 uint8_t cells[16]={0};uint16_t q[16];cells[5]=8;cells[6]=5;cells[10]=7;auto r=largestRedRegion(cells,q,16,16);assert(r.pixels==20&&r.x==4&&r.y==4&&r.w==8&&r.h==8);std::cout<<"PASS: red/green channel dominance, white/dark rejection, connected colour region geometry.\n";}
