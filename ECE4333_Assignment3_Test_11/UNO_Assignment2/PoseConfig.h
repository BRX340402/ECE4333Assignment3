#pragma once
// Measure these on YOUR car/track before enabling corrections. cm and radians.
static const bool POSE_CALIBRATED=false;
static const float TRACK_WIDTH_CM=14.0f; // placeholder effective left/right wheel spacing
// Rough nonzero trial estimates ONLY. Measure separately before claiming accuracy.
// Units: cm/s per signed PWM count; 50 PWM => ~3.5 cm/s assumed.
static const float LEFT_CM_S_PER_PWM=0.07f, RIGHT_CM_S_PER_PWM=0.07f;
static const float GYRO_SIGN=-1.0f; // ex2 convention; verify positive = left turn
static const bool LANDMARK_ENABLED=false;
static const float LANDMARK_X_CM=0,LANDMARK_Y_CM=0; // measured STOP sign centre in track frame
static const float CAMERA_FORWARD_CM=0; // camera offset ahead of robot reference point
static const float CAMERA_FUSION_GAIN=0.25f;
// One configured approach only. Select a different landmark/heading for the other figure-8 approach.
static const float APPROACH_HEADING_RAD=0;
