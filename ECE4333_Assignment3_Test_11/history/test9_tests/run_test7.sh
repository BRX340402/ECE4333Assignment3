#!/usr/bin/env bash
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
bash "$root/tests/run_test5.sh"
python3 "$root/tests/check_test6_navigation.py"
