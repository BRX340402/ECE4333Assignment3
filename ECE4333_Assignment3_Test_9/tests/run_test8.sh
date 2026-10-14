#!/usr/bin/env bash
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
bash "$root/tests/run_test7.sh"
python3 "$root/tests/check_test8_connection.py"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
g++ -std=c++11 "$root/tests/test8_handover.cpp" -o "$tmp/handover"
"$tmp/handover"
