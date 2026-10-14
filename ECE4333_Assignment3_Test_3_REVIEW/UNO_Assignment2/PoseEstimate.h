#pragma once
#include <Wire.h>
#include <math.h>
#include "PoseConfig.h"
// MPU6050 register addresses / 131 LSB per deg/s match ex2 MPU6050.cpp defaults.
struct PoseEstimate {
 float yawDegrees=0,gyroSign=GYRO_SIGN;
 float x=0,y=0,heading=0,kinX=0,kinY=0,kinHeading=0,bias=0;
 bool gyro=false;uint32_t last=0;bool corrected=false;
 static float wrap(float a){while(a>PI)a-=2*PI;while(a< -PI)a+=2*PI;return a;}
 bool writeReg(uint8_t reg,uint8_t v){Wire.beginTransmission(0x68);Wire.write(reg);Wire.write(v);return Wire.endTransmission()==0;}
 bool raw(int16_t &v){Wire.beginTransmission(0x68);Wire.write(0x47);if(Wire.endTransmission(false))return false;if(Wire.requestFrom(0x68,2)!=2)return false;v=(int16_t)((Wire.read()<<8)|Wire.read());return true;}
 void begin(){Wire.begin();
 #if defined(WIRE_HAS_TIMEOUT)
 Wire.setWireTimeout(3000,true);
 #endif
 gyro=writeReg(0x6B,0)&&writeReg(0x1B,0);long sum=0;int16_t v;
 for(int i=0;i<100&&gyro;i++){gyro=raw(v);if(gyro)sum+=v;delay(5);}bias=sum/100.0f;last=millis();}
 void step(int left,int right,uint32_t now){
 float dt=uint32_t(now-last)*0.001f;if(dt<0.01f)return;last=now;
 // Skip unusually delayed integration intervals; do not disable gyro permanently.
 if(dt>0.25f)return;
 // Wheel-command prediction MUST continue even when the IMU is unavailable.
 float vl=left*LEFT_CM_S_PER_PWM,vr=right*RIGHT_CM_S_PER_PWM,v=(vl+vr)*0.5f;
 float wk=(vr-vl)/TRACK_WIDTH_CM,w=wk;int16_t gz;
 if(gyro&&raw(gz)){float rate=gyroSign*(gz-bias)/131.0f;yawDegrees+=rate*dt;w=0.95f*rate*PI/180.0f+0.05f*wk;}else gyro=false;
 kinX+=v*cos(kinHeading+wk*dt/2)*dt;kinY+=v*sin(kinHeading+wk*dt/2)*dt;kinHeading=wrap(kinHeading+wk*dt);
 x+=v*cos(heading+w*dt/2)*dt;y+=v*sin(heading+w*dt/2)*dt;heading=wrap(heading+w*dt);
 }
 // Range alone cannot recover position AND heading. Use a mapped approach and
 // existing gyro/kinematic pose; gate the bearing innovation before correcting.
 bool camera(float forward,float bearing){
  if(!POSE_CALIBRATED||!LANDMARK_ENABLED||!gyro||forward<=0||fabs(bearing)>0.6f||fabs(wrap(heading-APPROACH_HEADING_RAD))>0.4f)return false;
  float dx=LANDMARK_X_CM-x-CAMERA_FORWARD_CM*cos(heading),dy=LANDMARK_Y_CM-y-CAMERA_FORWARD_CM*sin(heading);
  float predicted=sqrt(dx*dx+dy*dy),range=forward/cos(bearing);
  if(predicted<1||fabs(predicted-range)>40)return false;
  float hObs=wrap(atan2(dy,dx)-bearing),dh=wrap(hObs-heading);
  if(fabs(dh)>0.25f)return false;
  heading=wrap(heading+0.1f*dh);
  float ox=LANDMARK_X_CM-range*cos(heading+bearing)-CAMERA_FORWARD_CM*cos(heading);
  float oy=LANDMARK_Y_CM-range*sin(heading+bearing)-CAMERA_FORWARD_CM*sin(heading);
  x+=CAMERA_FUSION_GAIN*(ox-x);y+=CAMERA_FUSION_GAIN*(oy-y);corrected=true;return true;
 }
};
