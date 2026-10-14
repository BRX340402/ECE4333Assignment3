from pathlib import Path
import tempfile,subprocess
r=Path(__file__).resolve().parents[1]
stub='''#pragma once
#include <stdint.h>
#define PI 3.14159265358979323846f
extern uint32_t clockMs;
inline uint32_t millis(){return clockMs;}inline void delay(unsigned n){clockMs+=n;}
struct FakeWire {int value=131,index=0,jitter=0;bool ok=true;void begin(){}void beginTransmission(int){}void write(int){}int endTransmission(bool=true){return ok?0:1;}int requestFrom(int,int){return ok?2:0;}int read(){int n=value+((index/2)%2?jitter:0);return (index++%2)?(n&255):((n>>8)&255);}};
extern FakeWire Wire;
'''
code='''#include <cassert>
#include "PoseEstimate.h"
uint32_t clockMs=0;FakeWire Wire;
int main(){PoseEstimate p;p.begin();assert(p.gyro&&p.biasReady&&p.bias==131);
p.zero(clockMs);Wire.value=262;p.step(0,0,clockMs+20);assert(p.gyro&&p.yawRate<0);
Wire.jitter=500;p.begin();assert(!p.gyro&&!p.biasReady);
Wire.jitter=0;Wire.value=131;Wire.ok=false;p.begin();assert(!p.gyro&&!p.biasReady);
Wire.ok=true;p.begin();assert(p.gyro);Wire.value=32760;p.step(0,0,clockMs+20);assert(!p.gyro);
Wire.value=131;p.begin();p.step(0,0,clockMs+300);assert(!p.gyro);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'Wire.h').write_text(stub);(t/'test.cpp').write_text(code)
 subprocess.run(['g++','-std=c++11','-I'+str(t),'-I'+str(r/'UNO_Assignment2'),str(t/'test.cpp'),'-o',str(t/'test')],check=True);subprocess.run([str(t/'test')],check=True)
print('PASS stationary bias calibration, sample-spread rejection, I2C failure, saturated gyro, delayed updates')
