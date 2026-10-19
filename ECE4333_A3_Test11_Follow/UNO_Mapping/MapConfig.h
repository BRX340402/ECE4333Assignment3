#pragma once
// ===== Test11 mapping + target-following build: every tunable value is collected here =====
// UNO<->ESP32 serial link. Must equal UNO_LINK_BAUD in ESP32_Assignment2/Config.h.
// If communication fails, set both to 9600 and raise SAMPLE_MS to 150.
static const long LINK_BAUD = 19200;
static const int  FIRMWARE_UV = 41;        // N221 "UV"; ESP32/web page require 41

// Data logging
static const uint16_t SAMPLE_MS = 100;      // N230 sample period while driving (10 Hz)
static const uint16_t IDLE_SAMPLE_MS = 200; // N230 period while stopped (live gyro readout on the page)
static const uint16_t STATUS_MS = 1000;     // N221 (web page status) period
static const uint16_t SENSOR_MS = 2000;     // N210 (battery/ultrasonic) period

// Driving (values not set by the web page)
static const int  PIVOT_PWM = 100;          // in-place turn at corners/search
static const uint16_t CROSS_HOLD_MS = 400;  // hold heading this long across the 60-degree crossing
static const int  MAX_CORNER_DEG = 200;     // corner pivot limit before falling back to search
static const int  WIDE_SEARCH_DEG = 170;    // last-resort sweep amplitude
static const float NO_GYRO_DEG_PER_MS = 0.12f; // only used if gyro navigation is OFF/unavailable
static const int  FORWARD_ESTOP_CM = 12;    // 3 consecutive valid readings at/below this latch E-stop
static const int  CREEP_BLOCK_CM = 15;      // search will not creep forward if an obstacle is this close
// Fixed-route corner hint (LineFollower::routeHeading): start at C facing D. Set false
// for a different track or start point; corners then use the sensors only.
static const bool ROUTE_HINT = true;

// Pose model. Measured on the real car (Test10 run 2): corner C to corner D = 3.2 m in 20 s
// at PWM 34-50 -> about 0.33 cm/s per PWM. The map still fine-tunes the scale from complete straights.
static const float TRACK_WIDTH_CM = 14.0f;
static const float LEFT_CM_S_PER_PWM = 0.33f, RIGHT_CM_S_PER_PWM = 0.33f;
// Line sensors: hysteresis around the black threshold (ADC counts). A sensor riding the tape
// edge reads in between (Test10 run 2: left sensor 550-730 on a 600 threshold) and must not flicker.
static const int  LINE_HYST = 40;

// ===== Test11 additions =====
// Compile-time switches (set to 0 to hand flash back when the sketch gets tight)
#define FOLLOW_ENABLED 1     // vision target following (mode 7); 0 removes it (about 1 KB)
#define IMU_QMI8658 1        // also support the QMI8658 IMU; 0 = MPU6050/GY-521 only (about 0.4 KB)
#define PAN_SERVO 1          // centre the camera pan servo at start; 0 drops the Servo library (about 0.9 KB flash, ~60 B RAM) - the servo then stays where it is

// Target following. Defaults for what the web page sends with its Follow button.
static const int FOLLOW_STOP_W_PM = 250;   // stop when the target is this wide (per mille of the image width)
static const int FOLLOW_FOV_DEG   = 62;    // camera horizontal field of view; negative = image mirrored
static const int FOLLOW_KP10      = 15;    // inner-loop PD: PWM per degree x10
static const int FOLLOW_KD100     = 10;    // inner-loop PD: PWM per (deg/s) x100
static const uint16_t FOLLOW_OUTER_MS = 500;       // camera outer loop period (assignment: 0.5-1 s)
static const uint16_t FOLLOW_LOST_HOLD_MS = 1500;  // target not seen this long: stop
static const uint16_t FOLLOW_SWEEP_AFTER_MS = 3000;// ... and after this long start looking around
static const int FOLLOW_MIN_PWM = 40;      // lowest forward PWM that still moves the car
static const int FOLLOW_PIVOT_MIN_PWM = 50;// lowest in-place turn PWM that still turns the car
static const int FOLLOW_SWEEP_DPS = 45;    // search sweep speed
static const int FOLLOW_SWEEP_MAX_DEG = 400;
