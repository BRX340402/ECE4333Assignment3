# ECE 4333 Assignment 4 – Test14 race build (UNO UV=44 + ESP32)

## What is in this folder
| Folder | Board | Notes |
|---|---|---|
| `UNO_Mapping_Assignment4/` | Arduino UNO | State machine, map, traffic rules, avoidance. 31,836 / 32,256 bytes flash (420 free), 1,546 B static RAM (avr-gcc 7.3, checked). |
| `ESP32_Assignment4/` | ESP32 camera (M5Stack Wide, PSRAM on) | Camera, colour zones, STOP-sign FOMO model, web page, 6000-line log. |
| `libraries/StopSign_Detection_ArduinoLibraries.zip` | – | Edge Impulse export: FOMO, 160×160 RGB, class `stopsign` (project 1151471). |

**About the AI model:** the original A4 rar had the inference code but not the model; it expected a different library
(`A2_StopSign_FOMO_104`, 96×96, class "3"), and no training code. Training happens in Edge Impulse Studio.
The ESP32 sketch now uses your new export (`MODEL_SIDE=160`, label `stopsign`).

## Install / upload
1. Arduino IDE → Sketch → Include Library → Add .ZIP Library → `libraries/StopSign_Detection_ArduinoLibraries.zip`.
2. ESP32: board with PSRAM enabled, partition with ≥ 3 MB app (e.g. "Huge APP"). Upload `ESP32_Assignment4`.
   The model needs a ~480 KB arena; `EIMemory.cpp` puts it in PSRAM. Expect 1–2 s per STOP-sign inference.
3. UNO: upload `UNO_Mapping_Assignment4` (UV must be 44 or Start is refused).
4. Join Wi-Fi `ECE4333_A2_V2` (password `12345678`), open http://192.168.4.1

## Race procedure
1. Settings tab: line thresholds, gyro sign GS (turn the car LEFT by hand → yaw must increase). If GS is wrong anyway, the first corner pivot detects it and flips the sign automatically (until power-off; the page then tells you which GS to save).
2. Drive tab → *Servo test 0°*: if 0° looks RIGHT set `PAN_LEFT_LOW=false` in `MapConfig.h`.
3. Stop on red ON; tune RP (stop), RPS (slow, 0 = RP/3), RPL (red light, 0 = any red region) with the live pixel number.
4. Put the car at **S** (middle of O-D, in front of the traffic light, facing D). Tick *Wait for green*, set laps, Start.
5. Change the race speed while driving with *Live race speed* (not saved; Cruise PWM is the start value).

## Behaviour (UNO)
* **Map** (`MapConfig.h`, `LineFollower.h`): 8 segments from S: S-D, D-E, E-O (STOP), O-A, A-B, B-C (moose), C-O (STOP), O-S (light).
  Corners advance by heading, O by the crossing signature (position snapped), S by distance (lap count).
* **Vision zones**: traffic-light detection only on O-S / at the start; red blob + STOP AI only on E-O and C-O; off elsewhere.
* **STOP sign** (`RaceRules.h`): blob ≥ RPS → slow (% setting); blob ≥ RP or map stop line → stop. The 3 s timer starts with
  the AI request; after the ESP32 has the frame (N219) the servo looks left/right with the ultrasonic; go when ≥ 3 s and clear.
* **Avoid / overtake** (`Avoid.h`, Bug algorithm): race speed, no E-stop. 45° out on the outside of the loop → servo looks
  sideways and records the obstacle distance → echo gone or > recorded + 6 cm → one car length more → 45° back → rejoin.
  On E-O / C-O or too close to a corner it follows the competitor at the gap and passes later.

## Logs, map, lap times (Map tab)
* Every UNO line is stored on the tablet and in the ESP32 PSRAM ring (6000 lines ≈ 9 min, `/map-log`).
* Each 10 Hz sample has fused pose (gyro + wheels), predicted pose (wheels only), state-machine code and map segment.
* Downloads: **Track points CSV** (t, mapped x/y/heading, fused x/y/heading, predicted x/y/heading, state, lap, segment),
  **Lap times CSV** (per lap: time, distance, average speed, moving average speed, STOP holds, red stops, avoidance,
  fused/mapped end position) and a race total (time from the green light, distance, average speed, best lap),
  Data CSV (raw samples + state + segment), State transitions CSV, Map PNG/SVG, heading chart, segments.
* The map origin is S; the drawing starts at corner C = (−(150 + start offset), 0).

## Serial protocol additions (Test14)
| Msg | Direction | Meaning |
|---|---|---|
| N101 D1=6, D2..D15 | ESP→UNO | start: green, laps, start offset, slow %, stop line, avoid on, detect cm, lateral cm, side margin, car length, scan °, clear cm, gap cm, axle-sensor cm |
| N204 …,"R":0-2,"Z":z | ESP→UNO | colour frame; R level per zone (1 light red / STOP slow, 2 STOP stop) |
| N205 / N219 | ESP→UNO | STOP AI result / AI frame captured (servo may move) |
| N217 D1=pwm | ESP→UNO | live race speed |
| N221 …,Z,CV,SN,SA | UNO→ESP | zone, race speed, STOP holds, holds confirmed by AI |
| N230 d[18] | UNO→ESP | low nibble state code, high nibble map segment |

## Not verified here
* The ESP32 sketch was edited but **not compiled** (no ESP32 toolchain available) – compile it first and report errors.
* UNO logic was compiled and the STOP-sign, traffic-light and avoidance state machines were unit-tested on a PC
  (3 s hold, scan, green start, 45° detour around a 15 cm box with rejoin); the page's lap/map code was tested on a
  simulated 3-lap run. Nothing has been tested on the real car: tune RP/RPS/RPL, the detection distance and lateral offset.
* Removed from the UNO to fit flash: manual arrow driving (`MANUAL_DRIVE 0`), the shield KEY start/stop button,
  the no-gyro timed-turn mode (Start now always needs a working gyro), the duplicate-key check of the serial parser,
  and the IMU diagnostic fields IW/IQ/IB. Gyro sign auto-correction is kept.
* Spare candidates if you need more room later: `IMU_QMI8658 0` (−156 B, only if the page shows the MPU6050 chip),
  N5 servo test (−94 B), search creep step (−236 B), I2C bus recovery (−258 B), predicted pose (−358 B, not recommended).
