#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tools/build_web_page.py
bash tests/run_v19_host_checks.sh
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
g++ -std=c++11 tests/red_scan_v21_test.cpp -o "$build_dir/test"
"$build_dir/test"
