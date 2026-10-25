# Test11 – predicted/fused map, target following, UNO memory (read this first)

Built on Test10. Upload **both** sketches again (the UNO now reports `UV=41`, the ESP32 and web page require it):
`UNO_Mapping/` (UNO, uploaded with the ESP32 switch OFF) and `ESP32_Assignment2/` (ESP32). Everything else (wiring, camera settings, red/green detection, STOP-sign AI, settings tab) is unchanged.

**Not tested on the car and not compiled for AVR/ESP32 here.** What was run: the real UNO sketch on a PC (protocol, E-stop, follow mode, wait-for-green, lap limit, message widths), a closed-loop simulation of the follow controller, the line-following simulations, the page in headless Chromium against a mock car, and a syntax check of the ESP32 sketch. Flash numbers below are estimates; the Arduino IDE's number is the truth.

## 1. Plot: predicted vs fused (Map tab)
- Every `N230` sample now also carries the **predicted pose** (wheel model only: v=(vl+vr)/2, ω=(vr−vl)/b, Euler integration on the UNO, no gyro): heading×10, x, y. Fused x, y and gyro yaw were already logged.
- Map tab → **Layers**: Drawing / Predicted (brown dashed) / Fused (green) / Mapped (the old coloured line). Predicted and fused are plotted exactly as the car believed them (unscaled); Mapped keeps the gyro-offset/drift fit, distance scale and lap closure.
- New **heading chart** (gyro/fused, predicted, mapped, drawing heading per segment) – same idea as "Gyro vs Predicted Heading" in the assignment. Buttons: Heading chart PNG/SVG.
- Summary now lists, for predicted / fused / mapped: mean, rms, max distance to the drawn centre line (cm), heading error on the straights, predicted-minus-gyro heading, lap closure. Useful numbers for the report.
- `Track points CSV` has fused_* and pred_* columns; `Download data CSV` has pred_heading_deg, pred_x_cm, pred_y_cm and the target columns.
- Old recordings (no predicted pose) still open; the predicted layer is just absent.

## 2. What the code does when the map is wrong
The map is **not used to drive**. The car always follows the line with the 3 floor (IR) sensors; dead reckoning + gyro only picks the corner direction when the sensors are ambiguous (route hint) and detects segments for the plot. If the hint is wrong the sensor rule still applies; if the line is lost the gyro-guided search runs; if that fails the run stops with a fault code (FC). So "map fails → line sensors" is already how it works.

## 3. Assignment 3 check
| Item | Status |
|---|---|
| Kinematic model, Euler integration on the Arduino, predicted x, y, heading from the same PWM that drives the wheels | yes (`PoseEstimate.h`), now logged and plotted |
| Gyro fusion, gyro vs predicted heading comparison | yes + heading chart |
| Emergency stop (web button, obstacle) | yes (ultrasonic latch also while following) |
| Traffic light / STOP sign from camera | yes (Stop on red) |
| **PD heading control with gyro inner loop, camera outer loop (part 5)** | **was missing – added** (`HeadingPD.h`, `TargetFollow.h`). The old code only had a P-type heading hold while crossing |
| Robot starts when the light turns green | **was missing – added** as a checkbox (Drive tab, Race options) |
| Stop at the end of the figure 8 | added: optional lap limit (stops at C facing D) |
| Use camera distance to the STOP sign at the intersection to correct the predicted position (part 4) | **not done.** The car stops for the sign but does not correct odometry. Needs the sign position along C-O-D (measure it) and the calibrated STOP distance; say if you want it |
| Wheel radius, max motor/robot speed, turning radius, noise discussion | report items, nothing to code |

## 4. Target following (Drive tab → Target following)
Press **Follow target 跟随目标** (STOP CAR stops it). Uses the current camera and red/green detection settings, nothing is changed. Choose green or red target.
- **Outer loop** (ESP32 → UNO `N215`, used every 0.5 s): target centre x and width in per mille of the image + age of the frame. bearing = (0.5 − x)·FOV; heading set point = gyro heading *at capture time* + bearing (the UNO keeps a 0.4 s yaw history, so the 0.25 s camera latency does not matter). Speed from the apparent width: full speed far, slow near, stop at the **stop width** (default 250 ‰ of the image).
- **Inner loop** (every loop pass): PD on the gyro-fused heading, error = set point − heading, derivative = gyro rate; left = v−u, right = v+u. Turns on the spot if the error is above 20°.
- Target lost > 1.5 s: stop; > 3 s: slow sweep towards the side it was last seen (up to 400°), then stop. Reacquired → follows again.
- Settings on the page: target colour, stop width, camera horizontal FOV (default 62° – **unknown for your lens, tune it**), "image mirrored", Kp×10 (15), Kd×100 (10). If the car turns away from the target tick mirrored; under-turns → raise FOV; overshoots → lower it; wobbles → more Kd / less Kp.
- Logged like a normal run: it appears as "Follow run n" in the Map tab with the XY track (predicted + fused), set-point vs gyro vs predicted heading, target width/bearing, and a summary (time to lock, time to arrive, heading error rms, lost count). In follow samples the L,C,R slots carry heading error, target width, bearing.
- Test it first with the wheels off the ground / car on a stand, then on the floor at PWM 60–70.

## 5. Race options (Drive tab)
- *Wait for green*: after Start the car stays in WAIT FOR GREEN until two green frames (needs Red/green detection on).
- *Stop after laps* (0 = run until STOP): counts a lap when the car finishes B-C and turns onto C-O-D, stops there.

## 6. UNO memory
Measured by the Test10 README: flash 27 476 B (85 %), RAM 1 496 B (73 %). Test11 changes:
- Removed: `vfprintf` (replaced by the 40-line `LinkTx.h`), the `N210` battery line (the page never used it), unused fields of `N221`, 96 B of TX buffer (352 → 256, widest line checked by the test: N221 242 B, N230 149 B), one sample slot.
- Added: target following, PD, predicted log fields, race options.
- **Estimate (unmeasured): flash about +0.8 to +1.6 KB → roughly 88–90 %; RAM about −20 B → ~72 %.** Check "Sketch uses …" in the IDE. If you need room back, in `MapConfig.h`: `FOLLOW_ENABLED 0` (≈ −2 KB, removes target following), `PAN_SERVO 0` (≈ −0.9 KB, the camera servo is no longer centred at start), `IMU_QMI8658 0` (≈ −0.4 KB, only if your gyro is an MPU6050/GY-521; the Map tab shows the chip: IM 1 = MPU6050, 2 = QMI8658). With all three off the host-compiled sketch is the same size as Test10's.
- Code is now split: `LinkTx.h` (serial text), `HeadingPD.h`, `TargetFollow.h` (pure logic, tested on a PC), so the `.ino` stays small.

## 7. Tests (`bash tests/run_tests.sh`, needs g++, python3, node)
UNO protocol (71 checks incl. follow, wait-for-green, laps, line widths, `linkFmt` vs `snprintf`), follow simulation (static/moving/hidden/mirrored target, latency), route hint, line-following simulations (unchanged results), ESP32 syntax, page map logic (predicted/fused layers, charts, CSV).
Changed files vs Test10: `UNO_Mapping/*` (new: `LinkTx.h`, `HeadingPD.h`, `TargetFollow.h`; edited: `.ino`, `MapConfig.h`, `LineFollower.h`, `PoseEstimate.h`), `ESP32_Assignment2/` (`.ino`, `Config.h`, regenerated `TrafficPage*.h`), `tools/` (`map_core.js`, `map_ui.js`, `page_src.html`, `inject_map_page.py`), `tests/`. After editing `tools/` run `python tools/inject_map_page.py`.
