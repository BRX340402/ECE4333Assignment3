# ECE 4333 Assignment 4 – MERGED race build (new 160×160 model + your latest UNO/web changes)

Base: GitHub `BRX340402/ECE4333Assignment3`, branch `Test-14`, folder `A4` (new Edge Impulse model, project 1151471).
Merged with the changes made in the previous working copy (Assignment4TestV2).

## Folder contents
| Folder / file | Purpose |
|---|---|
| `UNO_Mapping_Assignment4/` | Arduino UNO sketch (line following, map, traffic light / STOP, avoidance, serial link) |
| `ESP32_Assignment4/` | ESP32 camera sketch (STOP-sign FOMO model, red/green colour, web page, map log) |
| `libraries/Stop_Sign_Detection_inferencing.zip` | NEW Edge Impulse STOP model (project 1151471, deploy v5, 160×160, class `stopsign`) |

## Test17 – fixes from the 13 Nov test data (UNO UV = 47; upload the ESP32 and ONE of the two UNO sketches)
**Two ready-to-upload UNO sketches (same code, one switch):**
| Folder | Use it for | Contains | Flash |
|---|---|---|---|
| `UNO_Race_Assignment4/` | Figure-8 race: STOP signs, traffic light, obstacle avoidance / overtaking | `ASSIGNMENT5_BUILD 0` | 32,242 / 32,256 B |
| `UNO_Mapping_Assignment5/` | Assignment 5: cone / target following, KEY, IR remote, battery LED, line following for SLAM | `ASSIGNMENT5_BUILD 1` | 30,192 / 32,256 B |

The Drive tab now shows which build is on the UNO. **What the 13 Nov data showed:** every Test16 run was on the Assignment 5
build, which has no STOP / light / avoidance code – only TRACK / CORNER / CROSSING states were logged and the car stood still with
the ultrasonic at 4–10 cm (the last-resort guard = "stops like an E-stop"). Use `UNO_Race_Assignment4` for the race.

| Problem (from the logs) | Fix |
|---|---|
| Ultrasonic gets NO echo from the plush moose until ~4 cm (earlier race runs: detour started at 4–15 cm) | The obstacle is fixed, so it is on the map: Drive tab → *Obstacle front: distance after corner B* (default 35 cm, from the logs). The detour starts 35 cm before that position even with no echo. |
| Detour cut back in beside the moose (PASS ended after "range + 30 cm" with range = 4 cm) | Pass by distance = distance to the obstacle + its length (22 cm) + car length, when the side echo never appears. |
| Obstacle right after corner B: no room for a 45° exit | Exit at 60° when the obstacle is closer than 40 cm. |
| Car pivoting at PWM 50 without turning (stalled, run 12) | Avoidance pivot PWM 50 → 80. |
| Car waits forever in front of something | After 1.5 s standing at the guard distance the detour is forced, whatever the map segment is. |
| Echoes from beyond a corner could trigger at 70 cm | An echo farther than the straight that is left (+10 cm) is ignored. |
| "Pulled away" counter never updated (a comment had swallowed the line) | Fixed (`far`). |
| Second STOP sign missed when the crossing came before the map stop line (run 12) | If O is reached on a STOP segment without a stop, the car stops right there for the 3 s (never crosses without stopping). |
| ESP32: `slowPixels` used before `driveSettings` was declared | Order fixed. |
| Target following: no blue | Blue target / cones (`b`): blue replaces green in the second colour slot for that run (g and b cannot be mixed). |
| Target following flapped TRACK ↔ ARRIVED | Arrive hysteresis 85 % → 70 % of the stop width. |
Removed from the RACE build only, to fit: servo test command, forward creep step of the line search, the "same echo twice = fixed
object" rule of the intersection scan, side-distance steering during the pass (it now only checks the echo). PC simulation of the
run-12 case (moose invisible, 35 cm after B): passed with 7 cm clearance, back on the line 37 cm after the moose, before corner C.
**UNO not on a COM port** is not a sketch problem: check the USB cable (data, not charge-only), the CH340 driver, and set the
shield switch to *Upload* while flashing.

## Test16 – Assignment 5 (localization and mapping). UNO UV = 46; upload BOTH boards
**Build profile** (`UNO_Mapping_Assignment4/MapConfig.h`): `ASSIGNMENT5_BUILD 1` (default) = Assignment 5 build: target / cone
following ON, KEY interrupt, IR remote, battery LED; the race STOP/light rules and overtaking are replaced by empty stand-ins
(`RaceStubs.h`). UNO **30,030 / 32,256 B**. `ASSIGNMENT5_BUILD 0` = the Test15 race build (32,096 B), same code base.

| A5 task | Where | Status |
|---|---|---|
| 1 KEY pin + interrupt starts the robot | `KeyIrLed.h` (INT0, falling edge, 250 ms debounce) | KEY starts the remote-selected state / stops |
| 1 IR remote selects the state | `KeyIrLed.h` NEC decoder on a pin-change interrupt (pin 9) | 1 = figure-8, 2 = cones, OK = start/stop, * # = stop (decoder tested with a simulated frame) |
| 1 Voltage detection + colour LED | A3 divider, WS2812 on pin 4 | green ≥ 7.6 V, yellow ≥ 7.0 V, red; blue flash on IR key; BV in N221 / Drive tab |
| 2 Follow the figure 8, gyro + camera readings | unchanged line follower, logs | ✔ |
| 3 Kinematic prediction + Gaussian-noise analysis | wheel model (page, from PWM), fused gyro+wheel; Map tab → **Gaussian noise analysis** | Monte Carlo: gyro σ 0–2 °/s × speed σ 0–10 %, 40 trials, mean/final position and heading error |
| 4 Vision target following, gyro inner loop + camera outer loop | `TargetFollow.h` + `HeadingPD.h` (mode 7) | re-enabled (was compiled out) |
| 5 Feature map along 3–5 colour cones | ESP32 cone sequence (Target following → Cone sequence, e.g. `rgrg`): ARRIVED → next colour → sweep → track; last cone → stop | alternate red / green cones |
| 6 EKF localization and mapping | ESP32 `Slam.h`: state [x, y, θ, landmarks], gyro + odometer prediction (F, G, M), range/bearing model (H, R), Kalman gain, Mahalanobis association, landmark initialisation | logged N250 pose, N251 observation, N252 landmark (with covariance), N253 cone reached |
| 5/7 Visualisation, debugging | Map tab → **EKF-SLAM map**: pose path, landmarks with 2σ ellipses, observation rays, time slider (evolving map); live track position | ✔ |
| 8 Compare with ground truth | Map tab: enter true landmark positions (`r 150 0 cone1`) → error per landmark, mean / max; **EKF-SLAM CSV** | ✔ |
| Traffic signs / intersection on the figure 8 | Test15 track localizer + objects (race build) or SLAM red/green landmarks (both builds) | ✔ |

Calibration for SLAM ranges: Settings → **CWS / CDC** (cone width ‰ at CDC cm; use the live "‰" readout with a red cone) and
WST / SDC for the STOP sign. Bearing = (0.5 − image x) × camera FOV (Follow tab FOV, mirrored if ticked). Camera must face ahead (pan 90).
PC simulation (2 loops, 4 cones, 7 % odometer error, gyro drift): 4 landmarks found, heading error 2.3° (dead reckoning 8.1°),
landmark errors 1–26 cm. Not tested on the car; the ESP32 sketch is not compiled here.

## Test15 – track localizer (UNO UV = 45; upload BOTH boards)
Everything except the competitor is fixed, so the robot's position is one number: the distance **P** along the track from S.
* **ESP32 `Localizer.h` (new)** – a 2-state EKF, x = [P, odometer scale k], runs on every 10 Hz UNO sample (N230).
  Predict: P += k · (wheel distance); during a detour only the along-track part counts.
  Update: every fixed landmark the UNO reports – corner pivot (state 3: D, E, A, B, C; axle 5 cm before the apex) and the
  intersection crossing (state 2: O twice a lap; axle 8 cm before O). The landmark is the nearest of that type to the predicted P
  (60 cm gate + 3-sigma test). The gyro drift is estimated on every straight against the known straight heading.
* **ESP32 → UNO N218** {D1 segment, D2 progress mm, D3 raw odometer mm, D4 scale ×1000}: sent after each landmark fix and every
  1 s while line following. The UNO sets its segment progress and odometer scale from it, so the STOP line, the "room before the
  corner" check and the detour all use the localized position. (The UNO's own O→O scale learning is removed.)
* **STOP sign by width** – Settings → WST (sign width, ‰ of the image, at the stop distance SDC cm; live value shown). With WST > 0
  the stop is triggered by the blob WIDTH (stop at WST, slow at WST/2) instead of the lighting-sensitive pixel count, and the range
  to the sign = SDC · WST / width is logged. WST = 0 keeps the RP/RPS pixel thresholds.
* **Map objects** – the ESP32 logs N241 {object, P, range} when the car stops for a STOP sign, waits for the light, or starts
  avoiding. The Map tab draws the STOP signs, traffic light and obstacle at their **localizer track position + range** on the drawn
  track and lists them as "E-O, 18 cm before O" etc. (fused-estimate positions are still listed in the summary).
  New download: **Localizer + objects CSV** (P, scale, ±1σ, landmark, innovation, gyro drift, objects). The summary reports the
  landmark fixes, mean/max innovation, final ±1σ, scale and drift. Drive tab shows the live track position.
* **UNO flash** – the wheel-only "predicted" pose is now rebuilt by the page from the logged PWM (same 0.33 cm/s/PWM, 14 cm model),
  freeing 350 B. UNO: **31,980 / 32,256 B**, RAM 1,564 B.
* Simulation (2 laps, odometer 7.5 % short, 1.5° gyro drift): scale learned 1.076 (true 1.075), drift 1.4°, 14/14 landmarks matched,
  mean innovation 5 cm, objects placed within ~10 cm of their true track position. Not tested on the car; ESP32 not compiled here.
* Tune if needed: `CORNER_OFS` (5 cm) and `SENSOR_AHEAD` (8 cm) in `Localizer.h` – the axle position when a pivot / crossing starts.

## Test14c avoidance / overtaking (replaces the Test14b detour)
Same manoeuvre for the moose and for overtaking a slower robot (`Avoid.h`):
1. **Detect** – 2 ultrasonic readings ≤ 70 cm (a missed echo only counts down, so the soft moose is not lost). The car no longer waits
   400 ms of "settled" line following first – in Run 13 that delay let it get within ~5 cm before reacting.
2. **Cut out** – pivot on the spot to 45° towards the OUTSIDE of the loop, drive straight until the car centre is 22 cm beside the line
   (no wide arc: OUT/BACK/JOIN pivot first whenever the heading error is > 20°).
3. **Turn back a bit + check** – heading back to 10° outwards, servo turns the ultrasonic at the obstacle. Once the echo is seen the car
   holds the **side keep distance** (15 cm from the sensor) with 2° of heading per cm of error.
4. **Clear** – echo gone (or > keep + 20 cm) on 2 readings → drive 20 cm more (sensor to car rear).
5. **Cut back sharp** – pivot 70° towards the track, drive to the line, pivot onto it, hand over. If the forward echo shows the robot
   being overtaken < 30 cm ahead, it stays parallel until clear. If the detour passed the next corner it joins the next straight.
PC simulation (moose 15×15 cm, car 20×25 cm): static obstacle passed with 10 cm side clearance and rejoined 30-45 cm after it;
robot at 8 cm/s overtaken without contact (tight); a robot nearly as fast as us is not passed (gives up after 150 cm and follows).
`MapConfig.h`: `OVERTAKE_EVERYWHERE = false` keeps passing to B-C only (as in your merge); set true to pass on every segment
except E-O / C-O. Flash 32,188 / 32,256 B (avr-gcc 7.3) – almost full.
New page key `a4race16`: defaults load once (detect 70, cut-out 22, keep 15, car length 20).

## Test14b fixes after Run 13
| Problem in Run 13 | Fix | Where |
|---|---|---|
| Second STOP sign (C-O) missed | The B-C detour rejoined the line after corner C, but the map still said B-C, so the C-O STOP zone never opened. The detour now notices when it has passed the corner, heads for C-O, and hands over with the map on C-O (STOP zone 2 active, progress kept). | `Avoid.h` BACK/JOIN (`past`, `turn`, `cornerCm`), sketch handover |
| Avoidance started too close | Detect distance default 35 → 70 cm (page max 80 → 120). | UNO `DEF/HI`, ESP32 `/traffic-start`, page |
| Servo looked away from the obstacle | `PAN_LEFT_LOW = false`. During PASS the servo must now turn towards the obstacle; check once with the servo test. | `MapConfig.h` |
| Detour path too long | Superseded by Test14c above. | `Avoid.h` |
| Stopped too far from the STOP sign | A big red blob only stops the car within 25 cm of the map stop line (`RaceRules::STOP_WIN`); farther away it keeps approaching slowly. The map stop line still stops it if the sign is not seen. Also raise RP on the page (bigger blob = closer). | `RaceRules.h` |
| UNO flash | The merged build did not fit (32,562 B with avr-gcc 7.3). Removed: map heading steer (`mapSteer`, it could fight the line follower), the SCL clock-out in I2C recovery, the heading hold during the search creep. Now **32,066 / 32,256 B**, 1,568 B RAM. Gyro sign auto-correction and odometer learning are kept. | `LineFollower.h`, `PoseEstimate.h` |
The page stores race options under a new key (`a4race15`), so the new defaults (70 cm, 8 cm) load once; re-enter any other values you had changed.

## What was merged / changed
Kept from the Test14 base (new model and race features): STOP-sign AI (160×160, label `stopsign`), traffic light zone, 3 s STOP hold,
map + lap times, logs, the 45° avoidance and STOP-zone logic, the web page map tab.

Removed (as requested):
* **Emergency stop** – UNO command N207 and the E-STOP / Reset buttons in the web page are gone. The "something right in front" guard
  now only pauses the car (no latch).
* **Red / STOP distance calibration** – the web routes `/traffic-sample`, `/traffic-calibrate`, `/traffic-clear`, `/calibrate` and the
  "Distance calibration" panel are removed. Red stop still uses RP pixels, so it does not need this.
* **No-gyro navigation mode** – N213 accepts only 1 (zero) or 2 (re-measure bias). Start is refused without a working gyro.
* **Laps limit** – the car runs until STOP / fault / STOP CAR. Laps are still counted and shown in the Map tab.

Restored / added:
* **Odometer scale learning** (`LineFollower.h`, `atO()`, `odo()`): the O→O loop (O-A, A-B, B-C, C-O = 421 cm) is timed and the
  odometer scale is corrected when the difference is within 25 %. Start counts from the gyro/wheel model as before.
* **Map heading steer** (`LineFollower.h`, `mapSteer()`): while the follower is centred, the wheels get ±15 PWM at most to hold
  the route heading (weight `MAP_STEER_PER_DEG` = 0.6 PWM/°, in `MapConfig.h`).
* **Overtaking / avoidance only on B-C** (segment 5, the moose): `ovt.armed = follower.seg == 5`. On the other segments the car
  follows the normal line rules.
* **Shorter avoidance margin**: `need()` = rg + latCm + tailCm + 10 (was +30), and the corner exclusion 25 → 10 cm.

## Install / upload
1. Remove the old `Stop_Sign_Detection_inferencing` folder from `Documents/Arduino/libraries` (deploy v3), then Arduino IDE → Sketch → Include Library → Add .ZIP Library → `libraries/Stop_Sign_Detection_inferencing.zip` (v1.0.5, deploy v5). The ESP32 build now stops with an error if the old model is still installed.
2. ESP32 (M5Stack Wide, PSRAM on, partition ≥ 3 MB, e.g. "Huge APP"): upload `ESP32_Assignment4/`.
3. UNO: upload `UNO_Race_Assignment4/` (race) or `UNO_Mapping_Assignment5/` (Assignment 5). UV must be 47.
4. Join Wi-Fi `ECE4333_A2_V2` (password `12345678`), open http://192.168.4.1

## Verification status
* **UNO**: compiled with clang (AVR target, stub Arduino headers) with no errors. Estimated text 28.2 KB (original: 28.3 KB by the same
  method). The real Arduino IDE flash/RAM figure has NOT been checked: please confirm it is below 32,256 bytes.
* **ESP32**: NOT compiled (no ESP32 toolchain here). The page JavaScript was syntax-checked with node and all element IDs used
  by the script exist in the HTML. The gzip page header was regenerated and verified by decompressing it.
* Nothing has been tested on the real car. Tune RP / RPS / RPL, the detection distance, the lateral offset, and the odometer scale.

## Known open points
* `distanceK` (ESP32) is still loaded/shown but is never written any more, so it always reads 0 ("not calibrated").
* `README_Test14.md` describes the old behaviour; use this file instead.
