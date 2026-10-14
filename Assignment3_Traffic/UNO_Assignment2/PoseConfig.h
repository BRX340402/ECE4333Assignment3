#pragma once
// Measure these on YOUR car/track before enabling corrections. cm and radians.
static const bool POSE_CALIBRATED=false;
static const float TRACK_WIDTH_CM=14.0f; // placeholder effective left/right wheel spacing
static const float LEFT_CM_S_PER_PWM=0.0f, RIGHT_CM_S_PER_PWM=0.0f;
static const float GYRO_SIGN=-1.0f; // ex2 convention; verify positive = left turn
static const bool LANDMARK_ENABLED=false;
static const float LANDMARK_X_CM=0,LANDMARK_Y_CM=0; // measured STOP sign centre in track frame
static const float CAMERA_FORWARD_CM=0; // camera offset ahead of robot reference point
static const float CAMERA_FUSION_GAIN=0.25f;
// One configured approach only. Select a different landmark/heading for the other figure-8 approach.
static const float APPROACH_HEADING_RAD=0;
