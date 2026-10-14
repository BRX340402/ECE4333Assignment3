/*
 * Control.h - motors, line-tracking PD controller, gyro heading (inner-loop) PD
 * controller and the ultrasonic obstacle sensor.
 */
#ifndef CONTROL_H
#define CONTROL_H
#include <Arduino.h>

struct LineSense
{
  int rawL, rawM, rawR;
  float wL, wM, wR;      // darkness weights 0..1
  float sum;
  float pos;             // -1 (line far left) .. +1 (line far right)
  bool valid;            // line seen
  bool allBlack;         // crossing line under all three sensors
  bool lifted;           // car off the ground
};

void Control_init(void);
void Control_setWheels(int left, int right);       // signed PWM, -255..255
void Control_stop(bool cutDriver);                 // cutDriver = also pull TB6612 STBY low
void Control_getLastCmd(int *l, int *r);

void Line_read(LineSense &s);
void Line_reset(void);
// PD line tracker. Returns the steering term (PWM) it applied.
float Line_control(const LineSense &s, float dt, float gyroCcwDps, int basePwm);
// Line lost: pivot toward the side where it was last seen.
void Line_searchLost(void);

// Inner loop: hold psiTarget using fused heading + gyro rate. basePwm = forward speed.
void Heading_control(float psiTargetRad, float psiNowRad, float gyroCcwDps, int basePwm);
float Heading_lastErrorDeg(void);

void Key_init(void);
bool Key_pressed(void);                            // debounced falling edge, once per press

void Ultra_init(void);
void Ultra_update(unsigned long now_ms);
int Ultra_distanceMM(void);                        // 9999 = nothing in range
#endif
