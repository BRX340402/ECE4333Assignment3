#include "Control.h"
#include "Config.h"
#include "Estimator.h"
#include "DeviceDriverSet_xxx0.h"
#include <math.h>

static DeviceDriverSet_Motor AppMotor;
static DeviceDriverSet_ITR20001 AppITR20001;

static int lastCmdL = 0, lastCmdR = 0;
static float lastPos = 0, dFilt = 0, lastSide = 1.0f;
static float headErrDeg = 0;

void Control_init(void)
{
  AppMotor.DeviceDriverSet_Motor_Init();
  AppITR20001.DeviceDriverSet_ITR20001_Init();
  Control_stop(false);
}

void Control_setWheels(int left, int right)
{
  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);
  lastCmdL = left;
  lastCmdR = right;
  // Demo2 convention: motor A = RIGHT, motor B = LEFT.
  AppMotor.DeviceDriverSet_Motor_control(right >= 0 ? direction_just : direction_back, (uint8_t)abs(right),
                                         left >= 0 ? direction_just : direction_back, (uint8_t)abs(left),
                                         control_enable);
}

void Control_stop(bool cutDriver)
{
  Control_setWheels(0, 0);
  if (cutDriver)
  {
    AppMotor.DeviceDriverSet_Motor_control(direction_just, 0, direction_just, 0, control_disable);
  }
}

void Control_getLastCmd(int *l, int *r)
{
  *l = lastCmdL;
  *r = lastCmdR;
}

/* ---------------- line sensing ---------------- */
static float darkness(int raw)
{
  float w = (float)(raw - LINE_WHITE_RAW) / (float)(LINE_BLACK_RAW - LINE_WHITE_RAW);
  return constrain(w, 0.0f, 1.0f);
}

void Line_read(LineSense &s)
{
  s.rawL = (int)AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_L();
  s.rawM = (int)AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_M();
  s.rawR = (int)AppITR20001.DeviceDriverSet_ITR20001_getAnaloguexxx_R();
  s.lifted = (s.rawL > LIFT_RAW && s.rawM > LIFT_RAW && s.rawR > LIFT_RAW);
  if (s.lifted)
  {
    s.wL = s.wM = s.wR = 0;
  }
  else
  {
    s.wL = darkness(s.rawL);
    s.wM = darkness(s.rawM);
    s.wR = darkness(s.rawR);
  }
  s.sum = s.wL + s.wM + s.wR;
  s.valid = s.sum >= LINE_LOST_SUM;
  s.pos = s.valid ? (s.wR - s.wL) / s.sum : lastPos;
  s.allBlack = !s.lifted && s.wL > INTERSECT_W && s.wM > INTERSECT_W && s.wR > INTERSECT_W;
}

void Line_reset(void)
{
  lastPos = 0;
  dFilt = 0;
}

/* ---------------- line PD ----------------
 * pos > 0  : line is to the right -> turn right (left wheel faster).
 * steer    = Kp*pos + Kd*d(pos)/dt + Kg*yawRate(CCW)  (last term damps rotation)
 */
float Line_control(const LineSense &s, float dt, float gyroCcwDps, int basePwm)
{
  float err = s.pos;
  float d = (err - lastPos) / dt;
  lastPos = err;
  dFilt += D_FILTER_ALPHA * (d - dFilt);
  if (fabsf(err) > 0.1f) lastSide = err > 0 ? 1.0f : -1.0f;

  float steer = KP_LINE * err + KD_LINE * dFilt + KG_LINE * gyroCcwDps;
  steer = constrain(steer, -STEER_MAX, STEER_MAX);

  float base = (float)basePwm - SLOW_GAIN * fabsf(err);
  if (base < MIN_BASE_PWM) base = MIN_BASE_PWM;
  if (base > basePwm) base = basePwm;

  int left = (int)(base + steer);
  int right = (int)(base - steer);
  left = constrain(left, -REVERSE_MAX_PWM, 255);
  right = constrain(right, -REVERSE_MAX_PWM, 255);
  Control_setWheels(left, right);
  return steer;
}

void Line_searchLost(void)
{
  int p = LOST_SEARCH_PWM;
  // lastSide > 0: line was on the right -> pivot right (left forward, right back)
  if (lastSide > 0) Control_setWheels(p, -p);
  else Control_setWheels(-p, p);
}

/* ---------------- gyro inner loop ----------------
 * e = target - heading (CCW positive). u>0 turns left: right wheel faster.
 * u = Kp*e - Kd*yawRate   (setpoint constant => d(e)/dt = -yawRate)
 */
void Heading_control(float psiTargetRad, float psiNowRad, float gyroCcwDps, int basePwm)
{
  float e = wrapPi(psiTargetRad - psiNowRad) * 57.29578f;
  headErrDeg = e;
  float u = KP_HEAD * e - KD_HEAD * gyroCcwDps;
  if (fabsf(e) < HEAD_DEADZONE_DEG && fabsf(gyroCcwDps) < 10.0f)
  {
    u = 0;
  }
  else if (basePwm <= 20 && fabsf(u) < TURN_MIN_PWM && fabsf(e) >= HEAD_DEADZONE_DEG)
  {
    u = (e > 0) ? TURN_MIN_PWM : -TURN_MIN_PWM;   // overcome dead band when pivoting
  }
  u = constrain(u, -HEAD_U_MAX, HEAD_U_MAX);
  int left = (int)(basePwm - u);
  int right = (int)(basePwm + u);
  left = constrain(left, -REVERSE_MAX_PWM - 20, 255);
  right = constrain(right, -REVERSE_MAX_PWM - 20, 255);
  Control_setWheels(left, right);
}

float Heading_lastErrorDeg(void) { return headErrDeg; }

/* ---------------- ultrasonic ---------------- */
static int ultraMM = 9999;
static unsigned long lastPing = 0;

void Ultra_init(void)
{
#if USE_ULTRASONIC
  pinMode(ULTRA_TRIG_PIN, OUTPUT);
  pinMode(ULTRA_ECHO_PIN, INPUT);
#endif
}

void Ultra_update(unsigned long now_ms)
{
#if USE_ULTRASONIC
  if (now_ms - lastPing < 60) return;
  lastPing = now_ms;
  digitalWrite(ULTRA_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRA_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRA_TRIG_PIN, LOW);
  unsigned long us = pulseIn(ULTRA_ECHO_PIN, HIGH, 3500UL);   // ~60 cm max
  ultraMM = (us == 0) ? 9999 : (int)(us * 0.1715f);
#endif
}

int Ultra_distanceMM(void) { return ultraMM; }

/* ---------------- on-board start key (pin 2, active low) ---------------- */
void Key_init(void)
{
#if USE_START_KEY
  pinMode(START_KEY_PIN, INPUT_PULLUP);
#endif
}

bool Key_pressed(void)
{
#if USE_START_KEY
  static bool raw = HIGH, stable = HIGH;
  static unsigned long changedAt = 0;
  bool k = digitalRead(START_KEY_PIN);
  if (k != raw) { raw = k; changedAt = millis(); }
  if (millis() - changedAt > 40 && stable != raw)
  {
    stable = raw;
    if (stable == LOW) return true;
  }
#endif
  return false;
}
