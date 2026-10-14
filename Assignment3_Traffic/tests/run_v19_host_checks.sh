#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash tests/run_v15_host_checks.sh
g++ -x c++ -std=c++11 -fsyntax-only -Itests/esp_syntax_stubs ESP32_Assignment2/ESP32_Assignment2.ino
python3 tests/watchdog_v19_test.py
python3 tests/boxes_v19_test.py
