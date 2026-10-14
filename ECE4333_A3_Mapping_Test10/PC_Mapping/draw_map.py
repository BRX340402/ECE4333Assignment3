#!/usr/bin/env python3
"""Rough road map from a Test10 mapping run (standard library only).

    python draw_map.py logs/scan_xxx/samples.csv
    python draw_map.py logs/scan_xxx/raw.jsonl --run 2
Options: --ref track_reference.json  --scale 3.2 (skip auto scale)  --no-closure

What it does
 1. Takes the driving samples (mode 6) of one run (the longest unless --run).
 2. Labels every sample with the nearest straight-segment heading of the figure 8
    (needs no distance calibration: gyro heading only).
 3. Distance scale = reference length / measured length of COMPLETE segments
    (corner to corner). The UNO distance uses a placeholder PWM->cm/s factor, so
    this scale also tells you the real factor (printed in summary.txt).
 4. Fits gyro drift + scale error on the straight segments and removes them.
 5. Integrates x/y, then closes each completed lap (end must equal the start corner).
Outputs next to the input: map.svg, map_points.csv, segments.csv, summary.txt
"""
import argparse, csv, json, math, os, sys

STATE_COLORS = {1: "#2a7de1", 2: "#f08c00", 3: "#d6336c", 4: "#ae3ec9", 5: "#000000"}
STATE_NAMES = {1: "FOLLOW", 2: "CROSS hold", 3: "corner TURN", 4: "SEARCH", 5: "FAIL"}
LABEL_TOL = 14.0  # deg; A-B (-210) and E-O-A (-240) differ by 30


# ---------------------------------------------------------------- loading
def decode(d):
    q, t, L, C, R, pl, pr, g10, yaw10, dist_mm, x, y, u, st, k = d[:15]
    return dict(q=q, t_ms=t, L=L, C=C, R=R, PL=pl, PR=pr, yaw_deg=yaw10 / 10.0, dist_cm=dist_mm / 10.0,
                x_cm=x, y_cm=y, ultrasonic_cm=u, mode=st >> 4, follow_state=st & 7, gyro_ok=(st >> 3) & 1,
                all_black=(k >> 3) & 1, ev_cross=(k >> 4) & 1, ev_turn=(k >> 5) & 1, ev_search=(k >> 6) & 1)


def load(path):
    rows = []
    if path.lower().endswith(".csv"):
        with open(path, newline="", encoding="utf-8") as f:
            for r in csv.DictReader(f):
                try:
                    rows.append({k: float(v) for k, v in r.items() if v not in ("", None)})
                except ValueError:
                    continue
    else:  # raw.jsonl from the logger, or plain UNO lines
        with open(path, encoding="utf-8") as f:
            for ln in f:
                ln = ln.strip()
                if not ln:
                    continue
                try:
                    o = json.loads(ln)
                except ValueError:
                    continue
                o = o.get("line", o)
                if o.get("N") == 230 and isinstance(o.get("d"), list):
                    rows.append({k: float(v) for k, v in decode(o["d"]).items()})
    return rows


def split_runs(rows):
    runs, cur, prev = [], [], None
    for r in rows:
        if int(r.get("mode", 0)) != 6:
            if cur:
                runs.append(cur); cur = []
            prev = None
            continue
        if prev is not None and (r["q"] < prev["q"] or r["t_ms"] < prev["t_ms"] or r["t_ms"] - prev["t_ms"] > 3000):
            runs.append(cur); cur = []
        cur.append(r); prev = r
    if cur:
        runs.append(cur)
    return [r for r in runs if len(r) >= 5]


# ---------------------------------------------------------------- geometry
def solve3(A, b):
    """Gaussian elimination for a 3x3 system."""
    M = [A[i][:] + [b[i]] for i in range(3)]
    for c in range(3):
        p = max(range(c, 3), key=lambda r: abs(M[r][c]))
        if abs(M[p][c]) < 1e-12:
            return None
        M[c], M[p] = M[p], M[c]
        for r in range(3):
            if r != c:
                f = M[r][c] / M[c][c]
                M[r] = [M[r][j] - f * M[c][j] for j in range(4)]
    return [M[i][3] / M[i][i] for i in range(3)]


def reference_polyline(ref):
    pts, x, y = [(0.0, 0.0)], 0.0, 0.0
    for s in ref["segments"]:
        h = math.radians(s["heading_deg"])
        x += s["length_cm"] * math.cos(h); y += s["length_cm"] * math.sin(h)
        pts.append((x, y))
    return pts


def straight_runs(run, yaw, dist, min_samples=6, tol=8.0):
    """Stable forward-driving stretches (scale-free). Pivot turns (wheels opposite)
    and searches are excluded, so headings swept during a pivot are never labelled."""
    fwd = [r["PL"] > 0 and r["PR"] > 0 and int(r.get("follow_state", 1)) in (1, 2) for r in run]
    runs, i, n = [], 0, len(run)
    while i < n:
        if not fwd[i]:
            i += 1; continue
        j, acc = i, [yaw[i]]
        while j + 1 < n and fwd[j + 1] and abs(yaw[j + 1] - sum(acc) / len(acc)) <= tol:
            j += 1; acc.append(yaw[j])
        if j - i + 1 >= min_samples:
            runs.append({"i0": i, "i1": j, "head": sum(acc) / len(acc), "len_model": dist[j] - dist[i]})
        i = j + 1
    merged = []  # join runs split by a crossing hold / wiggle (same heading)
    for r in runs:
        if merged and abs(merged[-1]["head"] - r["head"]) < 12.0:
            m = merged[-1]
            w0, w1 = m["i1"] - m["i0"] + 1, r["i1"] - r["i0"] + 1
            m["head"] = (m["head"] * w0 + r["head"] * w1) / (w0 + w1)
            m["i1"] = r["i1"]; m["len_model"] = dist[m["i1"]] - dist[m["i0"]]
        else:
            merged.append(dict(r))
    return merged


def assign_labels(runs, ref, tol=25.0):
    """Label straight runs by the known route ORDER and corner angles (drift-tolerant).
    First run = C-O-D. Each next run must turn by the expected corner angle."""
    heads = [s["heading_deg"] for s in ref["segments"]]
    nseg = len(heads)
    turn = [((heads[(l + 1) % nseg] - heads[l]) if l + 1 < nseg else (heads[0] + 0 - heads[l])) for l in range(nseg)]
    lab = None
    for k, r in enumerate(runs):
        if k == 0:
            lab = 0 if abs(r["head"] - heads[0]) < 30 else None
        else:
            prev = runs[k - 1]
            d = r["head"] - prev["head"]
            if prev["label"] is not None and abs(d - turn[prev["label"]]) <= tol:
                lab = (prev["label"] + 1) % nseg
            else:  # resync on absolute heading, offset by the last good run's drift
                ref_off = 0.0
                for q in range(k - 1, -1, -1):
                    if runs[q]["label"] is not None:
                        ref_off = runs[q]["head"] - heads[runs[q]["label"]]; break
                cand = [(abs(r["head"] - ref_off - h), i) for i, h in enumerate(heads)]
                e, i = min(cand)
                lab = i if e < 12 else None
                r["resync"] = True
        r["label"] = lab
    return runs


# ---------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input")
    ap.add_argument("--ref", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "track_reference.json"))
    ap.add_argument("--run", type=int, default=None, help="run number (1 = first Start); default longest")
    ap.add_argument("--scale", type=float, default=None, help="multiply UNO distance by this instead of auto")
    ap.add_argument("--no-closure", action="store_true")
    ap.add_argument("--outdir", default=None)
    a = ap.parse_args()

    ref = json.load(open(a.ref, encoding="utf-8"))
    nseg = len(ref["segments"])
    rows = load(a.input)
    runs = split_runs(rows)
    if not runs:
        sys.exit("No driving samples (mode 6) found in %s" % a.input)
    run = runs[a.run - 1] if a.run else max(runs, key=len)
    rid = (runs.index(run) + 1)
    out = a.outdir or os.path.dirname(os.path.abspath(a.input))
    t = [(r["t_ms"] - run[0]["t_ms"]) / 1000.0 for r in run]
    yaw = [r["yaw_deg"] - run[0]["yaw_deg"] for r in run]
    dist = [r["dist_cm"] - run[0]["dist_cm"] for r in run]
    qs = [int(r["q"]) for r in run]
    gaps = sum(max(0, (qs[i] - qs[i - 1] - 1) & 0xFFFF) for i in range(1, len(qs)))
    notes = []

    # ---- 1. straight runs labelled by route order (heading changes, drift tolerant)
    segs = [g for g in assign_labels(straight_runs(run, yaw, dist), ref) if g["label"] is not None]
    labels = [None] * len(run)
    for g in segs:
        for i in range(g["i0"], g["i1"] + 1):
            labels[i] = g["label"]
    expect_next = lambda lab: (lab + 1) % nseg
    for k, g in enumerate(segs):
        prev_ok = k > 0 and expect_next(segs[k - 1]["label"]) == g["label"] and not g.get("resync")
        next_ok = k + 1 < len(segs) and expect_next(g["label"]) == segs[k + 1]["label"] and not segs[k + 1].get("resync")
        # the first segment starts at the start corner C by definition
        g["complete"] = (prev_ok or (k == 0 and g["label"] == 0)) and next_ok
        g["in_order"] = prev_ok or next_ok or k == 0

    # ---- 2. scale
    comp = [s for s in segs if s["complete"] and s["len_model"] > 0]
    if a.scale:
        scale, how = a.scale, "given on command line"
    elif comp:
        scale = sum(ref["segments"][s["label"]]["length_cm"] for s in comp) / sum(s["len_model"] for s in comp)
        how = "auto from %d complete segment(s)" % len(comp)
    else:
        scale, how = 1.0, "NOT calibrated (no complete corner-to-corner segment) - lengths are UNO model units"
        notes.append("No complete segment: drive at least from C past corner E so a segment is bounded by two corners.")

    # ---- 3. gyro drift / scale fit on labelled straight samples: err = a + b*t + c*yaw
    A = [[0.0] * 3 for _ in range(3)]; bb = [0.0] * 3; used = 0
    for s in segs:
        if not s["in_order"]:
            continue
        h = ref["segments"][s["label"]]["heading_deg"]
        for i in range(s["i0"], s["i1"] + 1):
            if int(run[i].get("follow_state", 1)) != 1:
                continue
            v = [1.0, t[i], yaw[i]]; e = yaw[i] - h
            for p in range(3):
                bb[p] += v[p] * e
                for q in range(3):
                    A[p][q] += v[p] * v[q]
            used += 1
    fit = solve3(A, bb) if used > 30 and len(set(s["label"] for s in segs)) >= 3 else None
    if fit:
        a0, b0, c0 = fit
        yaw_c = [(yaw[i] - a0 - b0 * t[i]) / (1.0 + c0) for i in range(len(yaw))]
        gyro_txt = "gyro correction: offset %.2f deg, drift %.3f deg/s, scale error %.2f %%" % (a0, b0, c0 * 100)
    else:
        yaw_c = yaw[:]
        gyro_txt = "gyro correction: not fitted (need >=3 different straight segments)"

    # ---- 4. integrate
    xs, ys = [0.0], [0.0]
    for i in range(1, len(run)):
        ds = (dist[i] - dist[i - 1]) * scale
        h = math.radians((yaw_c[i] + yaw_c[i - 1]) / 2)
        xs.append(xs[-1] + ds * math.cos(h)); ys.append(ys[-1] + ds * math.sin(h))
    raw_err = None

    # ---- 5. loop closure at every completed lap (entering label 0 after label nseg-1)
    lap_ends = [segs[k]["i0"] for k in range(1, len(segs)) if segs[k]["label"] == 0 and segs[k - 1]["label"] == nseg - 1]
    if lap_ends and not a.no_closure:
        start_i = 0; cx, cy = 0.0, 0.0
        closure = []
        for end_i in lap_ends:
            ex, ey = xs[end_i] - cx, ys[end_i] - cy  # error vs start corner (0,0)
            closure.append(math.hypot(ex, ey))
            d0, d1 = dist[start_i], dist[end_i]
            for i in range(start_i, len(xs)):
                f = 1.0 if i >= end_i else (dist[i] - d0) / (d1 - d0 if d1 > d0 else 1)
                xs[i] -= ex * f; ys[i] -= ey * f
            start_i = end_i
        raw_err = closure

    # ---- outputs
    ref_pts = reference_polyline(ref)
    with open(os.path.join(out, "map_points.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["t_s", "s_cm", "x_cm", "y_cm", "heading_deg", "segment", "follow_state", "L", "C", "R", "all_black", "ev_cross"])
        for i, r in enumerate(run):
            lab = labels[i]
            w.writerow([round(t[i], 2), round(dist[i] * scale, 1), round(xs[i], 1), round(ys[i], 1), round(yaw_c[i], 1),
                        ref["segments"][lab]["name"] if lab is not None else "corner", int(r.get("follow_state", 0)),
                        int(r["L"]), int(r["C"]), int(r["R"]), int(r.get("all_black", 0)), int(r.get("ev_cross", 0))])
    with open(os.path.join(out, "segments.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["order", "segment", "complete", "ref_heading_deg", "meas_heading_deg", "ref_length_cm", "meas_length_cm", "length_error_cm", "t_start_s", "t_end_s"])
        for k, s in enumerate(segs):
            rs = ref["segments"][s["label"]]
            hh = [yaw_c[i] for i in range(s["i0"], s["i1"] + 1)]
            L = s["len_model"] * scale
            w.writerow([k + 1, rs["name"], int(s["complete"]), rs["heading_deg"], round(sum(hh) / len(hh), 1), rs["length_cm"],
                        round(L, 1), round(L - rs["length_cm"], 1) if s["complete"] else "", round(t[s["i0"]], 1), round(t[s["i1"]], 1)])
    write_svg(os.path.join(out, "map.svg"), ref_pts, xs, ys, run, segs, labels, scale, rid, ref)

    order = " > ".join(ref["segments"][s["label"]]["name"] + ("" if s["in_order"] else "(?)") for s in segs)
    lines = ["Test10 map summary  (run %d of %d, %d samples, %.1f s, UNO sample gaps %d)" % (rid, len(runs), len(run), t[-1], gaps),
             "distance scale: x%.3f  (%s)" % (scale, how),
             "  -> measured speed factor ~ %.3f cm/s per PWM (UNO uses 0.33); put it in MapConfig.h LEFT/RIGHT_CM_S_PER_PWM" % (0.33 * scale),
             gyro_txt,
             "laps completed: %d%s" % (len(lap_ends), "" if not raw_err else "; closure error before correction: " + ", ".join("%.1f cm" % e for e in raw_err)),
             "segment order seen: " + order,
             "crossing holds: %d, corner turns: %d, searches: %d, all-black hits: %d" % (
                 sum(int(r.get("ev_cross", 0)) for r in run), sum(int(r.get("ev_turn", 0)) for r in run),
                 sum(int(r.get("ev_search", 0)) for r in run), sum(int(r.get("all_black", 0)) for r in run))]
    if not any(v != 0 for v in yaw):
        notes.append("GYRO NOT WORKING: heading stayed exactly 0 for the whole run, so no map can be built and the car cannot recognise corners or the old track. Check IM/GY in the status (IM=0: no IMU chip found).")
    out_of_order = [s for s in segs if not s["in_order"]]
    if out_of_order:
        notes.append("Segments marked (?) are out of the expected C>D>E>A>B order: the car probably took a wrong branch or reversed there.")
    lines += ["NOTE: " + n for n in notes]
    lines.append("files: map.svg (open in a browser), map_points.csv, segments.csv")
    open(os.path.join(out, "summary.txt"), "w", encoding="utf-8").write("\n".join(lines) + "\n")
    print("\n".join(lines))


def write_svg(path, ref_pts, xs, ys, run, segs, labels, scale, rid, ref):
    allx = xs + [p[0] for p in ref_pts]; ally = ys + [p[1] for p in ref_pts]
    x0, x1, y0, y1 = min(allx) - 30, max(allx) + 30, min(ally) - 30, max(ally) + 30
    W = 900; k = W / (x1 - x0); H = int((y1 - y0) * k) + 70
    X = lambda x: (x - x0) * k
    Y = lambda y: (y1 - y) * k + 40
    o = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d" font-family="sans-serif">' % (W, H, W, H),
         '<rect width="100%" height="100%" fill="#ffffff"/>',
         '<text x="10" y="22" font-size="16" fill="#222">Test10 map, run %d - scale x%.2f - grid 50 cm</text>' % (rid, scale)]
    g = int(math.floor(x0 / 50) * 50)
    while g <= x1:
        o.append('<line x1="%.1f" y1="40" x2="%.1f" y2="%d" stroke="#eee"/>' % (X(g), X(g), H)); g += 50
    g = int(math.floor(y0 / 50) * 50)
    while g <= y1:
        o.append('<line x1="0" y1="%.1f" x2="%d" y2="%.1f" stroke="#eee"/>' % (Y(g), W, Y(g))); g += 50
    o.append('<polyline fill="none" stroke="#999" stroke-width="10" stroke-opacity="0.35" stroke-linejoin="round" points="%s"/>' %
             " ".join("%.1f,%.1f" % (X(p[0]), Y(p[1])) for p in ref_pts))
    # raw onboard estimate (UNO X/Y, same distance scale) for comparison
    if all("x_cm" in r for r in run):
        o.append('<polyline fill="none" stroke="#adb5bd" stroke-width="1" stroke-dasharray="4 3" points="%s"/>' %
                 " ".join("%.1f,%.1f" % (X(r["x_cm"] * scale), Y(r["y_cm"] * scale)) for r in run))
    for i in range(1, len(xs)):
        c = STATE_COLORS.get(int(run[i].get("follow_state", 1)), "#2a7de1")
        o.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="2.5" stroke-linecap="round"/>' % (X(xs[i - 1]), Y(ys[i - 1]), X(xs[i]), Y(ys[i]), c))
    for i, r in enumerate(run):
        if int(r.get("all_black", 0)):
            o.append('<circle cx="%.1f" cy="%.1f" r="2.5" fill="#212529"/>' % (X(xs[i]), Y(ys[i])))
    for k2, s in enumerate(segs[1:], 1):
        i = s["i0"]
        o.append('<rect x="%.1f" y="%.1f" width="7" height="7" fill="#1098ad"/>' % (X(xs[i]) - 3.5, Y(ys[i]) - 3.5))
    o.append('<circle cx="%.1f" cy="%.1f" r="6" fill="#2b8a3e"/><text x="%.1f" y="%.1f" font-size="13" fill="#2b8a3e">start C</text>' % (X(0), Y(0), X(0) + 8, Y(0) + 16))
    ly = H - 12; lx = 10
    for st, name in [(1, "FOLLOW"), (2, "CROSS hold"), (3, "corner TURN"), (4, "SEARCH")]:
        o.append('<rect x="%d" y="%d" width="14" height="4" fill="%s"/><text x="%d" y="%d" font-size="12">%s</text>' % (lx, ly - 4, STATE_COLORS[st], lx + 18, ly, name)); lx += 120
    o.append('<rect x="%d" y="%d" width="14" height="8" fill="#999" fill-opacity="0.35"/><text x="%d" y="%d" font-size="12">drawing reference</text>' % (lx, ly - 7, lx + 18, ly)); lx += 140
    o.append('<circle cx="%d" cy="%d" r="2.5" fill="#212529"/><text x="%d" y="%d" font-size="12">all 3 black</text>' % (lx + 5, ly - 3, lx + 12, ly)); lx += 100
    o.append('<rect x="%d" y="%d" width="7" height="7" fill="#1098ad"/><text x="%d" y="%d" font-size="12">corner</text>' % (lx, ly - 7, lx + 11, ly))
    o.append("</svg>")
    open(path, "w", encoding="utf-8").write("\n".join(o))


if __name__ == "__main__":
    main()
