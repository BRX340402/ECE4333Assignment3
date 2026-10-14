#pragma once
#include <Wire.h>
#include <math.h>
#include "MapConfig.h"
// Z gyro + wheel-command model.
// IMU is auto-detected (ELEGOO V4 boards ship with either chip):
//   type 1 = MPU6050 family at 0x68/0x69 (also clones: any WHO_AM_I that answers),
//            set to +/-500 dps (65.5 LSB/dps), 44 Hz low-pass, gyro Z at 0x47 (big endian)
//   type 2 = QMI8658 at 0x6B/0x6A (WHO_AM_I 0x00 == 0x05),
//            set to +/-512 dps (64 LSB/dps), gyro Z at 0x3F/0x40 (little endian)
// ELEGOO V4 kit: GY-521 (MPU6050) on A4=SDA / A5=SCL, AD0 not connected -> 0x68 (WHO_AM_I 0x68 = 104).
// If nothing answers at boot, the I2C bus is recovered (9 SCL pulses + STOP: an MPU6050
// interrupted mid-read by a UNO reset can hold SDA low until power-off) and detection is
// retried every 2 s while the car is parked.
// Test10 run1 showed yaw = 0 for a whole run: never drive a mapping run without a gyro.
struct PoseEstimate {
 float lsbPerDps=65.5f;
 float yawRate=0,yawDegrees=0,gyroSign=-1.0f; // yawDegrees: unwrapped, + = left turn (check with GS)
 float x=0,y=0,heading=0,kinX=0,kinY=0,kinHeading=0,bias=0,dist=0;
 bool biasReady=false,gyro=false;uint32_t last=0,lastDetect=0;
 uint8_t imuType=0,imuAddr=0,whoMpu=0xFF,whoQmi=0xFF; // diagnostics, reported in N221 (IM / IW / IQ)
 uint8_t readFails=0,busRecoveries=0;
 // Zero button on the web page: non-blocking 1 s bias re-measurement, then heading/position zeroed.
 bool calibrating=false,calDone=false;uint32_t calStart=0,calLast=0;long calSum=0;int calN=0;
 void startRecal(uint32_t now){if(!biasReady){zero(now);calDone=true;return;}calibrating=true;calStart=calLast=now;calSum=0;calN=0;}
 __attribute__((noinline)) void zero(uint32_t now){x=y=heading=kinX=kinY=kinHeading=yawDegrees=dist=0;last=now;}
 static float wrap(float a){while(a>PI)a-=2*PI;while(a< -PI)a+=2*PI;return a;}
 static bool writeReg(uint8_t addr,uint8_t reg,uint8_t v){Wire.beginTransmission(addr);Wire.write(reg);Wire.write(v);return Wire.endTransmission()==0;}
 static bool readReg(uint8_t addr,uint8_t reg,uint8_t *buf,uint8_t n){
  Wire.beginTransmission(addr);Wire.write(reg);if(Wire.endTransmission(false))return false;
  if(Wire.requestFrom(addr,n)!=n)return false;for(uint8_t i=0;i<n;i++)buf[i]=Wire.read();return true;
 }
 bool raw(int16_t &v){
  uint8_t b[2];
  if(imuType==1){if(!readReg(imuAddr,0x47,b,2))return false;v=(int16_t)((b[0]<<8)|b[1]);return true;}
  if(imuType==2){if(!readReg(imuAddr,0x3F,b,2))return false;v=(int16_t)((b[1]<<8)|b[0]);return true;}
  return false;
 }
 void busRecover(){
  Wire.end();
  pinMode(SDA,INPUT_PULLUP);pinMode(SCL,INPUT_PULLUP);delayMicroseconds(10);
  for(uint8_t i=0;i<9&&digitalRead(SDA)==LOW;i++){pinMode(SCL,OUTPUT);digitalWrite(SCL,LOW);delayMicroseconds(10);pinMode(SCL,INPUT_PULLUP);delayMicroseconds(10);}
  pinMode(SDA,OUTPUT);digitalWrite(SDA,LOW);delayMicroseconds(10);pinMode(SCL,INPUT_PULLUP);delayMicroseconds(10);pinMode(SDA,INPUT_PULLUP);delayMicroseconds(10); // STOP
  Wire.begin();
 #if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(3000,true);
 #endif
  busRecoveries++;
 }
 bool detect(){
  imuType=0;uint8_t w;
  // QMI8658 first: its WHO_AM_I is specific (0x05)
  for(uint8_t a=0x6B;a>=0x6A;a--){
   if(readReg(a,0x00,&w,1)){whoQmi=w;
    if(w==0x05){
     writeReg(a,0x60,0xB0);delay(20);                       // soft reset
     if(writeReg(a,0x02,0x40)&&writeReg(a,0x04,0x55)&&writeReg(a,0x08,0x03)){ // auto-increment LE; gyro +/-512dps; enable acc+gyro
      imuType=2;imuAddr=a;lsbPerDps=64.0f;delay(50);return true;}
    }
   }
  }
  for(uint8_t a=0x68;a<=0x69;a++){
   if(readReg(a,0x75,&w,1)){whoMpu=w;
    if(writeReg(a,0x6B,0)&&writeReg(a,0x1A,3)&&writeReg(a,0x1B,0x08)){imuType=1;imuAddr=a;lsbPerDps=65.5f;delay(50);return true;}
   }
  }
  return false;
 }
 // Blocking ~0.8 s: call only while the car is still.
 bool calibrate(){
  long sum=0;int16_t v;int ok=0;
  for(int i=0;i<200;i++){if(raw(v)){sum+=v;ok++;}delay(4);}
  biasReady=ok>=150;if(biasReady)bias=sum/(float)ok;
  gyro=biasReady;readFails=0;return biasReady;
 }
 void begin(){
  Wire.begin();
 #if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(3000,true);
 #endif
  delay(100);                                   // IMU power-up time
  if(!detect()){busRecover();if(detect())calibrate();}else calibrate();
  last=lastDetect=millis();
 }
 // parked: motors off for a while -> retry detection, slowly track gyro bias drift
 void step(int left,int right,uint32_t now,bool parked){
  if(!gyro&&biasReady&&parked)biasReady=false;   // reads keep failing while parked: start over
  if(!biasReady&&parked&&uint32_t(now-lastDetect)>=2000){lastDetect=now;busRecover();if(detect())calibrate();last=millis();return;}
  if(calibrating){
   int16_t v;if(uint32_t(now-calLast)>=5){calLast=now;if(raw(v)){calSum+=v;calN++;}}
   yawRate=0;
   if(uint32_t(now-calStart)>=1000){if(calN>=50)bias=calSum/(float)calN;zero(now);calibrating=false;calDone=true;}
   return;
  }
  float dt=uint32_t(now-last)*0.001f;if(dt<0.01f)return;last=now;
  if(dt>0.25f)return; // skip an abnormal stall instead of integrating a huge step
  float vl=left*LEFT_CM_S_PER_PWM,vr=right*RIGHT_CM_S_PER_PWM,v=(vl+vr)*0.5f;
  float wk=(vr-vl)/TRACK_WIDTH_CM,w=wk;int16_t gz;
  if(biasReady&&raw(gz)){
   gyro=true;readFails=0;
   if(parked&&fabs(gz-bias)<2*lsbPerDps)bias+=0.002f*(gz-bias);
   float rate=gyroSign*(gz-bias)/lsbPerDps;yawRate=rate;
   yawDegrees+=rate*dt; // still integrates while parked so a hand rotation can verify the sign
   w=rate*PI/180.0f;
  }else{
   yawRate=0;
   if(biasReady&&++readFails>=20){gyro=false;} // ~0.2 s of failed reads: report it
   if(gyro)w=0; // single missed read: keep heading, do not switch to the wheel model
  }
  dist+=v*dt;
  kinX+=v*cos(kinHeading+wk*dt/2)*dt;kinY+=v*sin(kinHeading+wk*dt/2)*dt;kinHeading=wrap(kinHeading+wk*dt);
  x+=v*cos(heading+w*dt/2)*dt;y+=v*sin(heading+w*dt/2)*dt;heading=wrap(heading+w*dt);
 }
};
