/*
 * Estimator.h - pose estimation for the differential-steering robot.
 *
 *  1. Pure kinematic prediction (xk,yk,psik): Euler integration of
 *        v = r(wR+wL)/2,  w = r(wR-wL)/L,  x' = v cos(psi), y' = v sin(psi)
 *     driven by the SAME signed PWM values sent to the motors.
 *  2. Raw gyro heading (psig): integral of the calibrated yaw rate.
 *  3. Fused estimate (xf,yf,psif): gyro-propagated heading with online bias
 *     learning (stopped / driving straight, using the model), optional weak pull
 *     to the model, and landmark corrections from the camera (range + bearing).
 *     Position is propagated with the FUSED heading.
 */
#ifndef ESTIMATOR_H
#define ESTIMATOR_H
#include <Arduino.h>

struct EstState
{
  float xk, yk, psik;     // kinematic only (mm, mm, rad)
  float psig;             // gyro only (rad)
  float xf, yf, psif;     // fused
  float wL, wR;           // modelled wheel speeds (rad/s)
  float v, omega;         // body speed (mm/s), yaw rate (rad/s)
  float odo;              // path length since reset (mm), kinematic
  float gyroBias_dps;     // learned residual bias
};
extern EstState est;

float wrapPi(float a);
float Est_cmdToOmega(int cmd, float k);
void Est_reset(float x, float y, float psi);
void Est_update(float dt, int cmdL, int cmdR, float gyroCcwDps);
// Body-frame measurement (range d, bearing beta) -> inertial landmark position.
void Est_landmarkFromCamera(float d_mm, float beta_rad, float *xs, float *ys);
// Correct fused pose against a known landmark. Returns false if gated out.
bool Est_cameraCorrect(float xs, float ys, float d_mm, float beta_rad);
// Pull fused position toward a known point (intersection centre loop closure).
void Est_pullPosition(float xt, float yt, float gain);
#endif
