#!/usr/bin/env bash
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
python3 "$root/tests/check_test10.py"
python3 "$root/tests/check_test10_gyro.py"
node "$root/tests/test10_ui.cjs"
g++ -std=c++11 -fsyntax-only -x c++ -I"$root/tests/esp_syntax_stubs" -I"$root/ESP32_Assignment2" "$root/ESP32_Assignment2/ESP32_Assignment2.ino"
echo 'PASS ESP32 host-stub syntax; not a target-board build'
