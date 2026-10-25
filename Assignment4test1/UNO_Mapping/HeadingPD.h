#pragma once
#include <stdint.h>
// PD heading controller: the gyro "inner loop" of the nested control in Assignment 3, part 5.
//   err  = target heading - gyro-fused heading   (degrees, + = the target is to the LEFT)
//   rate = measured yaw rate from the gyro        (deg/s,  + = turning left)
//   out  = wheel differential u (PWM units, + = turn left): left = v - u, right = v + u
// The D term uses the gyro rate directly (no numerical differentiation of the error), so it
// damps the turn without amplifying noise and without a derivative kick when the setpoint
// jumps at an outer-loop update.
struct HeadingPD{
 int kp10=15;     // proportional gain x10  (PWM per degree)
 int kd100=10;    // derivative gain x100   (PWM per deg/s)
 int maxOut=70;   // |u| limit
 float out(float err,float rate)const{
  float u=kp10*0.1f*err-kd100*0.01f*rate;
  return u>maxOut?maxOut:(u<-maxOut?-maxOut:u);
 }
 // Wheel PWM for a heading error. v = forward PWM; v < 1 turns on the spot (at least pivotMin, none inside 3 deg).
 void steer(float err,float rate,float v,int pivotMin,int &l,int &r)const{
  float u=out(err,rate),ae=err<0?-err:err;
  if(v<1){
   if(ae<3&&rate<20&&rate>-20)u=0;
   else if(u>0&&u<pivotMin)u=pivotMin;else if(u<0&&u>-pivotMin)u=-pivotMin;
   l=(int)(-u);r=(int)u;
  }else{float a=v-u,b=v+u;if(a<0)a=0;if(b<0)b=0;l=(int)a;r=(int)b;}
 }
};
