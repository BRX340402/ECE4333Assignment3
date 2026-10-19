#pragma once
// Minimal host stubs to run UNO_Mapping.ino on a PC (protocol test only).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>
#include <deque>
typedef uint8_t byte;
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define A5 19
#define SDA 18
#define SCL 19
#ifndef PI
#define PI 3.14159265358979f
#endif
template<class T,class L,class H> T constrain(T x,L a,H b){return x<a?a:(x>b?b:x);}
extern uint32_t g_ms; extern int g_analog[20]; extern unsigned long g_pulse; extern int g_pwm[20],g_dig[20];
inline uint32_t millis(){return g_ms;}
inline void delay(uint32_t d){g_ms+=d;}
inline void delayMicroseconds(unsigned){}
inline int analogRead(int p){return g_analog[p];}
inline void analogWrite(int p,int v){g_pwm[p]=v;}
inline void digitalWrite(int p,int v){g_dig[p]=v;}
inline int digitalRead(int){return HIGH;}
inline void pinMode(int,int){}
inline unsigned long pulseIn(int,int,unsigned long){return g_pulse;}
struct HostSerial{
 std::deque<char> in;std::string out;
 void begin(long){}
 int available(){return (int)in.size();}
 int read(){if(in.empty())return -1;char c=in.front();in.pop_front();return c;}
 int availableForWrite(){return 63;}
 size_t write(char c){out.push_back(c);return 1;}
};
extern HostSerial Serial;
