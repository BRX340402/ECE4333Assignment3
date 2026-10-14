#!/usr/bin/env bash
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
build_tmp=$(mktemp -d)
trap 'rm -rf "$build_tmp"' EXIT
python3 "$root/tests/check_test4.py"
python3 "$root/tests/check_test5_floor.py"
for name in test1_behavior test2_green_release test5_search; do
 g++ -std=c++11 "$root/tests/$name.cpp" -o "$build_tmp/$name"
 "$build_tmp/$name"
done
g++ -std=c++11 -fsyntax-only -x c++ -I"$root/tests/esp_syntax_stubs" -I"$root/ESP32_Assignment2" "$root/ESP32_Assignment2/ESP32_Assignment2.ino"
echo 'PASS ESP32 host-stub syntax (not ESP32 target compilation).'
