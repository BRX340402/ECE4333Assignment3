# ECE 4333 Assignment 4 – MERGED race build (new 160×160 model + your latest UNO/web changes)

Base: GitHub `BRX340402/ECE4333Assignment3`, branch `Test-14`, folder `A4` (new Edge Impulse model, project 1151471).
Merged with the changes made in the previous working copy (Assignment4TestV2).

## Folder contents
| Folder / file | Purpose |
|---|---|
| `UNO_Mapping_Assignment4/` | Arduino UNO sketch (line following, map, traffic light / STOP, avoidance, serial link) |
| `ESP32_Assignment4/` | ESP32 camera sketch (STOP-sign FOMO model, red/green colour, web page, map log) |
| `libraries/Stop_Sign_Detection_inferencing.zip` | NEW Edge Impulse STOP model (project 1151471, deploy v5, 160×160, class `stopsign`) |

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
3. UNO: upload `UNO_Mapping_Assignment4/` (UV must be 44).
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
