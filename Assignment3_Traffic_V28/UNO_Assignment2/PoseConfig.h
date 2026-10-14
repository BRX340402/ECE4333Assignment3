#pragma once
// ============================================================================
// Kinematic model parameters.  The car has NO wheel encoders, so wheel speed
// is predicted from the PWM actually sent to the motors:
//     v_wheel = k * (|PWM| - PWM_DEADBAND) * sign(PWM)          [cm/s]
//     v = (vR+vL)/2 ,  omega = (vR-vL)/TRACK_WIDTH               (assignment model)
// These are PLACEHOLDERS - tune them (procedure in README_V28_EN.md):
//   k  = measured straight distance / (seconds * (PWM - PWM_DEADBAND))
//   W  = increase until the PREDICTED heading of a 360 deg pivot ends at 360.
// ============================================================================
static const float TRACK_WIDTH_CM = 14.0f;     // effective wheel spacing
static const int   PWM_DEADBAND = 40;
static const float LEFT_CM_S_PER_PWM = 0.45f;
static const float RIGHT_CM_S_PER_PWM = 0.45f;
// MPU6050 yaw sign: +1 if a LEFT (counter-clockwise) turn gives positive Z rate.
// The web "Gyro Z sign" setting overrides this at run time.
static const float GYRO_SIGN = -1.0f;
static const float GYRO_LSB_PER_DPS = 65.5f;   // +-500 deg/s full scale
// ---- camera landmark (STOP signs at the crossing) -------------------------
// Needs A3_FX_NORMALIZED in the ESP32 sketch and a measured sign position.
// Position below = crossing X (R + 150 cm along the diagonal) shifted 20 cm to
// the right of the approach path; approach = segment 5, heading 0.
static const bool LANDMARK_ENABLED = false;
static const float LANDMARK_X_CM = -110.0f, LANDMARK_Y_CM = -20.0f;
static const float CAMERA_FORWARD_CM = 0.0f;
static const float CAMERA_FUSION_GAIN = 0.25f;
static const float APPROACH_HEADING_RAD = 0.0f;
