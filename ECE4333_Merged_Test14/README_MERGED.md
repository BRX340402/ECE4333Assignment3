# ECE 4333 Assignment 4 – MERGED race build (new 160×160 model + your latest UNO/web changes)

Base: GitHub `BRX340402/ECE4333Assignment3`, branch `Test-14`, folder `A4` (new Edge Impulse model, project 1151471).
Merged with the changes made in the previous working copy (Assignment4TestV2).

## Folder contents
| Folder / file | Purpose |
|---|---|
| `UNO_Mapping_Assignment4/` | Arduino UNO sketch (line following, map, traffic light / STOP, avoidance, serial link) |
| `ESP32_Assignment4/` | ESP32 camera sketch (STOP-sign FOMO model, red/green colour, web page, map log) |
| `libraries/StopSign_Detection_ArduinoLibraries.zip` | Edge Impulse library: install with Sketch → Include Library → Add .ZIP Library |

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
1. Arduino IDE → Sketch → Include Library → Add .ZIP Library → `libraries/StopSign_Detection_ArduinoLibraries.zip`.
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
