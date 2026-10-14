#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash tests/run_v23_host_checks.sh
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
g++ -std=c++11 tests/pixel_zero_v24_test.cpp -o "$build_dir/test"
"$build_dir/test"
