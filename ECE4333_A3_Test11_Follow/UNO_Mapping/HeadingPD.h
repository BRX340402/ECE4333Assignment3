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
};
