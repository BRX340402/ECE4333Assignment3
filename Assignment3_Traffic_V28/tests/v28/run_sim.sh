#!/usr/bin/env bash
# Host simulation of the real V28 firmware headers (PoseEstimate.h, TrackPlan.h) on a simulated robot.
set -e; cd "$(dirname "$0")"
g++ -O2 -std=c++17 -DA3_HOST_SIM -I../../UNO_Assignment2 sim_v28.cpp -o /tmp/sim_v28
for a in "--speed=1.0" "--speed=0.95" "--speed=0.88" "--speed=1.05" "--speed=1.12 --events=0 --trim=0" "--bias=2.0" "--width=1.15" "--track=1.04"; do echo "== $a"; /tmp/sim_v28 $a --out=/tmp/sim_run.csv; done
