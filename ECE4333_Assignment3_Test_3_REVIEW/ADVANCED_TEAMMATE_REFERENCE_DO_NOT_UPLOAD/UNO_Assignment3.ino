/*
 * ECE 4333 Assignment 3 - Differential steering robot: figure-8 tracking,
 * kinematic prediction + gyro + camera fusion, traffic-sign management,
 * state machine, ESP32 protocol and vision-based target following.
 *
 * Built on Elegoo Smart Robot Car V4 "Demo2" drivers (TB6612 motors, ITR20001
 * line sensors, MPU6050).  Board: Arduino Uno.  See README.md.
 *
 * File map
 *   Config.h      all tunable parameters
 *   Estimator.*   kinematic model, gyro heading, fusion, camera landmark correction
 *   Control.*     motors, line PD, gyro heading PD, ultrasonic
 *   Protocol.*    ESP32 <-> Arduino serial protocol
 *   Logger.*      non-blocking CSV/event log
 *   this file     state machine + scheduler
 */
#include <string.h>
#include <math.h>
#include <avr/pgmspace.h>
#include "Config.h"
#include "DeviceDriverSet_xxx0.h"
#include "MPU6050_getdata.h"
#include "Estimator.h"
#include "Control.h"
#include "Protocol.h"
#include "Logger.h"

#define DEG2RAD_F 0.017453292f
#define RAD2DEG_F 57.29578f

/* ------------------------------------------------------------------ */
/* States                                                              */
/* ------------------------------------------------------------------ */
enum
{
  S_INIT = 0,
  S_WAIT_GREEN,
  S_LINE_FOLLOW,
  S_APPROACH_INTERSECTION,
  S_STOP_CLASSIFY,
  S_STOP_SIGN_HOLD,
  S_RED_LIGHT_WAIT,
  S_CROSS_INTERSECTION,
  S_FINISHED,
  S_EMERGENCY,
  S_FOLLOW_WAIT,
  S_FOLLOW_TRACK,
  S_FOLLOW_SEARCH,
  S_COUNT
};
enum { R_NONE = 0, R_LIFTED, R_OBSTACLE, R_LINE_LOST, R_DEVIATION, R_GEOFENCE, R_COUNT };

const char sn0[] PROGMEM = "INIT";
const char sn1[] PROGMEM = "WAIT_GREEN";
const char sn2[] PROGMEM = "LINE_FOLLOW";
const char sn3[] PROGMEM = "APPROACH_INTERSECTION";
const char sn4[] PROGMEM = "STOP_CLASSIFY";
const char sn5[] PROGMEM = "STOP_SIGN_HOLD";
const char sn6[] PROGMEM = "RED_LIGHT_WAIT";
const char sn7[] PROGMEM = "CROSS_INTERSECTION";
const char sn8[] PROGMEM = "FINISHED";
const char sn9[] PROGMEM = "EMERGENCY";
const char sn10[] PROGMEM = "FOLLOW_WAIT";
const char sn11[] PROGMEM = "FOLLOW_TRACK";
const char sn12[] PROGMEM = "FOLLOW_SEARCH";
const char *const STATE_NAMES[] PROGMEM = {sn0, sn1, sn2, sn3, sn4, sn5, sn6, sn7, sn8, sn9, sn10, sn11, sn12};

const char rn0[] PROGMEM = "NONE";
const char rn1[] PROGMEM = "LIFTED";
const char rn2[] PROGMEM = "OBSTACLE";
const char rn3[] PROGMEM = "LINE_LOST";
const char rn4[] PROGMEM = "DEVIATION";
const char rn5[] PROGMEM = "GEOFENCE";
const char *const REASON_NAMES[] PROGMEM = {rn0, rn1, rn2, rn3, rn4, rn5};

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */
static uint8_t mode = DEFAULT_MODE;
static uint8_t state = S_INIT, prevState = S_INIT;
static unsigned long stateEntryMs = 0;

static LineSense line;
static float gyroDps = 0;      // CCW positive, static bias removed
static float gDt = 0.01f;
static unsigned long nowMs = 0;

// per-tick view of the camera
static bool tickNewFrame = false;
static Detection tickDet;

// last sightings
static unsigned long lastGreenMs = 0;
static unsigned long lastRedMs = 0;
static uint8_t lastRedCls = CLS_NONE;
static int lastRedD = 0;
static float lastRedOdo = 0;

// run bookkeeping
static unsigned long runStartMs = 0;
static float runOdo0 = 0;
static uint8_t crossings = 0;
static float cooldownEndOdo = 0;

// landmarks / map
struct Landmark { bool mapped; float x, y; };
static Landmark stopSign[2];
static bool lightMapped = false;
static bool interKnown = INTERSECTION_KNOWN;
static float interX = INTERSECTION_X_MM, interY = INTERSECTION_Y_MM;

// approach / stop / cross
static bool appRedSeen = false;
static float appOdo0 = 0;
static uint8_t camCorr = 0;
static float crossHeading = 0, crossStartOdo = 0, crossCenterOdo = 0;
static bool crossCenterSeen = false;

// monitors
static unsigned long blackSinceMs = 0, noLineSinceMs = 0, satSinceMs = 0, liftSinceMs = 0, obstSinceMs = 0;
static unsigned long obstClearMs = 0;
static bool blackConfirmed = false;
static uint8_t emReason = R_NONE;

// follow mode
static float psiTarget = 0, tgtD0 = 0, tgtOdo0 = 0, searchSwept = 0;
static unsigned long lastTargetMs = 0;

// heading history for latency compensation (50 ms steps)
#define HIST_N 24
static float hist[HIST_N];
static uint8_t histHead = 0;
static unsigned long histLastMs = 0;

static unsigned long lastLogMs = 0, lastStatusMs = 0, tickUs = 0;

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */
static int deg10(float rad) { return (int)lroundf(rad * RAD2DEG_F * 10.0f); }

static void copyName(const char *const *table, uint8_t i, char *out)
{
  strcpy_P(out, (PGM_P)pgm_read_word(&table[i]));
}

static void histPush(void)
{
  if (nowMs - histLastMs >= 50)
  {
    histLastMs = nowMs;
    hist[histHead] = est.psif;
    histHead = (histHead + 1) % HIST_N;
  }
}

static float histGet(unsigned long age_ms)
{
  unsigned long steps = age_ms / 50;
  if (steps > HIST_N - 1) steps = HIST_N - 1;
  return hist[(histHead + HIST_N - 1 - steps) % HIST_N];
}

static void logRecord(char tag, int id)
{
#if LOG_ENABLE
  int cl, cr;
  Control_getLastCmd(&cl, &cr);
  Log_printf_P(PSTR("%c,%d,%lu,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%d,%d,%ld\n"),
               tag, id, nowMs, mode, state,
               (int)est.xk, (int)est.yk, deg10(est.psik), deg10(est.psig),
               (int)est.xf, (int)est.yf, deg10(est.psif),
               cl, cr, (int)(line.pos * 100.0f),
               proto.det.cls, proto.det.dist_mm, (int)(proto.det.bearing_deg * 10.0f), (long)est.odo);
#endif
}

static void enterState(uint8_t s)
{
  prevState = state;
  state = s;
  stateEntryMs = nowMs;
#if LOG_ENABLE
  char nm[24], pv[24];
  copyName(STATE_NAMES, s, nm);
  copyName(STATE_NAMES, prevState, pv);
  Log_printf_P(PSTR("E,%lu,%u,%s,%s\n"), nowMs, mode, pv, nm);   // state transition
#endif
  int cl, cr;
  Control_getLastCmd(&cl, &cr);
  Proto_sendStatus(mode, s, (abs(cl) + abs(cr)) / 2);
}

static void resetMonitors(void)
{
  blackSinceMs = noLineSinceMs = satSinceMs = liftSinceMs = obstSinceMs = obstClearMs = 0;
  Line_reset();
}

// follow mode: the A2 firmware only reports stop signs, so a stop sign also counts as the target
static bool isTarget(const Detection &d) { return d.cls == CLS_TARGET || (mode == MODE_FOLLOW && d.cls == CLS_STOP_SIGN); }
static bool redClass(uint8_t c) { return c == CLS_RED || c == CLS_STOP_SIGN || c == CLS_RED_LIGHT; }
static bool redVisible(void) { return lastRedMs != 0 && (nowMs - lastRedMs) < (unsigned long)DET_STALE_MS; }
static bool lapsDone(void) { return crossings >= LAPS_TARGET * 2; }
static bool inCooldown(void) { return est.odo < cooldownEndOdo; }
static float distToOrigin(void) { return sqrtf(est.xf * est.xf + est.yf * est.yf); }

// predicted distance to the red object, dead-reckoned forward from the last frame
static float redDistNow(void)
{
  if (lastRedD <= 0) return 0.0f;   // unknown -> treat as "already there"
  float d = (float)lastRedD - (est.odo - lastRedOdo);
  return d < 0 ? 0 : d;
}

static bool nearIntersectionPose(void)
{
  if (!interKnown) return false;
  float dx = interX - est.xf, dy = interY - est.yf;
  float d = sqrtf(dx * dx + dy * dy);
  if (d > INTERSECTION_APPROACH_R_MM) return false;
  return (dx * cosf(est.psif) + dy * sinf(est.psif)) > 0.3f * d;   // centre must be ahead
}

static void driveLine(int basePwm)
{
  if (line.valid) Line_control(line, gDt, gyroDps, basePwm);
  else Line_searchLost();
}

/* ------------------------------------------------------------------ */
/* Emergency stop                                                      */
/* ------------------------------------------------------------------ */
static bool triggerEmergency(uint8_t reason)
{
  emReason = reason;
  Control_stop(true);
#if LOG_ENABLE
  char rn[16];
  copyName(REASON_NAMES, reason, rn);
  Log_printf_P(PSTR("X,%lu,%s,%d,%d\n"), nowMs, rn, (int)est.xf, (int)est.yf);
#endif
  obstClearMs = 0;
  enterState(S_EMERGENCY);
  return true;
}

// checkLine: also watch for line loss / deviation. Returns true if it tripped.
static bool checkEmergency(bool checkLine)
{
  // robot lifted off the ground
  if (line.lifted)
  {
    if (liftSinceMs == 0) liftSinceMs = nowMs;
    else if (nowMs - liftSinceMs > LIFT_TIMEOUT_MS) return triggerEmergency(R_LIFTED);
  }
  else liftSinceMs = 0;

  // obstacle in front (ultrasonic)
#if USE_ULTRASONIC
  if (Ultra_distanceMM() < OBSTACLE_MM)
  {
    if (obstSinceMs == 0) obstSinceMs = nowMs;
    else if (nowMs - obstSinceMs > 100) return triggerEmergency(R_OBSTACLE);
  }
  else obstSinceMs = 0;
#endif

  if (checkLine && !line.lifted)
  {
    if (!line.valid)
    {
      if (noLineSinceMs == 0) noLineSinceMs = nowMs;
      else if (nowMs - noLineSinceMs > LINE_LOST_TIMEOUT_MS) return triggerEmergency(R_LINE_LOST);
    }
    else noLineSinceMs = 0;

    if (line.valid && fabsf(line.pos) > SAT_POS)
    {
      if (satSinceMs == 0) satSinceMs = nowMs;
      else if (nowMs - satSinceMs > SAT_TIMEOUT_MS) return triggerEmergency(R_DEVIATION);
    }
    else satSinceMs = 0;
  }

#if GEOFENCE_ENABLE
  if (est.xf < GEOFENCE_MIN_X_MM || est.xf > GEOFENCE_MAX_X_MM || est.yf < GEOFENCE_MIN_Y_MM || est.yf > GEOFENCE_MAX_Y_MM)
    return triggerEmergency(R_GEOFENCE);
#endif
  return false;
}

/* ------------------------------------------------------------------ */
/* Figure-8 helpers                                                    */
/* ------------------------------------------------------------------ */
static void startRun(void)
{
  Est_reset(0, 0, INITIAL_HEADING_DEG * DEG2RAD_F);
  crossings = 0;
  runStartMs = nowMs;
  runOdo0 = 0;
  cooldownEndOdo = COOLDOWN_DIST_MM;      // ignore the start light for the first stretch
  lastRedMs = 0;
  stopSign[0].mapped = stopSign[1].mapped = false;
  interKnown = INTERSECTION_KNOWN;
  interX = INTERSECTION_X_MM;
  interY = INTERSECTION_Y_MM;
  resetMonitors();
}

static void finishRun(void)
{
  Control_stop(false);
  float t = (float)(nowMs - runStartMs) / 1000.0f;
  float dist = est.odo - runOdo0;
#if LOG_ENABLE
  Log_printf_P(PSTR("R,%lu,%d,%d,%u\n"), nowMs - runStartMs, (int)dist, (int)(t > 0 ? dist / t : 0), crossings);
#endif
  enterState(S_FINISHED);
}

static void beginApproach(bool red)
{
  appRedSeen = red;
  appOdo0 = est.odo;
  enterState(S_APPROACH_INTERSECTION);
}

static void beginCross(void)
{
  crossings++;
  crossHeading = est.psif;
  crossStartOdo = est.odo;
  crossCenterSeen = false;
  lastRedMs = 0;                         // forget the sign we just handled
  enterState(S_CROSS_INTERSECTION);
}

// Called when the all-black crossing is under the sensors (= intersection centre).
static void learnIntersection(void)
{
  if (!interKnown)
  {
    interX = est.xf;
    interY = est.yf;
    interKnown = true;
  }
  else
  {
    Est_pullPosition(interX, interY, INTERSECT_K_POS);   // loop closure
  }
#if LOG_ENABLE
  Log_printf_P(PSTR("I,%lu,%d,%d,%d,%d\n"), nowMs, (int)interX, (int)interY, (int)est.xf, (int)est.yf);
#endif
}

// Stop-sign range/bearing: map it on first sight, correct the pose on later sightings.
static void signUpdate(const Detection &d)
{
  uint8_t idx = crossings & 1;
  float beta = d.bearing_deg * DEG2RAD_F;
  if (!stopSign[idx].mapped)
  {
    Est_landmarkFromCamera((float)d.dist_mm, beta, &stopSign[idx].x, &stopSign[idx].y);
    stopSign[idx].mapped = true;
    camCorr++;
#if LOG_ENABLE
    Log_printf_P(PSTR("S,%lu,%u,%d,%d\n"), nowMs, d.cls, (int)stopSign[idx].x, (int)stopSign[idx].y);
#endif
  }
  else
  {
    bool ok = Est_cameraCorrect(stopSign[idx].x, stopSign[idx].y, (float)d.dist_mm, beta);
    camCorr++;
#if LOG_ENABLE
    Log_printf_P(PSTR("C,%lu,%u,%d,%d,%d,%d\n"), nowMs, idx, ok ? 1 : 0, (int)est.xf, (int)est.yf, deg10(est.psif));
#endif
  }
}

/* ------------------------------------------------------------------ */
/* Follow-mode outer loop (camera, ~1-2 Hz)                            */
/* ------------------------------------------------------------------ */
static void outerLoopUpdate(const Detection &d)
{
  float beta = d.bearing_deg * DEG2RAD_F;
  float psiCap = histGet(CAM_LATENCY_MS);        // heading when the picture was taken
  psiTarget = wrapPi(psiCap + beta);             // set point for the gyro inner loop
  tgtD0 = d.distKnown ? (float)d.dist_mm : 0.0f;
  tgtOdo0 = est.odo;
  lastTargetMs = nowMs;
#if LOG_ENABLE
  if (d.distKnown && d.dist_mm > 0)
  {
    float xs = est.xf + CAM_FWD_OFFSET_MM * cosf(psiCap) + (float)d.dist_mm * cosf(psiTarget);
    float ys = est.yf + CAM_FWD_OFFSET_MM * sinf(psiCap) + (float)d.dist_mm * sinf(psiTarget);
    Log_printf_P(PSTR("T,%lu,%d,%d,%d,%d\n"), nowMs, (int)xs, (int)ys, deg10(psiTarget), d.dist_mm);
  }
#endif
}

/* ------------------------------------------------------------------ */
/* State machine                                                       */
/* ------------------------------------------------------------------ */
static void runStateMachine(void)
{
  switch (state)
  {
  /* ---------------- common ---------------- */
  case S_INIT:
    Control_stop(false);
    enterState(mode == MODE_FIGURE8 ? S_WAIT_GREEN : S_FOLLOW_WAIT);
    break;

  case S_EMERGENCY:
  {
    Control_stop(true);
    if (emReason == R_OBSTACLE)
    {
      // auto-resume once the obstacle has been gone for a while
      if (Ultra_distanceMM() >= OBSTACLE_MM + 30)
      {
        if (obstClearMs == 0) obstClearMs = nowMs;
        else if (nowMs - obstClearMs >= OBSTACLE_CLEAR_MS)
        {
          resetMonitors();
          enterState(prevState);
        }
      }
      else obstClearMs = 0;
    }
    if (proto.cmdResume)   // operator resume ("$R*") after any emergency
    {
      proto.cmdResume = false;
      resetMonitors();
      enterState(mode == MODE_FIGURE8 ? S_LINE_FOLLOW : S_FOLLOW_WAIT);
    }
    break;
  }

  case S_FINISHED:
    Control_stop(false);
    if (proto.cmdStart)
    {
      proto.cmdStart = false;
      enterState(mode == MODE_FIGURE8 ? S_WAIT_GREEN : S_FOLLOW_WAIT);
    }
    break;

  /* ---------------- figure-8 ---------------- */
  case S_WAIT_GREEN:
  {
    Control_stop(false);
    if (!lightMapped && tickNewFrame && tickDet.cls == CLS_GREEN && tickDet.distKnown && tickDet.dist_mm > 0)
    {
      float xs, ys;       // estimated traffic-light location in the inertial frame
      Est_landmarkFromCamera((float)tickDet.dist_mm, tickDet.bearing_deg * DEG2RAD_F, &xs, &ys);
      lightMapped = true;
#if LOG_ENABLE
      Log_printf_P(PSTR("S,%lu,%u,%d,%d\n"), nowMs, tickDet.cls, (int)xs, (int)ys);
#endif
    }
    bool green = lastGreenMs != 0 && (nowMs - lastGreenMs) < (unsigned long)DET_STALE_MS;
    if (green || proto.cmdStart || START_WITHOUT_LIGHT)
    {
      proto.cmdStart = false;
      startRun();
      enterState(S_LINE_FOLLOW);
    }
    break;
  }

  case S_LINE_FOLLOW:
  {
    if (checkEmergency(true)) break;
    bool cool = inCooldown();
    if (lapsDone() && distToOrigin() < FINISH_RADIUS_MM) { finishRun(); break; }
    if (!cool && redVisible() && (lastRedD <= 0 || redDistNow() <= STOP_APPROACH_DIST_MM)) { beginApproach(true); break; }
    if (!cool && nearIntersectionPose()) { beginApproach(false); break; }
    if (!cool && blackConfirmed) { beginCross(); break; }   // crossing seen without a stop sign
    driveLine(BASE_PWM);
    break;
  }

  case S_APPROACH_INTERSECTION:
  {
    if (checkEmergency(true)) break;
    if (redVisible() && !appRedSeen)
    {
      appRedSeen = true;
      appOdo0 = est.odo;
    }
    if (appRedSeen)
    {
      if (redDistNow() <= STOP_STANDOFF_MM)
      {
        Control_stop(false);
        enterState(S_STOP_CLASSIFY);
        break;
      }
    }
    else
    {
      if (blackConfirmed) { beginCross(); break; }
      if (!nearIntersectionPose() && (est.odo - appOdo0) > 150.0f) { enterState(S_LINE_FOLLOW); break; }
    }
    driveLine(APPROACH_PWM);
    break;
  }

  case S_STOP_CLASSIFY:    // stopped at a red object: wait for the AI result (~0.5 s)
  {
    Control_stop(false);
    bool fresh = redVisible();
    if (fresh && lastRedCls == CLS_RED_LIGHT) { enterState(S_RED_LIGHT_WAIT); break; }
    if ((fresh && lastRedCls == CLS_STOP_SIGN) || (nowMs - stateEntryMs) >= CLASSIFY_TIMEOUT_MS)
    {
      camCorr = 0;
      enterState(S_STOP_SIGN_HOLD);   // unclassified red is treated as a stop sign
    }
    break;
  }

  case S_STOP_SIGN_HOLD:
  {
    Control_stop(false);
    if (tickNewFrame && (tickDet.cls == CLS_STOP_SIGN || tickDet.cls == CLS_RED) && tickDet.distKnown && tickDet.dist_mm > 0 && camCorr < CAM_MAX_CORRECTIONS)
      signUpdate(tickDet);
    if (nowMs - stateEntryMs >= STOP_HOLD_MS) beginCross();
    break;
  }

  case S_RED_LIGHT_WAIT:
  {
    Control_stop(false);
    if (lapsDone()) { finishRun(); break; }     // red light at the finish = end of run
    if (lastGreenMs != 0 && lastGreenMs >= stateEntryMs)
    {
      cooldownEndOdo = est.odo + COOLDOWN_DIST_MM;
      Line_reset();
      enterState(S_LINE_FOLLOW);
    }
    break;
  }

  case S_CROSS_INTERSECTION:
  {
    if (checkEmergency(false)) break;
    if (!crossCenterSeen && blackConfirmed)
    {
      crossCenterSeen = true;
      crossCenterOdo = est.odo;
      learnIntersection();
    }
    bool done = (crossCenterSeen && (est.odo - crossCenterOdo) >= CROSS_AFTER_CENTER_MM) ||
                (est.odo - crossStartOdo) >= CROSS_MAX_MM;
    if (done)
    {
      cooldownEndOdo = est.odo + COOLDOWN_DIST_MM;
      Line_reset();
      enterState(S_LINE_FOLLOW);
      break;
    }
    Heading_control(crossHeading, est.psif, gyroDps, CROSS_PWM);   // straight across, gyro holds heading
    break;
  }

  /* ---------------- vision-based target following ---------------- */
  case S_FOLLOW_WAIT:
    Control_stop(false);
    if (tickNewFrame && isTarget(tickDet))
    {
      outerLoopUpdate(tickDet);
      enterState(S_FOLLOW_TRACK);
    }
    break;

  case S_FOLLOW_TRACK:
  {
    if (checkEmergency(false)) break;
    if (tickNewFrame && isTarget(tickDet)) outerLoopUpdate(tickDet);   // outer loop
    unsigned long age = nowMs - lastTargetMs;
    if (age > (unsigned long)TARGET_LOST_MS)
    {
      searchSwept = 0;
      psiTarget = est.psif;
      enterState(S_FOLLOW_SEARCH);
      break;
    }
    int pwm = 0;
    float eDeg = fabsf(wrapPi(psiTarget - est.psif)) * RAD2DEG_F;
    if (age <= (unsigned long)FRAME_HOLD_MS)
    {
      float distNow = tgtD0 > 0 ? tgtD0 - (est.odo - tgtOdo0) : 999.0f;   // dead-reckoned between frames
      float err = distNow - FOLLOW_DIST_MM;
      if (err > 0)
      {
        float p = FOLLOW_MIN_PWM + FOLLOW_KP_DIST * err;
        if (p > FOLLOW_MAX_PWM) p = FOLLOW_MAX_PWM;
        float f = 1.0f - eDeg / FOLLOW_TURN_ONLY_DEG;     // turn first, then drive
        if (f < 0) f = 0;
        pwm = (int)(p * f);
      }
    }
    Heading_control(psiTarget, est.psif, gyroDps, pwm);   // inner loop (gyro-fused heading)
    break;
  }

  case S_FOLLOW_SEARCH:
  {
    if (checkEmergency(false)) break;
    if (tickNewFrame && isTarget(tickDet))
    {
      outerLoopUpdate(tickDet);
      enterState(S_FOLLOW_TRACK);
      break;
    }
    psiTarget = wrapPi(psiTarget + SEARCH_RATE_DPS * DEG2RAD_F * gDt);
    searchSwept += SEARCH_RATE_DPS * gDt;
    if (searchSwept > SEARCH_MAX_DEG)
    {
      Control_stop(false);
      enterState(S_FOLLOW_WAIT);
      break;
    }
    Heading_control(psiTarget, est.psif, gyroDps, 0);
    break;
  }

  default:
    Control_stop(true);
    break;
  }
}

/* ------------------------------------------------------------------ */
/* Arduino entry points                                                */
/* ------------------------------------------------------------------ */
void setup()
{
  Serial.begin(SERIAL_BAUD);
  Log_init();
  Proto_init();
  Control_init();
  Ultra_init();
  Key_init();

  bool gyroFail = MPU6050Getdata.MPU6050_dveInit();
  delay(300);
  MPU6050Getdata.MPU6050_calibrateBias(GYRO_CAL_SAMPLES);   // keep the car still!
  Est_reset(0, 0, INITIAL_HEADING_DEG * DEG2RAD_F);
  for (uint8_t i = 0; i < HIST_N; i++) hist[i] = est.psif;
  memset(&tickDet, 0, sizeof(tickDet));

#if LOG_ENABLE
  Log_printf_P(PSTR("H,type,id,t_ms,mode,state,xk,yk,psik_d10,psig_d10,xf,yf,psif_d10,cmdL,cmdR,pos_x100,cls,dist_mm,bear_d10,odo_mm\n"));
  if (gyroFail) Log_printf_P(PSTR("X,0,GYRO_NOT_FOUND,0,0\n"));
#endif
  nowMs = millis();
  enterState(S_INIT);
  tickUs = micros();
}

void loop()
{
  Proto_poll();
  Log_pump();

  unsigned long nowUs = micros();
  if (nowUs - tickUs < (unsigned long)CONTROL_PERIOD_MS * 1000UL) return;
  gDt = (float)(nowUs - tickUs) / 1.0e6f;
  tickUs = nowUs;
  if (gDt > 0.05f) gDt = 0.05f;
  nowMs = millis();

  // ---- sense ----
  gyroDps = MPU6050Getdata.MPU6050_getYawRateDps();
  Line_read(line);
  Ultra_update(nowMs);

  // ---- estimate (uses the wheel commands applied during the last tick) ----
  int cl, cr;
  Control_getLastCmd(&cl, &cr);
  Est_update(gDt, cl, cr, gyroDps);
  histPush();

  // ---- camera frame bookkeeping ----
  tickNewFrame = proto.newDet;
  proto.newDet = false;
  if (tickNewFrame)
  {
    tickDet = proto.det;
    if (tickDet.cls == CLS_GREEN) lastGreenMs = nowMs;
    if (redClass(tickDet.cls))
    {
      bool vis = redVisible();
      lastRedMs = nowMs;
      if (!(vis && lastRedCls == CLS_STOP_SIGN && tickDet.cls == CLS_RED)) lastRedCls = tickDet.cls;  // keep the AI verdict
      if (tickDet.distKnown || !vis)       // an assumed distance must not keep restarting the countdown
      {
        lastRedD = tickDet.dist_mm;
        lastRedOdo = est.odo;
      }
    }
  }

  // ---- intersection (all-black) debounce ----
  if (line.allBlack)
  {
    if (blackSinceMs == 0) blackSinceMs = nowMs;
    blackConfirmed = (nowMs - blackSinceMs) >= INTERSECT_MS;
  }
  else
  {
    blackSinceMs = 0;
    blackConfirmed = false;
  }

  if (Key_pressed()) proto.cmdStart = true;      // on-board key = start

  // ---- commands from ESP32 / serial monitor ----
  if (proto.cmdMode >= 0)
  {
    mode = (proto.cmdMode == MODE_FOLLOW) ? MODE_FOLLOW : MODE_FIGURE8;
    proto.cmdMode = -1;
    Control_stop(false);
    resetMonitors();
    enterState(mode == MODE_FIGURE8 ? S_WAIT_GREEN : S_FOLLOW_WAIT);
  }
  if (proto.cmdCheckpoint >= 0)
  {
    logRecord('K', proto.cmdCheckpoint);
    proto.cmdCheckpoint = -1;
  }

  // ---- decide + act ----
  runStateMachine();

  // ---- telemetry ----
  if (nowMs - lastLogMs >= (unsigned long)LOG_PERIOD_MS)
  {
    lastLogMs = nowMs;
    logRecord('L', 0);
  }
  if (nowMs - lastStatusMs >= (unsigned long)STATUS_PERIOD_MS)
  {
    lastStatusMs = nowMs;
    Control_getLastCmd(&cl, &cr);
    Proto_sendStatus(mode, state, (abs(cl) + abs(cr)) / 2);
  }
}
