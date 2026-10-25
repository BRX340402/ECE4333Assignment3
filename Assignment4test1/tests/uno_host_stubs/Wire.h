#pragma once
#include <stdint.h>
extern bool g_imuPresent;extern int16_t g_gz;
struct TwoWire{int n=0;void begin(){}void end(){}void beginTransmission(int){}void write(uint8_t){}uint8_t endTransmission(bool=true){return g_imuPresent?0:2;}
 uint8_t requestFrom(int,int c){n=0;return g_imuPresent?c:0;}int read(){int b=(n++==0)?((g_gz>>8)&0xFF):(g_gz&0xFF);return b;}};
extern TwoWire Wire;
