#pragma once
#include <stdint.h>
#include <math.h>
#include "PoseConfig.h"
#include "Track.h"
#if !defined(A3_HOST_SIM)
#include <Wire.h>
#endif
// Two pose estimates, both in the inertial frame of Track.h (cm, radians):
//   kin*  = PREDICTED : pure kinematic model, wheel speeds from the PWM sent to
//                       the motors, heading from (vR-vL)/W.
//   x,y,heading = FUSED : same translation model, but heading from the gyro
//                       (bias tracked), plus landmark corrections (nudge/camera).
// vScale = speed-model scale learned online from corner events (reported in the log;
// multiply LEFT/RIGHT_CM_S_PER_PWM by it to calibrate the PREDICTED model).
// yawDegrees = raw gyro heading (unwrapped, 0 at Start) used by the controller.
//
// GYRO: it is integrated, so it must be sampled continuously from boot until the
// end of the run (every loop pass, >=100 Hz here) - there is no "turn it on only
// for corners".  The chip stays awake (sleep bit cleared); a dropped I2C read
// does not kill it any more (5 failures in a row, then it re-initialises once
// per second); the zero-rate offset is re-learned whenever the car stands still.
struct PoseEstimate {
 float yawDegrees=0,gyroSign=GYRO_SIGN,rateDps=0;
 float x=0,y=0,heading=0,kinX=0,kinY=0,kinHeading=0,odo=0,speed=0,bias=0,vScale=1.0f;
 bool gyro=false,corrected=false;
 uint32_t last=0,stillAt=0,retryAt=0;uint8_t fails=0;bool still=false;
 static float wrap(float a){while(a>M_PI)a-=2*M_PI;while(a< -M_PI)a+=2*M_PI;return a;}
#if defined(A3_HOST_SIM)
 bool raw(int16_t &v);              // supplied by the simulator
 bool chipInit(){return true;}
#else
 bool writeReg(uint8_t reg,uint8_t v){Wire.beginTransmission(0x68);Wire.write(reg);Wire.write(v);return Wire.endTransmission()==0;}
 bool raw(int16_t &v){Wire.beginTransmission(0x68);Wire.write(0x47);if(Wire.endTransmission(false))return false;if(Wire.requestFrom(0x68,2)!=2)return false;v=(int16_t)((Wire.read()<<8)|Wire.read());return true;}
 // wake, PLL clock (gyro X), 44 Hz low-pass, +-500 deg/s
 bool chipInit(){return writeReg(0x6B,0x01)&&writeReg(0x1A,0x03)&&writeReg(0x1B,0x08);}
#endif
 void begin(){
#if !defined(A3_HOST_SIM)
  Wire.begin();Wire.setClock(400000);
  #if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(3000,true);
  #endif
#endif
  gyro=chipInit();long sum=0;int16_t v;int n=0;
  for(int i=0;i<200&&gyro;i++){if(raw(v)){sum+=v;n++;}
#if !defined(A3_HOST_SIM)
   delay(5);
#endif
  }
  if(n>20)bias=(float)sum/n;else gyro=false;
 }
 // Zero the pose at Start (origin = start point, heading = first segment).  Bias is kept.
 void reset(){yawDegrees=rateDps=x=y=heading=kinX=kinY=kinHeading=odo=speed=0;corrected=false;}
 // Corner events tell how far the car really went: ratio = expected/estimated distance.
 void learn(float ratio){float k=vScale*(1.0f+SCALE_LEARN_GAIN*(ratio-1.0f));if(k<SCALE_MIN)k=SCALE_MIN;if(k>SCALE_MAX)k=SCALE_MAX;vScale=k;}
 static float wheel(int pwm,float k){int a=pwm<0?-pwm:pwm;a-=PWM_DEADBAND;if(a<=0)return 0;return (pwm<0?-1.0f:1.0f)*k*a;}
 void step(int left,int right,uint32_t nowUs){
  uint32_t dtu=nowUs-last;if(dtu<8000UL)return;last=nowUs;
  float dt=dtu*1e-6f;if(dt>1.0f)dt=1.0f;               // after a stall integrate what really elapsed
  uint32_t nowMs=nowUs/1000UL;
  float vl=wheel(left,LEFT_CM_S_PER_PWM),vr=wheel(right,RIGHT_CM_S_PER_PWM),v=(vl+vr)*0.5f,wk=(vr-vl)/TRACK_WIDTH_CM;
  int16_t gz;
  if(gyro){
   if(raw(gz)){
    fails=0;float r=gz-bias;
    if(left||right)still=false;                         // zero-rate update only when standing still
    else{if(!still){still=true;stillAt=nowMs;}
     else if(nowMs-stillAt>400&&fabsf(r)<300.0f)bias+=0.02f*r;}
    rateDps=gyroSign*(gz-bias)/GYRO_LSB_PER_DPS;yawDegrees+=rateDps*dt;
   }else if(++fails>=5){gyro=false;retryAt=nowMs+200UL;}
  }else if((int32_t)(nowMs-retryAt)>=0){retryAt=nowMs+200UL;gyro=chipInit();fails=0;rateDps=0;}
  float wf=gyro?rateDps*(float)(M_PI/180.0):wk;         // fused turn rate: gyro, model if gyro is down
  kinX+=v*cosf(kinHeading+wk*dt*0.5f)*dt;kinY+=v*sinf(kinHeading+wk*dt*0.5f)*dt;kinHeading=wrap(kinHeading+wk*dt);
  float vf=v*vScale;                                     // fused translation uses the learned speed scale
  x+=vf*cosf(heading+wf*dt*0.5f)*dt;y+=vf*sinf(heading+wf*dt*0.5f)*dt;heading=wrap(heading+wf*dt);
  speed=vf;odo+=fabsf(vf)*dt;
 }
 // Landmark correction of the FUSED position only (predicted pose stays pure).
 void nudge(float dx,float dy){x+=dx;y+=dy;corrected=true;}
 // Range alone cannot recover position AND heading: use the mapped approach, the
 // gyro heading and the known sign position, gate the innovation, correct gently.
 bool camera(float forward,float bearing){
  if(!LANDMARK_ENABLED||!gyro||forward<=0||fabsf(bearing)>0.6f||fabsf(wrap(heading-APPROACH_HEADING_RAD))>0.4f)return false;
  float dx=LANDMARK_X_CM-x-CAMERA_FORWARD_CM*cosf(heading),dy=LANDMARK_Y_CM-y-CAMERA_FORWARD_CM*sinf(heading);
  float predicted=sqrtf(dx*dx+dy*dy),range=forward/cosf(bearing);
  if(predicted<1||fabsf(predicted-range)>40)return false;
  float ox=LANDMARK_X_CM-range*cosf(heading+bearing)-CAMERA_FORWARD_CM*cosf(heading);
  float oy=LANDMARK_Y_CM-range*sinf(heading+bearing)-CAMERA_FORWARD_CM*sinf(heading);
  x+=CAMERA_FUSION_GAIN*(ox-x);y+=CAMERA_FUSION_GAIN*(oy-y);corrected=true;return true;
 }
};
