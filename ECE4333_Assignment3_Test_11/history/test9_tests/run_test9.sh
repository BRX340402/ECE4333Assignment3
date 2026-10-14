#!/usr/bin/env bash
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
bash "$root/tests/run_test8.sh"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
g++ -std=c++11 "$root/tests/test9_threshold_angles.cpp" -o "$tmp/test9"
"$tmp/test9"
python3 - "$root" <<'PY'
from pathlib import Path
import sys
r=Path(sys.argv[1]);u=(r/'UNO_Assignment2/UNO_Assignment2.ino').read_text();e=(r/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text();p=(r/'ESP32_Assignment2/TrafficPage.h').read_text()
assert 'traffic.colour(doc[PSTR("S")],v,r,g,d,millis(),calibrated,anyRecentRed)' in u
assert 'releaseRed' not in u
assert 'ra>360' in u and '"RA"),30,360' in e
assert 'id="RA" type="number" min="30" max="360"' in p
print('PASS actual UNO threshold wiring and UI/ESP32/UNO angle limits')
PY
