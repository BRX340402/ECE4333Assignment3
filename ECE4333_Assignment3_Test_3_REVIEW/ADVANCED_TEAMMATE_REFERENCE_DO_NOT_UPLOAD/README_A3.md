# ECE 4333 Assignment 3 – Arduino code (built from Demo2)

Open `UNO_Assignment3/UNO_Assignment3.ino` in the Arduino IDE (board: Arduino Uno). All `.h/.cpp` files must stay in the same folder.
I2Cdev/MPU6050/DeviceDriverSet are the Demo2 files (one bug fixed: uninitialised `cout` in `MPU6050_dveInit`).
Demo2's `ApplicationFunctionSet` and ArduinoJson are no longer used.

## Score-sheet item → where it is

| Score item | Code |
|---|---|
| Controller tuning (figure 8 w/o veering) | `Control.cpp: Line_control` (PD on 3-sensor line position + gyro yaw-rate damping + speed scheduling + pivot in sharp corners). Gains in `Config.h §6` |
| Emergency stop | `checkEmergency()`: obstacle (ultrasonic), car lifted, line lost, cross-track saturation (deviation), optional geofence on fused pose |
| Kinematics prediction | `Estimator.cpp` (`xk,yk,psik`): v=r(wR+wL)/2, w=r(wR-wL)/L, Euler integration, driven by the same PWM sent to the motors |
| Data logging / comparison | 2 Hz CSV `L` records: predicted (kin), gyro, fused pose; `K` checkpoint records; `PC_Tools/plot_log_a3.py` |
| Gyro sensor fusion | `Est_update`: gyro heading + bias learning (stopped / driving straight vs model); position integrated with fused heading |
| Camera sensor fusion | `Est_cameraCorrect`: range+bearing to a mapped stop sign corrects position and heading; intersection-centre loop closure |
| Traffic light / stop sign | states `WAIT_GREEN`, `STOP_CLASSIFY`, `STOP_SIGN_HOLD`, `RED_LIGHT_WAIT` |
| Intersection management | `APPROACH_INTERSECTION` (red seen, or pose near learned centre), stop at standoff, hold 2 s, `CROSS_INTERSECTION` (gyro heading hold straight across) |
| State machine | `runStateMachine()`; every transition logged as `E,t,mode,from,to` |
| ESP32↔Arduino protocol | `Protocol.*`: reads the existing A2 JSON frames (N=200 stop sign, N=203 red heartbeat, optional N=204 green) **and** its own `$`-framed ASCII protocol with XOR checksum |
| Vision-based target following | `S_FOLLOW_*`: outer loop (camera frame → target heading set point, latency-compensated) + inner loop (`Heading_control`, gyro-fused heading PD) |
| Recorded data | `L` (est. vs predicted pose/heading), `S` (sign/light location), `T` (target location), `I` (intersection), `E` (states), `R` (completion time, distance, average speed) |

## Hardware / wiring assumptions
* Stock Elegoo V4 pins from Demo2 (TB6612 motors, ITR20001 on A0–A2, MPU6050 I²C). Ultrasonic TRIG 13 / ECHO 12 (set `USE_ULTRASONIC 0` if unused).
* ESP32 UART = the Uno's hardware Serial (same as A2). **Unplug/switch off the ESP32 while uploading.**
  `SERIAL_BAUD` is 9600 to match the A2 ESP32 firmware (Serial2 9600) – no ESP32 change needed.
* Serial Monitor shows the log *and* ESP32 frames. Set it to "Newline".

## Calibration (do once; all values in `Config.h`)
1. `WHEEL_BASE_MM`, `WHEEL_RADIUS_MM`: measure.
2. `MOTOR_W_PER_PWM_L/R`, `PWM_DEADBAND`: drive straight at PWM 70/100/130 for 3 s on the floor, measure distance. v = r·K·(PWM−deadband); fit K (left/right separately if the car curves). Spin in place and compare logged `psig` to a protractor to check `L` and `GYRO_SIGN`.
3. Line sensors: run Demo1, read floor/line/lifted values → `LINE_WHITE_RAW`, `LINE_BLACK_RAW`, `LIFT_RAW`.
4. Camera: `CAM_HFOV_DEG` and `CAM_IMAGE_W_PX` (bearing from the ESP32 box centre X), `ASSUMED_RED_DIST_MM` (distance at which the red heartbeat first turns on), `CAM_FWD_OFFSET_MM`.
5. Power up with the car **still** for ~1.5 s (gyro bias calibration).

## Controller tuning procedure (figure-8)
1. Set `BASE_PWM` ~70, `KD_LINE=0`, `KG_LINE=0`, raise `KP_LINE` until it just starts to oscillate on a straight, then use ~half.
2. Add `KD_LINE` (or `KG_LINE`) until the oscillation is damped; raise `BASE_PWM` step by step.
3. Sharp 60° corners: increase `SLOW_GAIN` (slows earlier) and `REVERSE_MAX_PWM` (inner wheel may reverse).
4. Follow-mode: tune `KP_HEAD`/`KD_HEAD` by commanding a 45° target step; then `FOLLOW_KP_DIST`.

## Running
* Figure-8: place the car at the start facing the track, power on (stays still ~1.5 s), then press the on-board key (pin 2), or show a green light if the ESP32 sends `{"N":204,"G":1}`, or type `$G` (or `START_WITHOUT_LIGHT 1`).
* Follow mode: `DEFAULT_MODE MODE_FOLLOW`, or type `$M,1`. Show the colour target.
* Serial commands (checksum optional when typed): `$G` go, `$M,0|1` mode, `$R` resume after e-stop, `$K,<n>` checkpoint (press when the car passes a floor mark; compare the logged estimate with the measured mark → "actual vs estimated").
* Save the Serial Monitor text, then `python PC_Tools/plot_log_a3.py run1.txt [--truth truth.csv]`.

## Log format
`L,0,t_ms,mode,state,xk,yk,psik_d10,psig_d10,xf,yf,psif_d10,cmdL,cmdR,pos_x100,cls,dist_mm,bear_d10,odo_mm`
(`k` = kinematic only, `g` = raw gyro, `f` = fused; mm; heading in 0.1°, CCW+, 0 = start direction).
`E,t,mode,from,to` state change · `X,t,reason,x,y` emergency · `S,t,cls,x,y` mapped sign/light · `C,t,idx,ok,x,y,psi` camera correction · `I,t,xI,yI,x,y` intersection · `T,t,x,y,psi,d` target · `R,time_ms,dist_mm,avg_mm/s,crossings` summary.

## Honest limits
* There are no wheel encoders: wheel speed comes from PWM, so the kinematic prediction is only as good as your calibration.
* Tested only in a host simulation (stubs + simple robot physics), not on hardware, and not compiled for AVR. Expect to retune gains and sensor thresholds on the real car.
