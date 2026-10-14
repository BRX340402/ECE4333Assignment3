#!/usr/bin/env bash
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
bash "$root/tests/run_test11.sh"
python3 "$root/tests/check_test12_start.py"
