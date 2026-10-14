# V11 validation — driver defaults

PASS host full-sketch syntax; UI read-only startup and default-reset request; actual cameraInit tested with all ISP setter pointers null; init failure/null sensor/frame-size failure handled; reset endpoint stops vehicle, disables analysis and queues reboot. Capturing and 96x96 model are unchanged. No board compile or hardware validation. Historical records below describe earlier versions only.

# V10 validation

96x96 original model archive restored; deployment1 / class3 metadata verified. Camera init and pins identical to uploaded FullUI. Full host syntax, UI and 108 decoder cases pass. Hardware color/noise cause remains unconfirmed. Earlier records below are historical.

# V9 validation — 2026-10-09

Reference: user-uploaded Assignment2_Camera_FullUI.rar. cameraInit is byte-identical and camera_pins.h matches. Defaults and initial geometry now use QVGA 320x240 (framesize 4). Raw JPEG preview path and unified UI retained. Host syntax and UI regression checks passed. No ESP32 target build or physical color/network verification available.

Earlier validation records (historical) follow:

# V8 validation — 2026-10-09

PASS: cameraInit byte-identical to current retrieved FAST19 source; full ESP32 host syntax with mocks; dashboard controls and explicit AWB preset; preview failure retention regression. Extracted cameraControl tests verify AWB/WB gain/WB mode/brightness/AGC calls, invalid value rejection, threshold queue, resolution queue and busy rejection. No target-board build or physical camera/network validation.

Run additional endpoint test from package root:
```
python3 tests/generate_camera_control_test.py
g++ -std=c++11 -fsanitize=address,undefined -Itests/esp_syntax_stubs tests/camera_control_generated.cpp -o /tmp/a3_camera
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_camera
```

Earlier validation records (historical) follow:

# V7 validation — 2026-10-09

PASS: full sketch host syntax; JS tests including503, network failure,204, corrupt-JPEG decode, last-frame retention/recovery and URL cleanup; extracted preview handler tested with partial writes, duplicate frames, stale cache and zero-byte send failure. Camera initialization byte-identical to deliveredV6. No target-board compilation or physical network test.

Earlier validation records follow:

# V6 validation — 2026-10-09

V6 restores original camera initialization profile: 10 MHz, JPEG, UXGA allocation, SVGA runtime, quality10, fb_count2, flip0/mirror0. No additional ISP setters; runtime /control returns410. UI has no camera-reset controls. Full sketch host syntax, UI and existing calibration/measurement/decoder scenarios pass; no target compiler or hardware verification available. SVGA makes old CIF calibration inactive by resolution matching.

The following describes retained V5 tests and their scope:

# V5 validation — 2026-10-09

Passed on host:
- Existing motion scenarios: immediate start, repeated STOP/red/green circuits, stop latch, right90/left180 recovery, intersection straight priority and heading lock.
- 108 image dimension/crop combinations: model RGB sampling is identical to the prior full-RGB reference; concurrent same-frame red extent and area match a separate reference computation. C++ ASAN/UBSAN, leak check disabled.
- Extracted ESP32 HTTP handlers: red, green and STOP independently scan/save; STOP requires AI confirmation; freeze/token semantics; NVS serialization; save failure; resolution mismatch; selected-profile clear; running-state restrictions and Start guard.
- Actual compact metrics function: three distance scales, STOP matching requirement, stale/absent target handling, clipped and wrong-resolution rejection.
- Dashboard JavaScript VM: auto preview request to port81; JS tab switch retains preview; compact metrics suppress stale distances; all three calibration selections; saved width/distance display; Start/Stop/heartbeat.
- Complete ESP32 sketch host syntax with mocked Arduino/camera/Wi-Fi/model APIs. This is NOT a target build or hardware test.
- Current Assignment2 FAST19 comparison: camera initialization, Config, camera pins, AIAdapter, model shape and arena code unchanged.

Not verified:
- ESP32/AVR target compile, final RAM/flash usage or on-device timing. Prior target compiler is unavailable. Historical V1 binaries/logs are not V5 build results.
- Live camera noise, physical driving, Wi-Fi throughput, FPS, model accuracy or multi-task hardware stability.
- Rendered browser layout: headless Chromium executable unavailable; JavaScript tests and source checks passed, no rendered screenshot claimed.

Reproduce from package root:
```
g++ -std=c++11 -fsanitize=address,undefined tests/traffic_test.cpp -o /tmp/a3_traffic
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_traffic
g++ -std=c++11 -fsanitize=address,undefined -Itests/decoder_stubs tests/decoder_test.cpp -o /tmp/a3_decoder
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_decoder
python3 tests/generate_routes_test.py
g++ -std=c++11 -fsanitize=address,undefined tests/esp_routes_generated.cpp -o /tmp/a3_routes
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_routes
python3 tests/generate_measurement_test.py
g++ -std=c++11 -fsanitize=address,undefined tests/measurement_generated.cpp -o /tmp/a3_metrics
ASAN_OPTIONS=detect_leaks=0 /tmp/a3_metrics
node tests/ui_test.cjs
g++ -x c++ -std=c++11 -fsyntax-only -Itests/esp_syntax_stubs ESP32_Assignment2/ESP32_Assignment2.ino
```

Mocks test code paths, not actual SDK compatibility or board performance. Use Arduino IDE ESP32 core1.0.4, ESP32 Dev Module, Huge APP, PSRAM enabled; then verify on the board.
