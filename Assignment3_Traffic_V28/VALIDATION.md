# V27 green detection and reverse-left sweep

Real Uno build passes: 29582/32256 flash bytes (91%); 1426/2048 static RAM bytes (69%). AVR Boards1.8.6, avr-gcc7.3.0-atmel3.6.1-arduino7, bundled Servo1.2.2 and FastLED3.2.10. Command: arduino-cli compile --fqbn arduino:avr:uno UNO_Assignment2. Log: validation/v27_build.log. No ESP32 target build or physical vehicle testing.

New tests/green_search_v27_test.cpp covers pale/dark/yellow-green/blue-green, neutral/white/near-black/yellow/cyan/red exclusion, unchanged red, three distinct green frames during stopped VERIFY, gyro360 completion, timed360 scanning beyond180 and side-line reacquisition. Compile with g++ -std=c++11 and run. V26 production publish test and run_v24_host_checks.sh pass (including full ESP host-stub syntax). Production settings route accepts SA360 and rejects361; real UNO parser test receives SA360. Historical motion test updated only to reflect the newly allowed 300-degree left sweep; right limit90 preserved. Logs: validation/v27_regression.log.

Green spatial allowance widens from18% to35% of frame width. Blob minimum and consecutive valid green count unchanged. Wider colour thresholds may admit more green background; physical lighting/camera accuracy is not verified. SA is measured from the right-to-left reversal point; existing saved SA remains unchanged until user saves360. Gyro turns retain a nominal-duration fallback and overall search timeout, so angle is not guaranteed if gyro is stalled or motor timing differs.

---

# V26 STOP scan release

Both firmware sides updated; UNO UV26 and ESP32 A3 STOP Release V26. Stored settings schema retained.

Stop-on-red publish no longer vetoes accepted STOP detections on colour ROI age/position. UNO counts positive scores already accepted by the ESP detector in this mode; legacy fixed800 threshold unchanged. Request identity, valid-result flag and sequence guards remain. Crossing restarts at cruise PWM in stop-on-red mode; legacy reduced crossing speed retained.

Checks: python3 tests/stop_release_v26.py extracts and executes production publish with stale, missing and shifted red; verifies legacy association still vetoes; tests two nonconsecutive positives and production CrossingControl output60 with CS60/DS30. Updated real UNO command-handler regression verifies duplicate rejection and a score650 second accepted frame after negative frames. bash tests/run_v24_host_checks.sh covers previous behavior and full ESP host-stub syntax.

Real Uno build: arduino-cli compile --fqbn arduino:avr:uno UNO_Assignment2, Arduino AVR Boards1.8.6, avr-gcc7.3.0-atmel3.6.1-arduino7, Servo1.2.2, FastLED3.2.10. Flash29556/32256, static RAM1426/2048. Logs: validation/v26_build.log and validation/v26_regression.log. No physical validation or ESP32 target compilation this revision. Physical startup torque is a plausible contributor, not a measured root cause.

---

# V25 actual Arduino Uno build

Board: arduino:avr:uno, Arduino AVR Boards 1.8.6, avr-gcc 7.3.0-atmel3.6.1-arduino7; bundled Servo 1.2.2 and FastLED 3.2.10. Arduino CLI 1.5.2-rc.1.

Baseline V24: 32414/32256 flash bytes (fails link size check); 1426/2048 static RAM bytes.
V25: 29594/32256 flash bytes (build succeeds); 1426/2048 static RAM bytes. Saved 2820 flash bytes. Remaining 2662 flash bytes and 622 bytes for runtime stack/allocations. Runtime stack high-water mark is not measured.

Change: IRrecv::decode calls only NEC decoder; unused protocol decoder sections are removed by the AVR linker. Kit NEC remote codes/repeat handling remain intact; non-NEC remotes are unsupported. UNO UV becomes25; ESP32 V24 unchanged.

Command: arduino-cli compile --fqbn arduino:avr:uno UNO_Assignment2
Logs: validation/v25_before.log, validation/v25_after.log, validation/v25_regression.log.
Host regression: bash tests/run_v24_host_checks.sh. No physical vehicle or IR reception testing, and no ESP32 target build this revision.

---

# V24 zero reduced speed and red action area

run_v24_host_checks.sh passes. Production helper tests cover threshold below/equal/above, no detection at zero, speed0 no turning/search timeout, PWM1 no forced minimum30, and crossing after confirmed STOP at cruise when DS0. Route tests accept DS0/RP160000 and reject RP160001; browser serializes RP. Previous motion, parser, UI and ESP host syntax checks pass. No target/hardware validation.

---

# V23 cumulative STOP and reduced PWM

run_v23_host_checks.sh passes: production TrafficLogic preserves one STOP across negative/invalid frames and WAIT_RED; second distinct positive releases; duplicate sequence rejected by actual UNO command handler; green release remains three valid frames despite negative AI; DS travels through UI/server/UNO settings and is capped by CS. Existing regressions and ESP host syntax pass. No hardware or target compilation claims.

---

# V22 settings application and immediate motor stop

Full ESP host syntax and previous regression suite pass. Actual UNO command-handler tests apply RS=1 / CS=60, start mode6, set motor outputs to60, and deliver valid red: VERIFY latches and both mock motor outputs become zero inside command handling. Browser tests confirm save waits for matching acknowledged settings and leaves driving stopped. Hardware image latency and motor coasting are not tested; no target build or RAM/flash measurement.

---

# V21 red-stop and cruise options

Run tests/run_v21_host_checks.sh. Production TrafficLogic tests cover stop-on-red latch, stale/lost-red hold, bad inference, two positives at far range, negative classification, green release, timeout and disabled legacy behaviour. Settings route tests cover CS/RS serialization and range rejection; UNO parser checks CS/RS application; UI checks checkbox and speed payload. Previous regressions and full ESP host syntax pass. No target compilation, memory-size or physical stopping-distance validation.

---

# V20 page delivery

Generated gzip round-trip matches the full HTML (26,703 -> 8,923 bytes). tests/page_transfer_test.cpp executes production writePageBody with partial writes, intermittent zero writes, permanent stall and disconnection. Browser UI regression and full ESP host syntax pass. No hardware transport or target build validation was performed. Root cause of the reported browser error remains unconfirmed.

---

# V19 validation

Run `bash tests/run_v19_host_checks.sh`. Existing motion, parser, calibration and UI tests pass. New tests execute the actual UNO timeout statement for autonomous/manual/IR cases, production liveObjectsJSON for all three classes and disabled/resolution states, and browser overlays for coordinates/expiry. Full ESP source passes host syntax with stubs. White-paper calibration code and its obsolete tests were removed. No target compilation, hardware test or measured inference speedup is claimed.

---

# V18 search correction

Production Ex3Tracking tests now cover: first right sweep boundary at -90°, reverse left through zero to +90°, no third sweep, left-only sensor recovery during right search, normal left steering, bounds on legacy150/300 settings, stuck-gyro reversal and timed fallback. V17 white-balance and existing traffic/parser/UI regressions remain in the host suite. No target compilation or physical yaw calibration was performed.

---

# V17 white-paper calibration validation

Host checks: `bash tests/run_v17_host_checks.sh`.

- Executes the production calibration header with mocked sensor, JPEG decoder and NVS: centre-only measurement; best fixed preset; lock and auto restore; save failures; rollback on corrupt/truncated/overexposed frames; frame return accounting; temporal consistency and request guards.
- Web VM checks calibration and restore buttons, lock status, disabled manual WB controls and existing preview/car controls.
- Existing V15 motion, settings/calibration and UNO parser regression checks pass.
- Full ESP32 source passes host syntax checking with stubs. Actual ESP32 1.0.4 sensor.h was inspected for the three WB setter interfaces used.

These checks do not exercise the physical OV2640, actual SCCB/I2C writes, actual JPEG decoder or target linker. No target firmware compilation, UNO memory measurement or hardware image-quality claim is made for V17. Firmware UNO remains V15.

---

# V14 diagnostic validation — 2026-10-09

Passed: complete UNO and ESP32 host syntax with mocks; existing traffic regression; 10000-message UNO parser test; tests/fault_v14_test.cpp tests specific actual fault triggers, originating state, first-cause retention and explicit stop reset; JS tests for fault text and old-UNO fallback. N221 extreme numeric-field length is 214 bytes including newline, below its 224-byte capacity. Camera init/model/thresholds are unchanged. No target compile, physical run or proof of the field fault's root cause.

Run added fault tests:

```bash
g++ -std=c++11 -fsanitize=address,undefined tests/fault_v14_test.cpp -o /tmp/a3_fault_v14
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_fault_v14
node tests/ui_test.cjs
```

# V13 UNO validation — 2026-10-09

Passed: current full UNO sketch host syntax with hardware stubs; extracted actual command(), serialInput(), telemetry() and pumpTelemetry() on ASAN/UBSAN; 10000 successive camera vision packets including sequence wrap; complete current settings/colour/mode/heartbeat messages; strict integer bounds and malformed, duplicate, truncated, overflowing and overlength frame rejection; RX per-call quota and next-frame recovery; chunked telemetry; existing traffic movement/STOP/crossing regression. LeakSanitizer cannot run under this environment's process restrictions; leak detection disabled. The production parser itself performs no heap allocation.

Only the UNO sketch and its new IntegerCommand.h change at runtime. The unused ArduinoJson header is removed. All ESP32 files/model libraries match the V12 archive byte-for-byte. New UNO telemetry fields: UV=13, FR=sampled free RAM estimate, UT=uptime milliseconds. Full AVR/ESP32 compilation, SRAM high-water measurement and physical UART/CAM switching are NOT verified. Do not interpret host tests as target memory usage.

Reproduce protocol/traffic checks from this directory:

```bash
python3 tests/generate_uno_v13_test.py
g++ -std=c++11 -fsanitize=address,undefined -Itests/uno_parser_stubs tests/uno_v13_generated.cpp -o /tmp/a3_uno_v13
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_uno_v13
g++ -std=c++11 -fsanitize=address,undefined tests/traffic_test.cpp -o /tmp/a3_traffic_v13
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_traffic_v13
```

# V12 validation — 2026-10-09

Reference: user-uploaded ESP32-WROVER-Camera.zip. V12 keeps the current dashboard and car control, restores this source's camera profile, excludes face processing, and separates capture from colour decoding.

## Passed on host

- Full ESP32 sketch C++11 syntax using mocked Arduino/camera/Wi-Fi/FreeRTOS/model APIs.
- Extracted actual cameraInit: UXGA allocation, JPEG quality 10, two buffers, 10 MHz, runtime SVGA and final flip=0/mirror=0. All other ISP setter pointers are null in this test; no unexpected white-balance/exposure/gain call occurs. Init failure, null sensor and size failure handled; reset stops the car before queuing a reboot.
- Source comparison: camera_pins.h identical to vendor; all 24 non-face camera controls retained; no face inference headers, face models/calls or face RGB allocations in the sketch.
- Actual capture/cache/colour code with sensor and decoder injections: 12 same-sized captures use two staging allocations; readers retain immutable private copies while four newer captures arrive; driver frame returned before colour decode; no camera mutex held during decode; bad JPEG headers/allocation failures retain the last cache; old settings epochs, stale frames and resolution-busy frames rejected. ASAN/UBSAN enabled, leak checking disabled for the intentionally interrupted infinite-task harness. This is an interleaving simulation, not a FreeRTOS timing test.
- Actual preview response: partial writes complete, duplicate frame returns 204, stale cache returns 503, incomplete send closes socket. Reusable send buffer validated.
- Dashboard JS VM: read-only initial camera load, manual controls, restart request, preview pause/resume, last-frame retention on 503/network failure/204/JPEG decode failure, recovery, object-URL cleanup, tabs, calibration, Start/Stop and heartbeat.
- Regenerated current calibration routes and metrics: red/green/STOP scan/save/clear, STOP confirmation, frozen sample/token, resolution mismatch, persistence failure, movement restrictions, independent distances, stale target rejection; STOP display can operate with colour detection disabled. Older PASS labels in inherited tests describe the test's origin, not the source version exercised.
- Byte comparison against V11: all UNO sources, bundled 96x96 model archive, ModelShape, AIAdapter, CameraDecode, RedCamera and EI allocator unchanged.

## Not verified

No ESP32/AVR target build, board timing, frame rate, peak memory/flash usage, physical camera colour/noise, real Wi-Fi continuity or physical driving test. The previously reported image fault is not established as fixed. The target compiler is unavailable in this environment. Driver and model APIs in syntax checks are mocked.

## Reproduce from this folder

```bash
python3 tests/generate_v12_test.py
g++ -std=c++11 -fsanitize=address,undefined -Itests/jpeg_stubs tests/capture_v12_generated.cpp -o /tmp/a3_capture_v12
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_capture_v12
g++ -std=c++11 -fsanitize=address,undefined -Itests/esp_syntax_stubs tests/vendor_defaults_generated.cpp -o /tmp/a3_vendor_v12
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_vendor_v12
node tests/ui_test.cjs
g++ -x c++ -std=c++11 -fsyntax-only -Itests/esp_syntax_stubs ESP32_Assignment2/ESP32_Assignment2.ino
python3 tests/generate_preview_test.py
g++ -std=c++11 -fsanitize=address,undefined -Itests/jpeg_stubs tests/preview_generated.cpp -o /tmp/a3_preview_v12
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_preview_v12
python3 tests/generate_camera_control_test.py
g++ -std=c++11 -fsanitize=address,undefined -Itests/esp_syntax_stubs tests/camera_control_generated.cpp -o /tmp/a3_controls_v12
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_controls_v12
python3 tests/generate_routes_test.py
g++ -std=c++11 -fsanitize=address,undefined tests/esp_routes_generated.cpp -o /tmp/a3_routes_v12
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_routes_v12
python3 tests/generate_measurement_test.py
g++ -std=c++11 -fsanitize=address,undefined tests/measurement_generated.cpp -o /tmp/a3_metrics_v12
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_metrics_v12
```

Historical validation notes are in validation/HISTORICAL_V1_V11.md.
