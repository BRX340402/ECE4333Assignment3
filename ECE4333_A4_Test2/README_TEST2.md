# ECE 4333 — Assignment 4 Test 2

**Project folder:** `ECE4333_A4_Test2`  
**Source branch:** `assignment4-test2`  
**Status:** **EXPERIMENTAL**. Code has not been shown to compile on Arduino Uno or run safely on the physical car. Do not assume functional based on GitHub upload.

## Scope and provenance

- **Navigation, traffic, map and IMU foundation:** copy of the team's physically tested `ECE4333_A3_Test11_Follow`, with the UNO sketch renamed `UNO_Test2.ino` and the ESP32 sketch renamed `ESP32_Test2.ino`. Maintains UNO/ESP32 communication version **UV=41**.
- **New avoidance algorithm:** `UNO_Test2/A4Bug2.h` written anew to combine the student's idea (servo aims the ultrasonic sensor toward obstacle boundaries; measured changes decide detour phases) with the instructor's ECE 4333 Lecture 7 **Bug 2** principle of leaving the reference line at an obstacle and rejoining the line after passing it.
- **Not a classical mathematically complete Bug 2 algorithm:** a practical *Bug 2-inspired*, scan–step–verify rectangular detour, using gyro heading hold and infrared line recovery. Does not trace arbitrary obstacle contours continuously.
- **NOT based on the older Test13 / Assignment4test1 Overtake.h.** No preset obstacle width or length controls determine the maneuver.
- **Mode 7 vision target following removed from UNO** through `FOLLOW_ENABLED=0`. STOP-sign FOMO and red/green colour detection remain on ESP32. Face detection and FastLED are not in this firmware.

## Code folders

- `UNO_Test2/UNO_Test2.ino`: UNO main application, motor driver, line sensors, IMU, ultrasonic measurement, servo scan ownership, traffic, mapping, log and new algorithm integration.
- `UNO_Test2/A4Bug2.h`: obstacle states and transition conditions.
- `UNO_Test2/MapConfig.h`: `A4_TEST2_ENABLED` (1 by default), `FOLLOW_ENABLED=0`, line and motion control settings. Preserves **PIVOT_PWM=100** from the successful Test 11.
- `ESP32_Test2/ESP32_Test2.ino`: matching ESP32 camera/traffic/wireless program copied from Test 11. The ESP32 does not decide obstacle movements.
- `Arduino_Libraries/`: model and servo library archives; install required compatible libraries per your existing Test 11 setup.
- `.github/workflows/a4-test2-uno.yml`: Arduino UNO compile check, where GitHub Actions is enabled.

## Avoidance stages — no fixed obstacle path

1. TRACK: follow black line, while reading forward ultrasound. Confirm obstacle with three fresh readings within 20 cm.
2. WAIT: do not begin a detour too close to the route's estimated intersection or corner. Wait for clearing; then proceed on the line, or stop on timeout.
3. TURN_OUT: use gyro to pivot 90 degrees toward the outside selected from reference route heading. **Verify that route-based outside side is physically correct on the actual track**. Low torque from old robot builds prompted retaining minimum pivot PWM 100.
4. SIDE_SCAN: while stopped, turn the **shared camera/sonar servo** inward (nominally 30/150 degrees). Observe the original obstacle. Take repeated measurements. Three reliable far echoes, or five repeated no-echo observations *after first seeing the object*, are candidates that its side edge has been crossed.
5. SIDE_FRONT / SIDE_STEP: point sonar forward; check space; move 3 cm sideways; stop and scan inward again. Each step is a sensor-controlled decision. A repeated no-echo result is treated only as tentative clearance and the robot must remeasure after each short step.
6. SIDE_MARGIN: after finding the side edge, move the distance required by robot half-width plus safety allowance. This is a body-clearance rule, not a guessed obstacle width.
7. TURN_ALONG / PASS_SCAN / PASS_FRONT / PASS_STEP: rotate back to the route heading and monitor the obstacle sideways during repeated short forward advances. The obstacle's end is detected from repeat observations, not a preprogrammed object length.
8. REAR_MARGIN: advance from the observed rear edge by sonar-to-rear-body offset plus rear safety margin.
9. TURN_IN / RETURN_STEP: pivot 90 degrees toward the black line and approach it. The estimated sideways displacement caps the return search but **does not substitute for real infrared tape detection**.
10. TURN_JOIN: rotate to the original route heading. Require a stable central black-line reading and gyro alignment before returning control to the old line follower. Correct the map segment progress so sidestepping is not counted as route length.

**Safety limits** in `A4Bug2.h`, for example `MAX_SIDE_CM=32` and `MAX_PASS_CM=70`, are failure guards; they are **not** programmed travel targets. Reaching them without detecting the obstacle edge results in a fault and motor stop.

## Important adjustable chassis measurements

These are initial **unverified estimates**, not facts from a robot datasheet:

```cpp
static constexpr float ROBOT_WIDTH_CM=15.0f;
static constexpr float SIDE_SAFETY_CM=5.0f;
static constexpr float SENSOR_TO_REAR_CM=22.0f;
static constexpr float REAR_SAFETY_CM=5.0f;
static constexpr float STEP_CM=3.0f;
static constexpr int PAN_LEFT=30, PAN_CENTER=90, PAN_RIGHT=150;
```

Measure chassis width, rear overhang relative to ultrasonic sensor, and sensor pan direction. The actual pan angles are mounting-dependent. NEVER reverse the wheel turn direction merely to compensate for the pan servo orientation; calibrate them separately.

## Diagnostics

UNO status message `N221` now adds an `"A4"` integer with states:

| A4 code | Meaning |
|---:|---|
| 0 | TRACK |
| 1 | WAIT |
| 2 | TURN_OUT |
| 3 | SIDE_SCAN |
| 4 | SIDE_FRONT |
| 5 | SIDE_STEP |
| 6 | SIDE_MARGIN |
| 7 | TURN_ALONG |
| 8 | PASS_SCAN |
| 9 | PASS_FRONT |
| 10 | PASS_STEP |
| 11 | REAR_MARGIN |
| 12 | TURN_IN |
| 13 | RETURN_STEP |
| 14 | TURN_JOIN |
| 15 | FAULT |

The Test 11 ESP32 code may show the raw `N221` line rather than a dedicated A4 widget. Do not assume it is displayed on the existing map webpage.

## Verification and safety sequence

1. Preserve the working Test 11 firmware and any A4 Test1 firmware separately; this new branch does not overwrite either.
2. Run GitHub Actions / Arduino IDE Verify with board **Arduino Uno**. Save actual Flash and static SRAM totals. Firmware may exceed Flash: removing Mode 7 saves memory but the new state machine adds code. **Do not upload if Verify fails.**
3. Verify UNO reports `UV=41` when ESP32 Test2 is installed (both built from this directory). Do not pair with A4test1 UV=43.
4. Motors disconnected/wheels raised: manually pan servo to 30°, 90°, 150°, verify physical left/centre/right. Ultrasonic is an HC-SR04-style sonar, **not LiDAR**.
5. Secure the robot with the wheels off the floor and verify gyroscope sign and PWM-100 turning. Do not test on the Figure 8 yet.
6. Confirm forward sonar echo readings and timeout behavior, including no-echo. Missing echo may mean nothing is present, or simply an unreflective object: there is no collision-free guarantee from one sonar.
7. Start with slow movement and one stationary box in an empty, supervised area with an accessible physical power switch. Check A4 states and actual distance against the PWM-based estimator.
8. Verify recovery onto the correct black line and correct direction. Test outside-of-track side selection independently at each permissible route segment.
9. Test STOP sign on both sides, red/green traffic behaviour and full Figure 8 **separately**, then test in combination. The original right-side STOP sign issue is **not fixed** by this avoidance change.
10. Only after acceptable physical tests attempt the full Assignment 4 demonstration. A single servo looks sideward during a detour so forward monitoring and STOP sign vision can be temporarily impaired. Keep people away from its path.

## Unverified / known engineering limitations

- The build was not compiled in the authoring environment, and no robot was available for a physical test.
- No wheel encoders. All distances are estimated from motor PWM/time and can be wrong due to slip or battery voltage.
- The same servo controls camera AND ultrasonic orientation; the camera can miss signs while looking sideways.
- One ultrasonic sensor cannot continuously measure ahead and to the side simultaneously. Short stopped scans reduce but do not eliminate this limitation.
- No-echo clearance is tentative, not positive proof. Geometry and beam width may cause an apparent edge before the full chassis has passed it.
- Route-side inference based on the next corner is a heuristic that must be compared with the instructor's physical outside-track rule.
- No continuous collision-free guarantee or implementation of general moving-obstacle overtaking.
