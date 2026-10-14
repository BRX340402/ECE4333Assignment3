#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tools/build_web_page.py
bash tests/run_v24_host_checks.sh
d="$(mktemp -d)"
trap 'rm -rf "$d"' EXIT
g++ -std=c++11 tests/a3_green_120_search_test.cpp -o "$d/a3"
"$d/a3"
python3 tests/stop_release_v26.py
