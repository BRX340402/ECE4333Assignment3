/*
 * Config.h - every tunable number for ECE 4333 Assignment 3 lives here.
 *
 * Conventions used everywhere in this sketch
 *   - Inertial frame: origin = start position (traffic light), +x = initial
 *     forward direction, +y = left.  Units: mm.
 *   - Heading psi: radians internally, counter-clockwise (CCW / left) positive,
 *     0 = +x axis.  Logged in tenths of a degree.
 *   - Camera bearing: CCW positive (target to the LEFT of image centre = +).
 *   - Wheel commands: signed PWM, -255..255, + = forward.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ===================== 1. Operating mode ===================== */
#define MODE_FIGURE8 0          // line tracking + signs + intersection + fusion
#define MODE_FOLLOW  1          // vision-based target following (nested loops)
#define DEFAULT_MODE MODE_FIGURE8   // can be changed at run time with "$M,0*" / "$M,1*"
#define START_WITHOUT_LIGHT 0   // 1 = bench testing: leave start without waiting for green
#define LAPS_TARGET 2           // figure-8 laps (1 lap = 2 intersection crossings)

/* ===================== 2. Serial / protocol ===================== */
#define SERIAL_BAUD 9600        // A2 ESP32 firmware uses 9600 on Serial2 (RX33/TX4)
#define PROTO_REQUIRE_CHECKSUM 0 // 1 = reject frames without "*HH"
#define DET_STALE_MS 1500       // detection older than this is ignored
#define LOG_ENABLE 1
#define LOG_PERIOD_MS 500       // 2 Hz log, same rate as the sample plots
#define STATUS_PERIOD_MS 1000   // Arduino -> ESP32 status packet
#define SEND_STATUS_TO_ESP 0    // 1 = also print "$S,..." lines (ESP32 A2 firmware ignores them)

/* ---- Adapter for the Assignment-2 ESP32 JSON protocol (N=200 stop sign, N=203 red heartbeat) ---- */
#define STOP_MIN_P            700    // min confidence (0..1000) to accept an N=200 stop-sign result
#define CAM_IMAGE_W_PX        400    // analysis image width the ESP32 reports X in (A2: IMAGE_W)
#define CAM_HFOV_DEG          66.0f  // horizontal field of view -> bearing from X. MEASURE (wide lens?)
#define ASSUMED_RED_DIST_MM   450    // used when the ESP32 gives no distance (R = -1) or only the
                                     // red heartbeat: set = real distance at which the red heartbeat
                                     // first turns on (measure once)
#define USE_START_KEY         1      // on-board key (pin 2) starts a run, like "$G"
#define START_KEY_PIN         2

#define CONTROL_PERIOD_MS 10    // 100 Hz inner loop

/* ===================== 3. Robot / kinematic model ===================== */
// Measure these on YOUR car (see README, "Calibration").
#define WHEEL_BASE_MM      135.0f   // L: distance between left and right wheel centres
#define WHEEL_RADIUS_MM    32.5f    // r
#define PWM_DEADBAND       30       // PWM below this does not move the wheel
#define MOTOR_W_PER_PWM_L  0.0667f  // wheel rad/s per PWM count above deadband (left)
#define MOTOR_W_PER_PWM_R  0.0667f  // (right)  -> max wheel speed ~ 0.0667*(255-30)=15 rad/s
#define MOTOR_TAU_S        0.10f    // first-order motor lag used by the model
#define INITIAL_HEADING_DEG 0.0f
#define CAM_FWD_OFFSET_MM  60.0f    // camera position ahead of the robot reference point

/* ===================== 4. Gyroscope + fusion ===================== */
#define GYRO_SIGN               1       // set to -1 if heading goes the wrong way
#define GYRO_CAL_SAMPLES        500     // ~1 s, robot must stand still at power-up
#define GYRO_BIAS_STILL_TAU_S   1.5f    // bias learning time constant while stopped
#define GYRO_BIAS_STRAIGHT_TAU_S 20.0f  // ...while driving ~straight (model says w ~ 0)
#define GYRO_BIAS_STRAIGHT_W_DPS 4.0f   // |model w| below this counts as "straight"
#define FUSION_MODEL_PULL_TAU_S 0.0f    // 0 = off. >0 pulls fused heading toward the
                                        // dead-reckoned heading (complementary filter).
#define CAM_K_POS               0.7f    // landmark correction gain, position (0..1)
#define CAM_K_HEAD              0.25f   // landmark correction gain, heading (0..1)
#define CAM_GATE_RANGE_MM       250.0f  // reject innovations bigger than this
#define CAM_GATE_BEARING_DEG    25.0f
#define CAM_MAX_CORRECTIONS     3       // per stop
#define CAM_LATENCY_MS          400     // capture->arrival delay of a detection (follow mode)
#define INTERSECT_K_POS         0.6f    // loop-closure gain at the intersection centre

/* ===================== 5. Line sensors (ITR20001) ===================== */
// Raw ADC: white floor low, black line mid, lifted-in-air very high (as in Demo2).
// Read Demo1 on your track and set these.
#define LINE_WHITE_RAW   100
#define LINE_BLACK_RAW   600
#define LIFT_RAW         950     // all three above this = car lifted off the ground
#define LINE_LOST_SUM    0.25f   // weight sum below this = line lost
#define INTERSECT_W      0.65f   // all three weights above this = crossing line
#define INTERSECT_MS     40      // ...for at least this long

/* ===================== 6. Line-tracking controller (PD) ===================== */
#define BASE_PWM         85      // cruising PWM on straights
#define MIN_BASE_PWM     45      // speed floor in tight corners
#define SLOW_GAIN        40.0f   // PWM removed per unit |line position|
#define KP_LINE          60.0f   // PWM per unit position error (pos in -1..1)
#define KD_LINE          3.0f    // PWM per (unit/s)
#define KG_LINE          0.08f   // PWM per deg/s: gyro yaw-rate damping
#define D_FILTER_ALPHA   0.35f   // derivative low-pass (0..1, higher = faster)
#define STEER_MAX        130.0f
#define REVERSE_MAX_PWM  70      // inner wheel may reverse this much (sharp 60 deg corners)
#define LOST_SEARCH_PWM  70      // pivot speed when the line is lost
#define APPROACH_PWM     60      // speed while approaching an intersection / sign

/* ===================== 7. Heading controller (gyro inner loop) ===================== */
#define KP_HEAD          1.6f    // PWM per degree of heading error
#define KD_HEAD          0.12f   // PWM per deg/s of yaw rate
#define HEAD_U_MAX       110.0f
#define HEAD_DEADZONE_DEG 2.0f
#define TURN_MIN_PWM     (PWM_DEADBAND + 12) // minimum pivot command that really moves

/* ===================== 8. Traffic light / stop sign / intersection ===================== */
#define STOP_APPROACH_DIST_MM 800    // red seen closer than this -> slow down
#define STOP_STANDOFF_MM      180    // stop when predicted distance to sign <= this
#define APPROACH_EXTRA_MM     150    // fallback: stop after d0 + this of travel
#define CLASSIFY_TIMEOUT_MS   1500   // wait for AI result (~0.5 s inference); then assume stop sign
#define STOP_HOLD_MS          2000   // full stop at a stop sign
#define COOLDOWN_DIST_MM      450    // ignore signs for this path length after leaving one
#define CROSS_PWM             80
#define CROSS_AFTER_CENTER_MM 90     // path after the all-black centre before line tracking resumes
#define CROSS_MAX_MM          330    // give up waiting for the centre
#define INTERSECTION_KNOWN    0      // 1 = use the coordinates below
#define INTERSECTION_X_MM     1000.0f
#define INTERSECTION_Y_MM     0.0f
#define INTERSECTION_APPROACH_R_MM 380.0f // slow down when the centre is this close and ahead
#define FINISH_RADIUS_MM      500.0f // laps done and this close to origin -> finish

/* ===================== 9. Emergency stop ===================== */
#define USE_ULTRASONIC       1       // Elegoo V4: TRIG 13, ECHO 12
#define ULTRA_TRIG_PIN       13
#define ULTRA_ECHO_PIN       12
#define OBSTACLE_MM          120
#define OBSTACLE_CLEAR_MS    800
#define LINE_LOST_TIMEOUT_MS 2500    // deviation: no line for this long
#define SAT_POS              0.95f   // deviation: |pos| above this ...
#define SAT_TIMEOUT_MS       2500    // ...for this long
#define LIFT_TIMEOUT_MS      200
#define GEOFENCE_ENABLE      0       // 1 = e-stop when the fused pose leaves the box below
#define GEOFENCE_MIN_X_MM   (-800.0f)
#define GEOFENCE_MAX_X_MM    3500.0f
#define GEOFENCE_MIN_Y_MM   (-1500.0f)
#define GEOFENCE_MAX_Y_MM    1500.0f

/* ===================== 10. Vision-based target following ===================== */
#define FOLLOW_DIST_MM       300     // keep this distance from the target (stop inside it)
#define FOLLOW_KP_DIST       0.15f   // PWM per mm of distance error
#define FOLLOW_MIN_PWM       55
#define FOLLOW_MAX_PWM       95
#define FOLLOW_TURN_ONLY_DEG 55.0f   // heading error at which forward speed drops to 0
#define FRAME_HOLD_MS        1200    // no new frame for this long -> stop translating
#define TARGET_LOST_MS       2500    // ...then search
#define SEARCH_RATE_DPS      25.0f   // search spin rate
#define SEARCH_MAX_DEG       380.0f

#endif
