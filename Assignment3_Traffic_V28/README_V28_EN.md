# Assignment 3 V28 - kinematic figure-8 follower (no line search), gyro fix, CSV logging

**Upload only `UNO_Assignment2/UNO_Assignment2.ino`** (whole folder). The motors, gyro and plan all live on the
UNO; the ESP32 is unchanged (it already forwards the UNO telemetry at `/traffic-status`; UV=28 passes its >=24 check).
The web page's "Search: idle" text and the search-angle fields are now cosmetic.

## 1. How the robot knows what to do at a curve
`Track.h` holds the track as segments (turn, length). Inertial frame: origin = start/traffic light, +X = initial
heading (along the long diagonal toward the upper-left corner), +Y = left, heading + = left turn.
Read from your figure: upper loop X-UR 1.2 m, X-UL 1.7 m, 60 deg at X -> top edge 1.513 m; lower loop X-A 1.0 m,
X-R 1.5 m, 60 deg at R, right angle at L -> kink A 30 deg, A-L 0.433 m, L-R 1.25 m. Travel direction = the figure's arrows;
the robot drives straight through X both times. Turns: UL -136.6, UR -103.4, A +30, L +90, R +120 (right turns
in the top loop, left in the bottom loop; sum 0 so the plan closes). Lap = 8.6 m. START_TO_UL_MM = 600 is my
estimate of where the light sits; the figure is hand-drawn, so **measure your tape and edit Track.h**.

Control (`TrackPlan.h`): drive a segment with a gyro heading-hold PD; when the estimated position has travelled the
segment length, pivot with the gyro to the next planned heading; continue. **No sweep/search anywhere.**
The floor sensors are only an outer loop: side sensors trim the heading set-point (+-8 deg); "line ends near a planned
corner" is used as a landmark that corrects the fused position (and learns the speed scale). If the line is not seen,
nothing happens - the plan just continues. Stops after LAPS_TO_RUN (2) laps at the start point. Red light / STOP sign
logic (TrafficLogic) is unchanged; it only decides whether the plan may move and at which PWM.

## 2. Gyro: does it have to be always on? Yes.
Yaw is an integral, so it must be sampled every loop from boot to the end of the run; skipping samples loses heading.
Problems found in V27 and fixed in `PoseEstimate.h`: one loop gap >250 ms switched the gyro off permanently, and the
kinematic pose froze with it; the zero-rate offset was measured once at boot only; +-250 deg/s range and no filter.
Now: +-500 deg/s, 44 Hz low-pass, PLL clock, micros() timing, offset re-learned whenever the car stands still, I2C errors
recover automatically (retry every 200 ms; plan stops and raises GYRO_UNAVAILABLE after 1.5 s), predicted pose no
longer depends on the gyro, no ultrasonic pulseIn() stalls in autonomous mode. Wrong Z sign during a pivot raises fault 10
(flip "Gyro Z sign" on the web page). Quick check: turn the car by hand 90 deg left; `YA` must go to about +90.

## 3. Two estimates (kinematics + fusion)
Predicted (`KX,KY,KH`): wheel speed from the PWM actually sent, v=(vR+vL)/2, omega=(vR-vL)/W, Euler/midpoint integration.
Fused (`X,Y,H`): same translation, heading from the gyro, plus corner-event and camera corrections.
There are no encoders, so `PoseConfig.h` (W, PWM_DEADBAND, cm/s per PWM) are placeholders. Calibrate: straight run at
your cruise PWM, k = distance / (seconds*(PWM-deadband)); pivot 360 deg and raise W until the predicted heading ends at 360.
Shortcut: the log column `speed_scale` is what the corner events learned - multiply the two k constants by it.

## 4. CSV / plot (PowerShell)
```
powershell -ExecutionPolicy Bypass -File tools\log_pose.ps1 -Out run1.csv      # PC on the car's Wi-Fi, then press Start
powershell -ExecutionPolicy Bypass -File tools\plot_pose.ps1 -In run1.csv      # Windows PowerShell 5.1 -> run1.png
python tools\plot_pose.py run1.csv [--rotate 150] [--actual points.csv]        # alternative, also prints report numbers
```
Telemetry is 2.5 Hz (9600 baud link). CSV: fused_*, pred_*, gyro heading, segment/lap/phase, event error, speed scale, faults.
"Actual" = the reference track overlay (dashed) plus an optional hand-measured points file (`x_cm,y_cm`).
`--rotate 150` shows it oriented like the assignment picture.

## 5. What was and was not verified
* `tests/v28/run_sim.sh`: the real PoseEstimate.h/TrackPlan.h run against a simulated robot. 2 laps complete, stays within
  ~3 cm of the tape when the model speed is -12%..+5% off, with gyro bias, wider wheel base, 4% larger track; degrades
  beyond (about +12% speed error gave up to 15 cm). `tests/v28/sim_demo_plot.png` shows the resulting figure-8.
* UNO sketch passes a host syntax check with Arduino stubs; N221 telemetry is at most 243 chars (ESP32 limit 250).
* **Not done:** AVR compile (flash/RAM unknown - V27 used ~91% flash; I removed Ex3Tracking/CrossingControl and added
  TrackPlan, but please check the Arduino IDE numbers), any real-robot run, PowerShell scripts (written, untested here).
* Real-car tuning knobs, in order: PWM->speed constants, SENSOR_AHEAD_CM, CORNER_LEAD_CM, TURN_MIN_PWM, HOLD_KP, START_TO_UL_MM.
* The camera landmark (STOP sign) correction stays off (LANDMARK_ENABLED=false) until A3_FX_NORMALIZED and the sign position are measured.
