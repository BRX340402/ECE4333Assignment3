# ECE 4333 – Red→Green resume and 120° line-search patch

## Source and scope

This folder is a **modified copy of the user-supplied `Assignment3_Traffic 1.rar`**. It preserves the existing UNO/ESP32 traffic-control implementation, model library archive, and existing autonomous tracking functions. It does not overwrite GitHub or include unrelated proposed Safety/CSV integrations from another version.

**Status:** Offline source and host regression tests passed. **Not validated by compiling both complete Arduino firmware targets or by driving a real robot.** Test with wheels lifted and the physical power switch reachable. This is not a certified emergency-stop system.

## Which files were updated?

1. `UNO_Assignment2/TrafficLogic.h` — In **Stop on red** mode, three *different, valid* frames detecting green while no red is detected can release **VERIFY** as well as **WAIT_RED**. Previously VERIFY kept the motors stopped until STOP AI resolution or a 20-second wait before green was allowed to release WAIT_RED. STOP AI confirmation still uses the existing two-accepted-frame logic and moves to CROSS. A green reading never automatically clears an arbitrary FAULT or OFF state.
2. `ESP32_Assignment2/ESP32_Assignment2.ino` — When the robot is stopped in VERIFY or WAIT_RED in Stop-on-red mode, accept a sufficiently large green blob anywhere across the camera image rather than requiring green to appear within 18% of the prior red blob's horizontal position. Normal mode keeps the former location filter. Accept settings for the larger search; new installations default to cruise PWM 60 and reduced PWM 40. On a previously configured device, only the old default search setting (90°/180°) is migrated once to 120°/240°; custom scan angles are left alone. **Saved speed settings are not overwritten**.
3. `UNO_Assignment2/Ex3Tracking.h` — Search initially right **up to 120°**, then reverse left **up to 240° total** (ending 120° left of the original heading). Search stops earlier if a line is reacquired. The gyro yaw angle is used if available; otherwise it falls back to timed motor turning. Timed turning is an **uncalibrated approximation**, not a guaranteed physical angle. The former maximum was right90° then reverse-left180°.
4. `UNO_Assignment2/UNO_Assignment2.ino` — Expand acceptance of configurable scan angles in the `N=206` settings protocol: right 30–120°, reverse-left 60–240° with `sweep <= first+120`.
5. `ESP32_Assignment2/TrafficPage.h` — 120°/240° defaults and UI limits, adjusted green-release description. `ESP32_Assignment2/TrafficPageGzip.h` was **regenerated** from the HTML using `tools/build_web_page.py` (the root webpage serves this compressed version).
6. `tests/motion_v15_test.cpp` — Explicitly preserve 90°/180° for historical behavior tests and update the legacy-clamp case.
7. `tests/a3_green_120_search_test.cpp` and `tests/run_a3_green_120_checks.sh` — New regression checks.

All other original files, including the model package, remain available. See `CHANGES_FROM_FRIEND.patch` for an exact unified diff.

## What was the red→green issue?

A) The UNO state machine distinguishes `VERIFY` (number 4) and `WAIT_RED` (number 5). In Stop-on-red mode, **VERIFY already commands the car to stop**, but the old code only accepted green to leave WAIT_RED. Without STOP AI results, VERIFY could remain stopped for up to 20 seconds.

B) The ESP32 was also requiring the green blob to appear near the last red blob's **horizontal image coordinate**. When a teammate held up a different green sheet, it might be offset or the red position might be stale. The UI could show green while the UNO received `G=0`.

This patch fixes both. The UNO still needs three **distinct** accepted frames that are green-only, and the ESP32's color stream must still be valid. If green cannot be detected, or red background remains above threshold, the car intentionally remains stopped.

## Upload and use

**You need BOTH sketches from this package**, not just the UNO sketch. Do not mix with earlier `Assignment3_Traffic_A3_integrated.zip` unless someone explicitly merges the changes. Ask the teammate controlling the robot to back up the currently working firmware and upload these in a new Git branch.

1. Install/retain the exact supported board package and libraries described in `README_CN.md` and `Arduino_Libraries/`. Open `UNO_Assignment2/UNO_Assignment2.ino` as an Arduino sketch with all accompanying .h/.cpp files in the same folder. Target **Arduino Uno**. Verify then upload.
2. Open `ESP32_Assignment2/ESP32_Assignment2.ino` from its folder. Install the ESP32 core and the stop-sign AI library `Arduino_Libraries/A2_StopSign_FOMO_104.zip` if not already installed. Use the ESP32 board, Huge APP partition and PSRAM settings established in your teammate's working setup. Verify then upload.
3. Connect to the ESP32 Wi-Fi network; access `http://192.168.4.1/`. Switch preview resolution from startup SVGA to **CIF 400×296** before doing traffic tests (follow working team configuration). Changing camera resolution may automatically stop the car.
4. Under **Car control**, use: `Stop on red, then scan for STOP = checked`, **Cruise PWM 60**, **Reduced PWM 40** (or your team's already tested 30 if it moves acceptably), **First right search = 120**, **Reverse-left sweep = 240**. For red action threshold, start with the value your team has actually tested (the older project's notes cite 170 pixels). Do not treat guessed threshold values as proven.
5. Clicking **Save car settings** stops the car; wait for **UNO confirmed** before pressing **Start**. If already configured, verify actual values on the page since saved preferences persist.
6. With the wheels lifted: ensure Start triggers only expected slow motor behavior; display red paper: both PWM outputs should stop; replace with clean green paper: wait for three valid green-only detection frames, then confirm the car enters TRACK. Test the STOP sign independently; AI must detect and confirm it before the existing crossing logic releases the stop.
7. For searching: position the three floor sensors over a known non-black patch, then test first with wheels lifted and a gyro/yaw reading. The expected *commanded* pattern is right to 120° relative to the original heading, then reverse-left by 240° total. The search must stop immediately on an accepted line, or end in `SEARCH_EXHAUSTED` and stop if it cannot find the line. **Do not test this unattended near table edges, people, or obstacles.**

## Troubleshooting when green does not restart

While the issue is happening, save the output of `http://192.168.4.1/traffic-status` (copy its JSON into a text file) and a screenshot of the browser UI. Important values:

- `uno.TS`: `2` TRACK, `4` VERIFY, `5` WAIT_RED, `7` CROSS, `8` FAULT. If 8, inspect `uno.FC`, because green must never clear safety faults.
- `settings.RS` should be `1`; `settings.applied` should be `1`; `settings.RA=120` and `SA=240` after saving.
- `objects.green.seen` must be true on valid **fresh** frames; `vision_health.green_seen`, `vision_health.age_ms`, and `objects.red.seen` help diagnose stale/color classification.
- If `objects.green.seen` is false, this is a vision/threshold/lighting issue, not the state machine. Confirm the green sheet under good lighting, with red moved completely out of frame.
- If the ESP UI sees green, but UNO stays VERIFY/WAIT_RED, verify that UART traffic is flowing and three distinct frames reach the UNO. A broad red background above red-action threshold can reset the UNO's green counter.
- If the UNO enters TRACK but wheels do not rotate, check motor wiring/power and selected cruise PWM; 30–40 PWM may be too weak to overcome static friction.
- If black line search seems too small or too large, log `uno.YA`, `uno.GY`, `uno.SE`, `uno.SP`, `uno.SM`. With no trustworthy gyro, 12 milliseconds/degree at PWM70 is only a guess; measure actual rotation and calibrate the time factor in `Ex3Tracking.h` if necessary.

## Offline verification

`bash tests/run_a3_green_120_checks.sh` runs the original regression tests, added behavior tests, and STOP AI acceptance tests. These are **software tests, not physical-hardware validation**. Running legacy test scripts also regenerates the gzip UI header from HTML; that header is included.

## Important caveats

- No wheel angle calibration, obstacle-avoidance redesign, or new CSV logging was added in this patch; those belong to the separate Assignment 3 development work.
- If the traffic-light color remains red even after swapping the sheets, check the HSV/red-action pixel thresholds and camera image rather than widening them blindly.
- A detected red stop sign is still processed through the team's existing AI verification and crossing behavior; this patch does **not** add a mandatory timed 3-second stop for stop signs in Stop-on-red mode. That can be implemented if the instructor requires a hold.
- The 120° command is **a search-limit/target**, not a promise of exactly 120° physical rotation. The car often stops searching earlier when sensors find the line.
