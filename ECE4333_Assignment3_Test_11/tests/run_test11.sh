#!/usr/bin/env bash
set -eu
root=$(cd "$(dirname "$0")/.." && pwd)
bash "$root/tests/run_test10.sh"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for name in test11_route test11_log; do
 g++ -std=c++11 "$root/tests/$name.cpp" -o "$tmp/$name"
 "$tmp/$name"
done
python3 "$root/tests/check_test11_protocol.py"
