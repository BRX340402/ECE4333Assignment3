#pragma once
// No right-wheel PWM compensation.
static const int FORWARD_ESTOP_CM=12;
// Automatically latch if a valid forward ultrasonic measurement is this close.
// There is NO new automatic stop if the sensor is unavailable (-1); line recovery
// already avoids forward repositioning when distance is unknown.
