/*
 * MPU6050_getdata.h - Demo2 gyro wrapper, extended for Assignment 3.
 * Added: MPU6050_calibrateBias() and MPU6050_getYawRateDps().
 */
#ifndef _MPU6050_getdata_H_
#define _MPU6050_getdata_H_
#include <Arduino.h>
class MPU6050_getdata
{
public:
  bool MPU6050_dveInit(void);
  bool MPU6050_calibration(void);                       // original Demo2 (100 fast samples)
  bool MPU6050_dveGetEulerAngles(float *Yaw);           // original Demo2 (clockwise-positive yaw)
  bool MPU6050_calibrateBias(uint16_t samples);         // NEW: robot must be still
  float MPU6050_getYawRateDps(void);                    // NEW: CCW-positive deg/s, bias removed

public:
  int16_t gz;
  unsigned long now, lastTime = 0;
  float dt;
  float agz = 0;
  long gzo = 0; // gyro offset (raw counts)
};

extern MPU6050_getdata MPU6050Getdata;
#endif
