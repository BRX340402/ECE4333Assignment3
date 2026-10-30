#!/usr/bin/env bash
# Host-side checks for the Test13 (Assignment 4) build (no hardware needed).
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd); tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
g++ -std=c++14 -w -I"$root/tests/uno_host_stubs" -I"$root/UNO_Mapping" "$root/tests/uno_protocol_test.cpp" -o "$tmp/proto" && "$tmp/proto" | tail -1
g++ -std=c++14 -w -DFOLLOW_ENABLED=1 -I"$root/tests/uno_host_stubs" -I"$root/UNO_Mapping" "$root/tests/uno_protocol_test.cpp" -o "$tmp/protof" && "$tmp/protof" | tail -1 | sed 's/^/(with target following compiled in) /'
g++ -std=c++14 -I"$root/UNO_Mapping" "$root/tests/follow_test.cpp" -o "$tmp/ft" && "$tmp/ft" | tail -1
g++ -std=c++14 -I"$root/UNO_Mapping" "$root/tests/route_hint_test.cpp" -o "$tmp/rh" && "$tmp/rh" | tail -1
g++ -O2 -std=c++14 -I"$root/UNO_Mapping" "$root/tests/sim_track.cpp" -o "$tmp/sim"
"$tmp/sim" | tail -1
"$tmp/sim" --stress | tail -1
"$tmp/sim" --inverted | tail -1
g++ -O2 -std=c++14 -I"$root/UNO_Mapping" "$root/tests/ovt_sim.cpp" -o "$tmp/ovt" && "$tmp/ovt" | grep "OVERTAKING SIM"
g++ -std=c++11 -fsyntax-only -x c++ -I"$root/tests/esp_syntax_stubs" -I"$root/ESP32_Assignment2" "$root/ESP32_Assignment2/ESP32_Assignment2.ino" && echo "PASS ESP32 host-stub syntax (not a target compile)"
"$tmp/sim" --dump "$tmp/run.jsonl" >/dev/null && python3 "$root/PC_Mapping/draw_map.py" "$tmp/run.jsonl" --outdir "$tmp" | sed -n 2,5p
node "$root/tests/test_map_core.js" "$tmp/run.jsonl" | sed -n 2p | sed 's/^/web page map core: /'
node "$root/tests/test_map_core.js" "$tmp/run.jsonl" | grep -E "^(PASS|FAIL)" | sed 's/^/web page Test13: /'

g++ -std=c++14 -I"$root/UNO_Mapping" "$root/tests/a4_regression.cpp" -o "$tmp/a4reg"
"$tmp/a4reg"
node "$root/tests/a4_map_regression.js"

node "$root/tests/a4_small_obstacle_ui.js"
