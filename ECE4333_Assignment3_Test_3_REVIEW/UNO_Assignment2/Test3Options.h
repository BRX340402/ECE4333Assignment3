#pragma once
// Assignment 3 Test 3 — conservative hardware-trial parameters.
// RIGHT_PWM_TRIM=8 adds 8 PWM counts to the RIGHT wheel only when it is commanded
// to move (in either direction), without changing the 50 PWM nominal cruise.
// If it starts veering the opposite way, set this to 0 and upload the UNO again.
static const uint8_t RIGHT_PWM_TRIM=8;
// Web E-stop is non-safety-rated. This is a low-speed obstacle demonstration.
static const int FORWARD_ESTOP_CM=12;
// Automatically latch if a valid forward ultrasonic measurement is this close.
// There is NO new automatic stop if the sensor is unavailable (-1); line recovery
// already avoids forward repositioning when distance is unknown.
