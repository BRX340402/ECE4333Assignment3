#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir="$(mktemp -d)"
trap 'rm -rf "$build_dir"' EXIT
python3 tests/generate_v15_test.py
python3 tests/generate_routes_test.py
python3 tests/generate_uno_v13_test.py
for test in motion_v15 migration_v15_generated; do
 source_file="tests/${test}.cpp"
 if [[ "$test" == motion_v15 ]]; then source_file=tests/motion_v15_test.cpp; fi
 g++ -std=c++11 "$source_file" -o "$build_dir/$test"
 "$build_dir/$test"
done
g++ -std=c++11 -Itests/esp_syntax_stubs tests/camera_v15_generated.cpp -o "$build_dir/camera"
"$build_dir/camera"
g++ -std=c++17 tests/esp_routes_generated.cpp -o "$build_dir/routes"
"$build_dir/routes"
g++ -std=c++11 -Itests/uno_parser_stubs tests/uno_v13_generated.cpp -o "$build_dir/uno"
"$build_dir/uno"
node tests/ui_test.cjs
