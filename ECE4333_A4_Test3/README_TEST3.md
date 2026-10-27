# ECE 4333 Assignment 4 — Test 3: Obstacle Avoidance + Data Logging

**Branch:** `assignment4-test3`  
**Folder:** `ECE4333_A4_Test3`  
**Status:** Experimental. Host tests and Arduino compilation do not establish safe physical performance.

## Intent and source attribution

- **Preserved, unchanged:** `ECE4333_A3_Test11_Follow` (working Assignment 3 baseline), and `ECE4333_A4_Test2` (previous experiment).
- **Test 3 is a separate copy of Test 2**, including the Test 11 Figure 8 tracking, gyro pose estimator, red/green camera logic, STOP-sign FOMO and existing ESP32 dashboard.
- **Mapping/logging source:** `ECE4333_A3_Test11_Follow/PC_Mapping/map_logger.py`, `draw_map.py` and `track_reference.json`. The same N230 and N221 formats are retained.
- **New in Test 3:** nonblocking N231 obstacle avoidance transition events; Python CSV decoding and avoidance attempt metrics.
- **Bug 2 claim:** Bug 2-*inspired* scan/step/verify rectangular detour, not fully classical Bug 2 contour following. No obstacle dimensions or preprogrammed obstacle length determine when it changes phase.
- **Moving-robot overtaking is NOT active in the default code.** Test 3 is intended for a **single stationary obstacle first**, under supervision.

## Folder organization

| Folder | Role |
|---|---|
| `UNO_Test3/UNO_Test3.ino` | Arduino Uno: motion, IR sensors, ultrasonic panning, AI traffic integration, telemetry |
| `UNO_Test3/A4Bug2.h` | avoidance state machine (copied from Test 2 plus commented optional observation gate) |
| `UNO_Test3/A4EventLog.h` | 3-event fixed-memory FIFO for transitions |
| `UNO_Test3/MapConfig.h` | Arduino link settings, calibration constants, optional policy switch |
| `ESP32_Test3/ESP32_Test3.ino` | ESP32 camera, existing page, UART bridge and `/map-log` buffer |
| `PC_Mapping/map_logger.py` | records all UNO messages over Wi-Fi and exports CSV/JSONL |
| `PC_Mapping/draw_map.py` | retained Assignment 3 map generator |
| `PC_Mapping/draw_avoidance.py` | Test 3 PC-only SVG: estimated detour trajectory, events and obstacle point |
| `tests/` | host C++ and Python tests |
| `.github/workflows/a4-test3-uno.yml` | Test 3 compile / automated checks |

**Use the matching Test 3 Arduino + ESP32 sketches.** The UART still uses `19200` baud and reports protocol `UV=41`. The ESP32 must have the same STOP-sign model library as your tested Test 11 setup; GitHub Actions currently compiles **only Arduino Uno**, not the ESP32 ML firmware.

## Default obstacle avoidance

1. Follow the line and require three **fresh valid** ultrasonic readings of an obstacle within 20 cm.
2. If too close to the inferred Figure 8 crossing or end of segment, WAIT rather than begin an unsafe detour.
3. Otherwise pivot 90 degrees toward the supposed outside, using the gyro.
4. Scan toward the obstacle while stopped. Take short sensor-checked sideways steps, determine the detected edge, and add a *robot-body* clearance margin.
5. Rotate along the path, take repeated forward steps while scanning sideways, detect the end of the obstacle, and add measured rear-body clearance.
6. Return toward the line, confirm IR tape and heading, and hand over to Test 11 tracking.
7. Timeout, stale echo, gyro failure or an unsafe front reading must produce a fault and motor stop.

**Critical limitation:** Outside-of-track choice currently uses a route-heading heuristic; measure and check each segment on the real Figure 8. The actual robot width, sonar offset, servo left/right orientation, PWM speed and turn torque remain to be verified. No-echo is not proof of empty space.

## Optional moving-target observation logic (OFF by default)

**File:** `UNO_Test3/MapConfig.h`. Search for:

```cpp
#define A4_MOVING_OVERTAKE_EXPERIMENT 0
```

To compile the *optional experimental observation gate*, change **0 to 1**; to revert, set it back to **0**. Its code is surrounded by `#if defined(A4_MOVING_OVERTAKE_EXPERIMENT) && A4_MOVING_OVERTAKE_EXPERIMENT` in `UNO_Test3/A4Bug2.h`.

When enabled, the robot stops approximately one second to inspect the change in range. Significant movement causes a conservative WAIT rather than a blind detour. **This is NOT a fully implemented, validated moving-robot overtaking function** and should not be treated as permission to demonstrate overtaking a moving robot. First validate the stationary obstacle behavior and track-side rule. The default requires no uncommenting and does not run this optional code.

## Data logging: reuse your teammate's Assignment 3 workflow

1. Connect the PC to the ESP32 hotspot `ECE4333_A2_V2`, as in the tested Test 11 system.
2. From `ECE4333_A4_Test3/PC_Mapping`, run:

   ```bash
   python map_logger.py --from-start
   ```

3. Start the robot using the same mapping-run control from the ESP32 web page. Keep the PC logger running during the obstacle test.
4. Press Ctrl+C to close the logger and write the summary files.
5. Generate the original track map with `python draw_map.py logs/scan_YYYYMMDD_HHMMSS/samples.csv`.
6. Generate the obstacle-avoidance plot with `python draw_avoidance.py logs/scan_YYYYMMDD_HHMMSS`. Open `avoidance_map.svg` in a browser.

Output in `logs/scan_<timestamp>/`:

| File | Contents |
|---|---|
| `raw.jsonl` | every unmodified UNO JSON message plus PC/ESP32 receive timestamp |
| `samples.csv` | original N230 line, motor, ultrasonic and pose samples (~10 Hz) |
| `status.csv` | N221 dashboard status including `A4` state |
| `sensors.csv` | N210 battery and sonar status |
| `avoidance_events.csv` | new N231 event: previous/next state, run, t, estimated x/y, gyro yaw, odometry, sonar distance, pan angle, segment, detour side and distances, estimated obstacle point |
| `avoidance_runs.csv` | each detected avoidance attempt, **success/fault/cleared_without_detour/cancelled/incomplete**, elapsed time and estimated path travel |
| `avoidance_map.svg` | generated separately on PC: estimated route, detour intervals, transitions and approximate obstacle point |
| `log_summary.json` | counts and loss diagnostics |

N231 uses the compact 15-element array:

`[t_ms,run,from_state,to_state,x_cm,y_cm,yaw_deg*10,odometry_cm*10,sonar_cm,pan_deg,segment,side,side_cm*10,pass_cm*10,event_queue_drops]`.

State IDs 0–15 match Test 2's `A4Bug2::State`; 16 denotes operator cancellation during an avoidance maneuver. The Python decoder estimates the obstacle x/y **on the PC** when a fresh front-facing obstacle is first detected, using yaw and ultrasonic distance. Unknown coordinates are blank. These are **rough estimates**, not independently surveyed ground truth, and they do **not** yet compensate for sonar mounting offset or beam width.

### Interpretation and reliability

- **Estimated** pose comes from gyro integration + PWM/time-based wheel dead reckoning, not encoders.
- For professor-required **actual** pose/heading, measure known positions and heading manually during controlled trials and compare against `samples.csv`; these values are **not generated automatically**.
- `avoidance_runs.csv` labels **software state-machine completion**, not collision-free physical success. Film and verify actual maneuvers.
- Event FIFO is only 3 records; if it overflows, `event_queue_drops` increases. Wi-Fi map-log ring can separately overwrite older messages. Monitor both counters.
- Logging captures **state transitions** rather than every ultrasonic echo; periodic N230 samples contain sonar measurements. A single pan servo cannot monitor forward and sideways simultaneously.

## Suggested test order

1. Verify GitHub Actions for the latest Test 3 commit (Arduino Uno must compile, memory within limits).
2. Motors disconnected / wheels off the floor: verify servo pan 30°/90°/150° and sonar echo; verify gyro sign and actual left/right pivot direction.
3. Measure chassis width, sensor-to-rear distance, and the real motor/PWM distance model. Replace the **unverified estimates** in `A4Bug2.h`.
4. Capture stationary sensor and gyro data with the logger (no driving). Inspect `status.csv` and `raw.jsonl`.
5. Test one stationary box on an empty straight segment at low speed, with a physical power switch accessible. Confirm the A4 state sequence and logging. **Do not start on the crossing**.
6. Independently confirm the outside turn on each allowable section of the Figure 8; only then combine with STOP-sign and red/green tests.
7. Run multiple measured trials with different static obstacle sizes/shapes, log success/fault, stop-sign rule compliance and physical trajectory.
8. Only after these pass, discuss and independently validate **moving-target overtaking**.

## Course deliverable reminder

Assignment 4 requires performance evaluation, mapping/logging, estimated **and actual** pose comparisons, sign and obstacle positions in an inertial frame, completion time, average speed, state transitions and a written report. Test 3 now creates the obstacle transition data but does not itself satisfy every measurement or moving-robot demonstration requirement.
