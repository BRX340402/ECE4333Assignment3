# ECE 4333 Assignment 3 — Test 3 (TEAM REVIEW / NOT HARDWARE VERIFIED)

**Baseline:** the physically tested Test 2. Test 3 is a proposed one-package integration, NOT proof that all scoring-sheet items are complete. Keep Test 2 backed up. Do NOT flash the alternate advanced UNO code inside `ADVANCED_TEAMMATE_REFERENCE_DO_NOT_UPLOAD`.

## Firmware you must upload as a MATCHED pair

1. **Arduino UNO:** `UNO_Assignment2/UNO_Assignment2.ino` (the folder name is historical; this is Assignment 3). Board = Arduino UNO. Compile first; check Flash and SRAM usage.
2. **ESP32 camera:** `ESP32_Assignment2/ESP32_Assignment2.ino`. Use the ESP32 board package compatible with the camera/AI libraries, Huge APP, PSRAM Enabled, correct COM port, and your Elegoo upload-mode selector. Compile first.
3. Return the Elegoo selector from UNO **Upload** to **CAM/run** before normal operation. The code is designed to keep motors OFF until explicitly started.
4. Connect to Wi-Fi `ECE4333_A2_V2` using the password already set in `ESP32_Assignment2/Config.h`. Open `http://192.168.4.1/traffic`.
5. First check `http://192.168.4.1/traffic-status`: `uno.UV=32`, `settings.applied=1`, and recent top-level `age_ms`. The web page must read **Test 3**. Never mix Test 3 UNO with Test 1/2 ESP32 or vice versa.

## Preserve Test 2 features

- Start begins normal black-line tracking; no green required to start.
- Red enters VERIFY; confirmed STOP holds for 3 s then CROSS; red traffic light waits for **two valid distinct green observations within 6 s**, retaining progress through intermittent invalid frames but resetting for accepted red.
- Black-line recovery: 120° right, 240° left, 120° right return, <=100 ms low-speed forward nudge **only with ultrasonic clearance**, max 5 attempts.
- Nominal autonomous cruise PWM remains **50**; reduced/approach PWM **30**. No camera resolution or recognition changes.
- The original web **STOP CAR** control remains a normal stop.

## What Test 3 changes

### Manual and automatic E-stop (UNO + ESP32)

- Two **new** browser controls: **EMERGENCY STOP (latched)** and **Reset E-stop**. Uses compact UART request `N=207, D1=1` or `D1=0`. Does not require the infrared remote.
- While latched, the final motor-output gate forces PWM=0 and disables TB6612 STBY. Pressing Start cannot override it. Reset returns the robot to standby: **you must press Start separately**, after checking the area is safe.
- During straight-ahead autonomous motion only, a valid ultrasonic obstacle detection at <=**12 cm** automatically latches a stop. If the sensor fails or measures -1, this particular obstacle check is not active; other recovery logic remains. It is a **software** stop, not safety-rated. Never rely on network buttons to protect a person or prevent an edge fall.
- `uno.ES` = 1 if latched; `uno.ER` = 1 web command, 2 ultrasonic. `uno.PL/PR` are the *actual applied signed PWM commands*; read 0 when latched.

### Right-wheel PWM trial

`UNO_Assignment2/Test3Options.h` contains `RIGHT_PWM_TRIM=8`. At commanded right PWM=50, actual right PWM becomes 58; negative right PWM gets the same magnitude addition. Zero stays zero and physical motors are commanded OFF in stopped states.

This is **experimental** to investigate low torque at PWM 50. If the robot steers too far, set `RIGHT_PWM_TRIM=0` and recompile ONLY the UNO. First compare with wheels lifted and then brief tests on the floor. A worn motor, wiring or low battery can cause weak wheel torque; extra PWM doesn't repair hardware.

### Wheel kinematics prediction

`PoseEstimate.h` now updates kinematic X/Y/heading even if the gyro is absent (`GY=0`). Provisional constants in `PoseConfig.h`: `0.07 cm/s/PWM` left/right, `TRACK_WIDTH_CM=14`. **These are NOT calibrated or verified**; the robot may have unequal wheel speeds. The source retains `POSE_CALIBRATED=false` and camera landmark correction disabled until measured. Gyro-assisted X/Y/heading is an estimate, **not ground-truth actual pose**.

Pure differential-drive model: `v_L=k_L * PWM_L`, `v_R=k_R * PWM_R`; `v=(v_R+v_L)/2`, `omega=(v_R-v_L)/track_width`. Integrate `(x,y,heading)` over elapsed time. The code uses elapsed `millis` and signed effective motor commands. Gyro updates heading only if the compatible MPU6050 registers respond. Your latest physical logs showed `GY=0`; check your actual IMU model before claiming fusion.

### CSV data logging and comparison

On a Windows laptop connected to ESP32 Wi-Fi, open `PC_Tools` in Explorer and run `START_LOGGING_WINDOWS.bat`. It launches `python a3_wifi_logger.py --seconds 120 --out run01` and saves `run01_pose.csv`, `run01_events.csv`, `run01_raw.jsonl`. The logger does **not control the robot**. A teammate can also control with an iPad connected to the same access point. Avoid locking the output CSV in Excel during recording; open after the 120 s run.

`ANALYZE_WINDOWS.bat` estimates total distance and average speed from *model-predicted* positions, not ground truth. For actual comparisons, fill `actual_points_template.csv` with checkpoint timestamps and ruler/protractor measurements (cm and degrees from a marked origin) and run:
`python compare_actual.py run01_pose.csv actual_points_template.csv`

The optional plots require `matplotlib`. Python 3 standard library suffices to log CSV.

### Serial format and memory precautions

Existing `N=221` traffic telemetry is extended modestly with `PL,PR,ES,ER` and firmware `UV=32`. To avoid filling the 256-byte UNO TX buffer and the 250-byte ESP32 receiver, the unused `PV`, `CF` and `SM` fields are not emitted by Test 3. `FR` is retained to monitor free RAM. Run a full Arduino Verify and **record SRAM/flash sizes**: no target-board toolchain was available in the preparation environment.

## Minimal safe hardware acceptance test (do in this order)

1. **Stationary:** power up with wheels securely lifted. Confirm Test 3 UI, `UV=32`, `applied=1`, `ES=0`, fresh telemetry. Keep physical battery switch accessible.
2. **STOP and Reset:** Start a low-speed wheels-up test; press **EMERGENCY STOP**. Verify both wheel PWMs become 0 and `ES=1`. Press Start and verify it cannot restart. Press **Reset E-stop**, verify `ES=0` and state OFF; only then press Start.
3. **Automatic obstacle:** On a clear floor at low speed, place a solid target where ultrasonic sensor detects it before contact. Verify a latched stop and `ER=2`. Don't make it collide; threshold is 12 cm, not guaranteed stopping distance.
4. **Right-wheel trial:** With wheels lifted compare free rotation at effective right PWM 58 vs left 50. Test gentle motion on floor. If it veers or behaves worse, disable trim and reflash UNO. Don't buy motors until checking connectors, battery and mechanical resistance.
5. **Test 2 regression:** Red -> WAIT_RED -> two accepted green confirmations -> TRACK. Confirm STOP-sign 3s hold. Check multi-stage line search in a big safe area. Stop testing at unexpected movement.
6. **Kinematics/CSV:** Record a 2 m straight test with timing, then a measured in-place rotation. Enter true physical checkpoints and calibrate both motor coefficients and effective track width before attempting a Figure 8 comparison.

## What this package does NOT prove or complete

- Physical motor PWM, obstacle reaction, full flash/RAM fit, and serial latency **not tested**.
- Gyroscope sensor fusion is implemented conditionally for an MPU6050-compatible device; your hardware has shown `GY=0`. If board uses QMI8658C, this driver does not work for that chip.
- Camera landmark-to-global-frame fusion exists as gated scaffold in `PoseEstimate.camera()`, but needs measured landmark coordinates, angle and range calibration before enabling. It is **NOT a demonstrated sensor-fusion result**.
- Teammate's advanced nested gyro/camera target follower and PD line controller are preserved as a separate reference program: they are NOT merged into the running UNO firmware. They use conflicting state machines and some different serial frames. Physical calibration and a mode/protocol port are needed.
- Physical actual position, sign positions, Figure 8 completion time and actual average speed require ground-truth measurement; PWM/gyro prediction is NOT actual position.

This is a practical test package for the student's own three selected tasks and preserves a safe platform for integrating the remaining teammates' deliverables. It is not a claim that the whole score sheet is satisfied.
