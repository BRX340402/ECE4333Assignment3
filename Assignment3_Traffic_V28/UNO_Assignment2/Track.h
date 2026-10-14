#pragma once
// ============================================================================
// Figure-8 track plan (Assignment 3 figure).  Units: mm and 0.1 degree.
//
// INERTIAL FRAME: origin = start point (traffic light), +X = the robot's
// initial heading (along the long diagonal toward the upper-left corner),
// +Y = left of that heading, heading positive = counter-clockwise (left turn).
// The robot must be placed on the start point pointing along the first
// segment before Start is pressed.
//
// Corner names (see figure):  S start/light, UL upper-left, UR upper-right,
// X crossing, A first kink on the lower-left side, L lower-left corner,
// R lower-right corner.
//
// HOW THE NUMBERS WERE OBTAINED FROM THE FIGURE
//   upper loop  : X-UR = 1.2 m, X-UL = 1.7 m, angle at X = 60 deg
//                 -> top edge UL-UR = 1513 mm, angle UL = 43.4, UR = 76.6 deg
//   lower loop  : X-A = 1.0 m (down the same straight line as UR-X),
//                 X-R = 1.5 m, angle R = 60 deg, angle L = 90 deg (right-angle mark)
//                 -> kink A turns 30 deg, A-L = 433 mm, L-R = 1250 mm
//   The robot drives STRAIGHT through X both times (two straight lines cross),
//   so UR->X->A is one 2200 mm segment and R->X->UL is one 3200 mm segment.
//   Turn sum of the loop is exactly 0 (-136.6 -103.4 +30 +90 +120), so the
//   plan closes on itself.  Right turns in the upper loop, left turns in the
//   lower loop: the figure-8.
// The figure is hand-drawn and "approximate", so MEASURE your taped track and
// edit the numbers below (nothing else needs to change).
// ============================================================================
#include <stdint.h>
#if defined(__AVR__)
#include <avr/pgmspace.h>
#define A3_PGM PROGMEM
#define a3_rd16(p) ((int16_t)pgm_read_word(p))
#else
#define A3_PGM
#define a3_rd16(p) (*(p))
#endif

// Distance from the start point (traffic light) to the UL corner.  The figure
// puts the light about one third of the way from UL toward X on the diagonal.
static const int16_t START_TO_UL_MM = 600;

// Segment k = first rotate by PLAN_TURN10[k]/10 degrees (CCW +), then drive
// PLAN_MM[k] mm straight.  Segment 0 is only used on the first lap; the loop
// repeats from segment LOOP_FIRST_SEG to the last one.
static const int16_t PLAN_TURN10[] A3_PGM = {    0, -1366, -1034,  300,  900, 1200};
static const int16_t PLAN_MM[]     A3_PGM = {START_TO_UL_MM, 1513, 2200,  433, 1250, 3200};
static const uint8_t PLAN_SEGMENTS = 6;
static const uint8_t LOOP_FIRST_SEG = 1;
// Laps before the robot stops at the start point (0 = keep going until Stop).
static const uint8_t LAPS_TO_RUN = 2;

// ---- corner / sensor geometry --------------------------------------------
// Distance from the wheel axle (pose reference point) to the line sensors.
static const float SENSOR_AHEAD_CM = 7.0f;   // MEASURE on your car
// Line-end events only correct position inside a window around the corner:
// max(EVENT_WINDOW_CM, EVENT_WINDOW_FRAC * segment length).
static const float EVENT_WINDOW_CM = 30.0f;
static const float EVENT_WINDOW_FRAC = 0.12f;
// Speed-model scale learning from corner events (long segments only).
static const float SCALE_LEARN_GAIN = 0.3f;
static const float SCALE_MIN = 0.6f, SCALE_MAX = 1.6f;
static const float SCALE_MAX_STEP = 0.25f;    // ignore events whose implied scale error is bigger (false trigger)
static const uint16_t EVENT_MIN_MS = 40;     // line must stay lost this long
static const float EVENT_MIN_LINE_CM = 12.0f;// ...after being followed for at least this far (crossing X does not count)

// ---- controller tuning ----------------------------------------------------
static const float HOLD_KP = 2.0f;           // PWM per degree of heading error (straight)
static const float HOLD_KD = 0.15f;          // PWM per deg/s (damping, uses gyro rate)
static const float HOLD_UMAX = 40.0f;        // max wheel-speed difference correction
static const float TURN_KP = 1.4f;           // PWM per degree while pivoting
static const int   TURN_MIN_PWM = 55;        // below this the motors stall
static const int   TURN_MAX_PWM = 90;
static const float TURN_TOL_DEG = 2.5f;
static const float TURN_COAST_S = 0.08f;     // cut power this many seconds of rotation early
static const uint16_t TURN_SETTLE_MS = 80;
static const uint16_t TURN_TIMEOUT_MS = 5000;
static const float CORNER_LEAD_CM = 1.5f;    // start the pivot this early: the car coasts about this far
static const float CORNER_SLOW_CM = 10.0f;   // slow down this close to a corner
static const int   CORNER_PWM = 70;
static const bool  LINE_TRIM_ENABLED = true; // outer-loop heading trim from side sensors
static const float TRIM_MAX_DEG = 8.0f;
static const float TRIM_STEP_DEG = 0.15f;
static const bool  LINE_EVENT_ENABLED = true;// line-end = corner landmark (NOT a search)
static const float CROSS_CLEAR_CM = 30.0f;   // distance after a STOP release before TRACK resumes
