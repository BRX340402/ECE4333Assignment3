# Test13 - Assignment 4 build (overtaking, state machine log, camera pan at the intersection), based on Test11

Upload **both** sketches (UV is now 43): `UNO_Mapping/` (UNO, ESP32 switch OFF) and `ESP32_Assignment2/`. Target following (Assignment 3 part 5) is compiled OUT by default to make room (`FOLLOW_ENABLED 0` in `MapConfig.h`; the page still shows its panel but the UNO refuses it). The Test12 stop-sign distance correction is not in this build.

**Not tested on the car, not compiled for AVR/ESP32 here.** Run on a PC: the real UNO sketch (72 protocol checks incl. pan, overtaking trigger, no-room wait, failed-detour stop), the follow logic, the line-following simulations, an overtaking simulation on the figure 8, the page in headless Chromium against a mock car. Test plan: `A4_Test_Plan.pdf`.

## 1. Score-sheet items -> where they are
| Score sheet / assignment item | In this build |
|---|---|
| Bug algorithm, overtake on the OUTSIDE, resume figure-8 tracking | `Overtake.h`, Bug-2 style: obstacle seen by the ultrasonic -> 90 degree side-step to the outside of the next turn (left on C-O-D) -> drive alongside -> step back -> the line (m-line) is found again -> hand back to the line follower. The boundary is not followed with a sensor (the only range sensor looks forward): the pass is dead-reckoned with the gyro (PD) and odometer. |
| Traffic light, stop sign, obstacle detection, camera every 1-2 s, results to the Arduino | Colour / STOP-sign frames from the ESP32 as before. Obstacles: ultrasonic (the assignment allows "camera or range sensor"). **Camera-based obstacle recognition is not implemented.** |
| State machine for overtaking | Overtake states IDLE, WAIT, OUT_TURN, OUT_DRIVE, ALIGN, PASS, RET_TURN, RET_DRIVE, JOIN, FAIL; fault code 15 and a stop if a detour cannot be completed (never drives blind). |
| Intersection management | The map position (re-anchored at the crossing, see 3) tells when O is near: camera pans toward the STOP sign, speed is reduced, stop sign 3 s hold as before. |
| State machine + serial protocol | 16 state codes in every sample (`N230` field 19), start options in `N101` (D4-D11), overtaking score `OV` in `N221`; STOP command unchanged. |
| Overtaking time and estimated overtaking trajectory | Map summary lists each manoeuvre (duration, result, obstacle position); the path is highlighted on the map; heading chart. |
| Record and display states and transitions | Page: state timeline chart, transitions table, downloads "State transitions CSV" and "State timeline PNG". |
| Estimated vs actual position/heading, stop sign / light / obstacle locations in the inertial frame, completion time, average speed | Predicted / fused / mapped tracks with error against the drawing; stop sign, traffic light and obstacle markers (stop pose + the distance you enter next to the map, default 35 cm; light = start point unless a red light stop occurs); lap times and average speed in the summary. |
| Traffic light / stop sign / off-road violations | Self-check lines in the summary: STOP holds shorter than 3 s, line-search episodes longer than 3 s. Judged violations are the judge's call. |

## 2. Camera pan at the intersection (Drive tab, "Camera pan")
Camera turns to the angle you set only while the car is near O (distance before O is a setting), back to centre after it. Test buttons turn the camera now (find the angle where the STOP sign fills the image). While the camera is turned the ultrasonic is ignored (it points at the sign), and for 0.4 s after it is straight again. Optional slow-down near the intersection. Pass 1 (C to O) is the default because the signs in the drawing stand at the end of that leg; pass 2 (E to O) is selectable.

## 3. Corners and map auto-correction
- Simulation first: the stress run (388/400 laps) already picks the right direction almost always, and the "map overrides the sensors" ideas I tried (heading window for accepting a corner line, map direction at sharp corners) **did not improve it or made it worse**, so only these stayed: (a) **intersection landmark**: all three sensors black while heading along C-O-D or E-O-A re-anchors the segment and the progress to the known 150 cm (C to O) / 120 cm (E to O), which corrects odometer drift twice per lap and makes the pan / slow-down window accurate; (b) `ROUTE_STRONG` (map direction at sharp corners) as an option, default off (`LineFollower.h`).
- Diagnostics so the real failure can be found: the segment table now shows expected vs measured turn at every corner and flags WRONG WAY. If the car turns the wrong way, send "Download all lines" and the corner name.

## 4. Overtaking: what to expect (simulation, not the car)
- Needs about 125 cm of clear straight between the detection point and the next corner / the intersection window. Elsewhere the car **waits behind** the obstacle (state OVERTAKE: WAIT BEHIND) and continues when it is gone. On this track the usable place is the leg after the intersection (O to D); the first part of C to O only just fits.
- Simulation (gyro/odometer/tape errors, noise): static obstacle on O to D: 94 % overtaken and lap completed; big obstacle (12 cm radius) 94 %; obstacle only ~65 cm after the corner at C: poor (31 %), which is the "obstacle right after a corner" case this method does not handle; with no room: waited or stopped safely in 58 of 64 runs; contact with the obstacle in 1 of 144 runs. Moving robots are **not** reliably overtaken (the pass length must be tuned by hand).
- Tuning (Drive tab): detection distance, pass length (obstacle length + clearance; the car adds the measured obstacle distance), distance wheel axle to line sensors (the join step), side (auto / left / right). Side clearance is `outCm` (26 cm) in `Overtake.h`.

## 5. UNO memory (important)
Test10 README: flash 27 476 B (85 %), RAM 1 496 B (73 %). **No AVR compiler was available, these are estimates.** A PC compile used as a size proxy says Test13 as shipped is about 2 KB bigger than Test11 even with target following removed, so flash is probably **93-97 %**: it may not fit. RAM about 72 % (limit for the IDE warning is 75 %).
If the IDE says the sketch is too big or "low memory", change in `MapConfig.h`, in this order: `MANUAL_DRIVE 0` (about 0.4 KB; the web arrows stop working), `IMU_QMI8658 0` (about 0.4 KB; only if your gyro is an MPU6050/GY-521: the Map tab shows the chip, IM 1 = MPU6050, 2 = QMI8658), then `OVERTAKE_ENABLED 0` (about 2 KB) for runs that only need the pan and the state log. `PAN_SERVO 0` removes the Servo library but also the pan. `RING` 5 to 4 in the `.ino` saves 40 B RAM.

## 6. Changed vs Test11
UNO: new `Overtake.h`, `HeadingPD.steer()` shared by follow and overtake, `LineFollower` (landmark resync, `nearO`, `freeAhead`, `toCorner`, `outside`, `settled`, `resume`, fault 15), sketch (options, pan, slow-down, state codes, `OV`), `MapConfig.h` (switches). ESP32: `/traffic-start` passes the new options, version 43. Page and tools: Camera pan and Overtaking panels, state timeline, transitions table, turn check column, object markers. Removed from telemetry: `MQ` (unused) to keep `N221` inside the 256 B buffer.
Tests: `tests/ovt_sim.cpp` (overtaking on the figure 8), new protocol and page checks; `bash tests/run_tests.sh` runs everything (g++, python3, node).
