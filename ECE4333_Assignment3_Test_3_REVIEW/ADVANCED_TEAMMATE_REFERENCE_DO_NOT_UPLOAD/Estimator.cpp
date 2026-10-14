#include "Estimator.h"
#include "Config.h"
#include <math.h>

EstState est;

static const float DEG2RAD = 0.017453292f;
static const float RAD2DEG = 57.29578f;
static float stillTime = 0.0f;

float wrapPi(float a)
{
  while (a > PI) a -= TWO_PI;
  while (a <= -PI) a += TWO_PI;
  return a;
}

// PWM command -> wheel angular speed (rad/s), with dead band.
float Est_cmdToOmega(int cmd, float k)
{
  if (cmd == 0) return 0.0f;
  float m = (float)abs(cmd) - (float)PWM_DEADBAND;
  if (m < 0) m = 0;
  float w = m * k;
  return cmd > 0 ? w : -w;
}

void Est_reset(float x, float y, float psi)
{
  est.xk = est.xf = x;
  est.yk = est.yf = y;
  est.psik = est.psig = est.psif = wrapPi(psi);
  est.odo = 0;
  stillTime = 0;
}

void Est_update(float dt, int cmdL, int cmdR, float gyroCcwDps)
{
  // ---- wheel model with motor lag ----
  float wLt = Est_cmdToOmega(cmdL, MOTOR_W_PER_PWM_L);
  float wRt = Est_cmdToOmega(cmdR, MOTOR_W_PER_PWM_R);
  float a = dt / (MOTOR_TAU_S + dt);
  est.wL += a * (wLt - est.wL);
  est.wR += a * (wRt - est.wR);
  float vL = WHEEL_RADIUS_MM * est.wL;
  float vR = WHEEL_RADIUS_MM * est.wR;
  est.v = 0.5f * (vL + vR);
  est.omega = (vR - vL) / WHEEL_BASE_MM;

  // ---- 1. kinematic-only dead reckoning ----
  est.psik = wrapPi(est.psik + est.omega * dt);
  est.xk += est.v * cosf(est.psik) * dt;
  est.yk += est.v * sinf(est.psik) * dt;

  // ---- 2. raw gyro heading ----
  float g = gyroCcwDps * DEG2RAD;
  est.psig = wrapPi(est.psig + g * dt);

  // ---- 3. fused ----
  // bias learning: stopped -> gyro should read 0; straight -> gyro should match model (~0)
  bool stopped = (cmdL == 0 && cmdR == 0);
  stillTime = stopped ? stillTime + dt : 0.0f;
  float omegaModelDps = est.omega * RAD2DEG;
  if (stopped && stillTime > 0.3f)
  {
    est.gyroBias_dps += (dt / (GYRO_BIAS_STILL_TAU_S + dt)) * (gyroCcwDps - est.gyroBias_dps);
  }
  else if (!stopped && fabsf(omegaModelDps) < GYRO_BIAS_STRAIGHT_W_DPS && fabsf(gyroCcwDps) < 2.0f * GYRO_BIAS_STRAIGHT_W_DPS)
  {
    float resid = gyroCcwDps - omegaModelDps;
    est.gyroBias_dps += (dt / (GYRO_BIAS_STRAIGHT_TAU_S + dt)) * (resid - est.gyroBias_dps);
  }
  float gf = (gyroCcwDps - est.gyroBias_dps) * DEG2RAD;
  float psiPrev = est.psif;
  est.psif = wrapPi(est.psif + gf * dt);
  if (FUSION_MODEL_PULL_TAU_S > 0.0f)   // optional weak pull toward the dead-reckoned heading
  {
    est.psif = wrapPi(est.psif + (dt / (FUSION_MODEL_PULL_TAU_S + dt)) * wrapPi(est.psik - est.psif));
  }
  float psiMid = psiPrev + 0.5f * wrapPi(est.psif - psiPrev);
  est.xf += est.v * cosf(psiMid) * dt;
  est.yf += est.v * sinf(psiMid) * dt;

  est.odo += fabsf(est.v) * dt;
}

void Est_landmarkFromCamera(float d_mm, float beta_rad, float *xs, float *ys)
{
  float xb = CAM_FWD_OFFSET_MM + d_mm * cosf(beta_rad);
  float yb = d_mm * sinf(beta_rad);
  float c = cosf(est.psif), s = sinf(est.psif);
  *xs = est.xf + c * xb - s * yb;
  *ys = est.yf + s * xb + c * yb;
}

bool Est_cameraCorrect(float xs, float ys, float d_mm, float beta_rad)
{
  float xb = CAM_FWD_OFFSET_MM + d_mm * cosf(beta_rad);
  float yb = d_mm * sinf(beta_rad);
  float rm = sqrtf(xb * xb + yb * yb);        // measured range from robot reference point
  float bm = atan2f(yb, xb);                  // measured body bearing
  float dx = xs - est.xf, dy = ys - est.yf;
  float rp = sqrtf(dx * dx + dy * dy);        // predicted range
  if (rp < 1.0f) return false;
  float bp = wrapPi(atan2f(dy, dx) - est.psif);   // predicted body bearing
  float er = rp - rm;                         // >0: we are closer than we thought
  float eb = wrapPi(bm - bp);
  if (fabsf(er) > CAM_GATE_RANGE_MM || fabsf(eb) > CAM_GATE_BEARING_DEG * DEG2RAD) return false;
  est.xf += CAM_K_POS * er * dx / rp;
  est.yf += CAM_K_POS * er * dy / rp;
  est.psif = wrapPi(est.psif - CAM_K_HEAD * eb);
  return true;
}

void Est_pullPosition(float xt, float yt, float gain)
{
  est.xf += gain * (xt - est.xf);
  est.yf += gain * (yt - est.yf);
}
