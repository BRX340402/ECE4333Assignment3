#!/usr/bin/env python3
"""Render Test3 estimated robot path, avoidance state changes and approximate obstacles.
Standard library only. Usage: python draw_avoidance.py logs/scan_YYYYMMDD_HHMMSS
Output: avoidance_map.svg (open in a web browser).
This is dead-reckoned geometry, NOT a survey of physical ground truth.
"""
import argparse
import csv
import html
import os


def read_rows(path):
    if not os.path.isfile(path):
        return []
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def numeric(row, field):
    try:
        return float(row[field])
    except (KeyError, TypeError, ValueError):
        return None


def render(folder, out=None):
    samples = read_rows(os.path.join(folder, "samples.csv"))
    events = read_rows(os.path.join(folder, "avoidance_events.csv"))
    runs = read_rows(os.path.join(folder, "avoidance_runs.csv"))
    pose = [(numeric(r, "x_cm"), numeric(r, "y_cm"), numeric(r, "t_ms"))
            for r in samples if r.get("mode") == "6"]
    pose = [(x,y,t) for x,y,t in pose if x is not None and y is not None and t is not None]
    hits = [(numeric(e, "obstacle_x_cm"), numeric(e, "obstacle_y_cm"))
            for e in events]
    hits = [(x,y) for x,y in hits if x is not None and y is not None]
    points = [(x,y) for x,y,_ in pose] + hits
    if not points:
        raise ValueError("No autonomous pose samples or estimated obstacle coordinates in " + folder)
    left = min(x for x,y in points)-20
    right = max(x for x,y in points)+20
    bottom = min(y for x,y in points)-20
    top = max(y for x,y in points)+20
    W,H,pad = 960,700,65
    scale = min((W-2*pad)/max(1,right-left),(H-2*pad)/max(1,top-bottom))
    def xy(x,y):
        return (pad+(x-left)*scale,H-pad-(y-bottom)*scale)
    def pp(vals):
        return " ".join("%.1f,%.1f" % xy(x,y) for x,y in vals)
    parts = [
        '<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">'%(W,H,W,H),
        '<rect width="100%" height="100%" fill="#f8fafc"/>',
        '<text x="25" y="32" font-family="Arial" font-size="18" fill="#1e293b">ECE 4333 Test 3 — estimated obstacle avoidance</text>',
        '<text x="25" y="51" font-family="Arial" font-size="12" fill="#64748b">PWM/gyro dead reckoning; red markers are approximate sonar projections</text>',
    ]
    # Regular sampled path in blue.
    if len(pose)>=2:
        parts.append('<polyline fill="none" stroke="#2563eb" stroke-width="2.5" points="%s"/>' %
                     pp([(x,y) for x,y,t in pose]))
    # Highlight portions that occurred during the avoidance attempt.
    for run in runs:
        start,end = numeric(run,"start_ms"),numeric(run,"end_ms")
        if start is None or end is None:
            continue
        detour = [(x,y) for x,y,t in pose if start<=t<=end]
        if len(detour)>1:
            parts.append('<polyline fill="none" stroke="#f59e0b" stroke-width="4" points="%s"/>' % pp(detour))
    # Track state transitions: name and location come from N231.
    for e in events:
        x,y = numeric(e,"x_cm"),numeric(e,"y_cm")
        if x is None or y is None:
            continue
        X,Y=xy(x,y)
        state=e.get("to_name","?")
        special=state in ("TURN_OUT","TURN_JOIN","FAULT","CANCELLED")
        color="#dc2626" if state in ("FAULT","CANCELLED") else "#0f766e"
        parts.append('<circle cx="%.1f" cy="%.1f" r="%d" fill="%s"><title>%s at %s ms</title></circle>'%
                     (X,Y,5 if special else 3,color,html.escape(state),html.escape(e.get("t_ms",""))))
        if special:
            parts.append('<text x="%.1f" y="%.1f" font-size="11" font-family="Arial" fill="%s">%s</text>'%
                         (X+8,Y-7,color,html.escape(state)))
    # Mark obstacle estimates only at detection, not on every state change.
    for x,y in hits:
        X,Y=xy(x,y)
        parts.append('<circle cx="%.1f" cy="%.1f" r="7" fill="#dc2626" opacity="0.8"/>'%(X,Y))
        parts.append('<text x="%.1f" y="%.1f" font-size="11" fill="#991b1b" font-family="Arial">Obstacle est.</text>'%(X+10,Y+4))
    parts.append('<text x="25" y="%d" font-size="12" font-family="Arial" fill="#334155">Blue: estimated route   Orange: avoidance interval   Green: state transitions   Red: obstacle estimate or fault</text>'%(H-18))
    parts.append('</svg>')
    out = out or os.path.join(folder,"avoidance_map.svg")
    with open(out,"w",encoding="utf-8") as f:
        f.write("\n".join(parts))
    return out


if __name__=="__main__":
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument("folder",help="log folder containing samples.csv and avoidance_events.csv")
    ap.add_argument("--out",default=None)
    args=ap.parse_args()
    print("Saved:",render(args.folder,args.out))
