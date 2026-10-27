#!/usr/bin/env python3
"""Assignment 4 Test3 logger - saves every UNO telemetry line (no third-party packages).

Connect the PC to the car hotspot (ECE4333_A2_V2), then run:
    python map_logger.py                 # default http://192.168.4.1
    python map_logger.py --host 192.168.4.1 --out logs
Start the car from the web page as usual. Press Ctrl+C to finish.

Output folder logs/scan_<time>/ :
    raw.jsonl     every UNO line + ESP32 index/receive time + PC time (nothing dropped)
    samples.csv   decoded N230 samples (10 Hz while driving) -> input for draw_map.py
    status.csv    N221 status lines, sensors.csv N210 battery/ultrasonic
    avoidance_events.csv  N231 A4 avoidance transitions and approximate obstacle points
    avoidance_runs.csv    outcome/duration/estimated distance per avoidance attempt
The ESP32 keeps ~2000 lines (~3 min) in PSRAM, so a short Wi-Fi drop loses nothing:
the logger resumes from the last index it received.
"""
import argparse, csv, json, math, os, sys, time, urllib.request, urllib.error

SAMPLE_FIELDS = ["q", "t_ms", "L", "C", "R", "PL", "PR", "gyro_rate_dps", "yaw_deg", "dist_cm",
                 "x_cm", "y_cm", "ultrasonic_cm", "mode", "follow_state", "gyro_ok",
                 "L_seen", "C_seen", "R_seen", "all_black", "ev_cross", "ev_turn", "ev_search", "ev_estop"]
STATE_NAMES = {0: "IDLE", 1: "FOLLOW", 2: "CROSS", 3: "TURN", 4: "SEARCH", 5: "FAIL"}
A4_STATES = {
    0: "TRACK", 1: "WAIT", 2: "TURN_OUT", 3: "SIDE_SCAN",
    4: "SIDE_FRONT", 5: "SIDE_STEP", 6: "SIDE_MARGIN",
    7: "TURN_ALONG", 8: "PASS_SCAN", 9: "PASS_FRONT",
    10: "PASS_STEP", 11: "REAR_MARGIN", 12: "TURN_IN",
    13: "RETURN_STEP", 14: "TURN_JOIN", 15: "FAULT", 16: "CANCELLED",
}

def decode_a4_event(d):
    """N231 compact transition packet; position/heading are estimates, not truth."""
    if not isinstance(d, list) or len(d) != 15:
        raise ValueError("N231 requires 15 entries")
    t, run, before, after, x, y, yaw10, dist10, cm, pan, seg, side, sd10, pd10, lost = [int(v) for v in d]
    # Projection is an estimate: sensor forward/at robot origin assumed.
    visible = before == 0 and after in (1, 2) and cm >= 0 and pan == 90
    theta = math.radians(yaw10 / 10.0)
    ox = round(x + cm * math.cos(theta)) if visible else ""
    oy = round(y + cm * math.sin(theta)) if visible else ""
    return dict(
        t_ms=t, run=run, from_state=before, to_state=after,
        from_name=A4_STATES.get(before, "UNKNOWN"),
        to_name=A4_STATES.get(after, "UNKNOWN"),
        x_cm=x, y_cm=y, yaw_deg=yaw10/10.0, odometry_cm=dist10/10.0,
        ultrasonic_cm=cm, pan_deg=pan, segment=seg, outside_side=side,
        sideways_cm=sd10/10.0, along_cm=pd10/10.0,
        obstacle_x_cm=ox,
        obstacle_y_cm=oy,
        event_queue_drops=lost,
    )

def summarize_avoidance(events):
    """Pair each TRACK->avoidance entry with its finish; never invent a success."""
    results, pending = [], {}
    for event in events:
        run = event["run"]
        old, new = event["from_state"], event["to_state"]
        if old == 0 and new in (1, 2):
            if run in pending:
                previous = pending.pop(run)
                previous["outcome"] = "incomplete"
                results.append(previous)
            pending[run] = dict(
                run=run, start_ms=event["t_ms"], start_x_cm=event["x_cm"],
                start_y_cm=event["y_cm"], obstacle_x_cm=event["obstacle_x_cm"],
                obstacle_y_cm=event["obstacle_y_cm"], start_odometry_cm=event["odometry_cm"],
                outcome="incomplete",
            )
        if run not in pending:
            continue
        outcome = ("success" if old == 14 and new == 0 else
                   "cleared_without_detour" if old == 1 and new == 0 else
                   "fault" if new == 15 else "cancelled" if new == 16 else None)
        if outcome:
            row = pending.pop(run)
            row.update(outcome=outcome, end_ms=event["t_ms"],
                       duration_s=round(((event["t_ms"]-row["start_ms"]) & 0xffffffff)/1000, 3),
                       end_x_cm=event["x_cm"], end_y_cm=event["y_cm"],
                       estimated_travel_cm=round(event["odometry_cm"]-row["start_odometry_cm"],2),
                       measured_side_estimate_cm=event["sideways_cm"],
                       measured_pass_estimate_cm=event["along_cm"])
            results.append(row)
    results.extend(pending.values())
    return results



def decode_sample(d):
    """N230 'd' array -> dict (see UNO_Mapping.ino scheduleTx)."""
    q, t, L, C, R, pl, pr, g10, yaw10, dist_mm, x, y, u, st, k = d[:15]
    return {"q": q, "t_ms": t, "L": L, "C": C, "R": R, "PL": pl, "PR": pr,
            "gyro_rate_dps": g10 / 10.0, "yaw_deg": yaw10 / 10.0, "dist_cm": dist_mm / 10.0,
            "x_cm": x, "y_cm": y, "ultrasonic_cm": u, "mode": st >> 4, "follow_state": st & 7, "gyro_ok": (st >> 3) & 1,
            "L_seen": k & 1, "C_seen": (k >> 1) & 1, "R_seen": (k >> 2) & 1, "all_black": (k >> 3) & 1,
            "ev_cross": (k >> 4) & 1, "ev_turn": (k >> 5) & 1, "ev_search": (k >> 6) & 1, "ev_estop": (k >> 7) & 1}


def flatten(prefix, obj, out):
    for key, val in obj.items():
        if isinstance(val, dict):
            flatten(prefix + key + ".", val, out)
        elif isinstance(val, list):
            out[prefix + key] = json.dumps(val)
        else:
            out[prefix + key] = val


class DictCsv:
    """CSV whose header grows if new fields appear (rewritten at the end)."""
    def __init__(self, path, fields=None):
        self.path, self.rows, self.fields = path, [], list(fields or [])
        self.f = open(path, "w", newline="", encoding="utf-8")
        self.w = None
        if fields:
            self.w = csv.DictWriter(self.f, fieldnames=self.fields, extrasaction="ignore")
            self.w.writeheader()

    def add(self, row):
        new = [k for k in row if k not in self.fields]
        if self.w is None or new:
            self.fields += new
            self.rows.append(row)
            self._rewrite()
            return
        self.rows.append(row)
        self.w.writerow(row)
        self.f.flush()

    def _rewrite(self):
        self.f.close()
        self.f = open(self.path, "w", newline="", encoding="utf-8")
        self.w = csv.DictWriter(self.f, fieldnames=self.fields, extrasaction="ignore")
        self.w.writeheader()
        self.w.writerows(self.rows)
        self.f.flush()

    def close(self):
        self.f.close()


def fetch(host, start, timeout):
    url = "http://%s/map-log?from=%d&max=200" % (host, start)
    with urllib.request.urlopen(url, timeout=timeout) as r:
        return r.read().decode("utf-8", "replace")


def parse_header(line):
    out = {}
    for part in line.lstrip("#").split():
        if "=" in part:
            k, v = part.split("=", 1)
            out[k] = v
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="192.168.4.1")
    ap.add_argument("--out", default="logs")
    ap.add_argument("--interval", type=float, default=0.25, help="poll period, s")
    ap.add_argument("--from-start", action="store_true",
                    help="also download what the ESP32 already buffered before the logger started")
    a = ap.parse_args()

    folder = os.path.join(a.out, time.strftime("scan_%Y%m%d_%H%M%S"))
    os.makedirs(folder, exist_ok=True)
    raw = open(os.path.join(folder, "raw.jsonl"), "w", encoding="utf-8")
    samples = DictCsv(os.path.join(folder, "samples.csv"), ["pc_time", "esp_idx", "esp_ms"] + SAMPLE_FIELDS)
    status = DictCsv(os.path.join(folder, "status.csv"))
    sensors = DictCsv(os.path.join(folder, "sensors.csv"))
    events = DictCsv(os.path.join(folder, "avoidance_events.csv"))
    event_rows = []
    print("Logging to", folder, "- Ctrl+C to stop")

    nxt = None
    stats = {"lines": 0, "samples": 0, "events": 0, "esp_lost": 0, "uno_gaps": 0, "bad": 0}
    last_q, last_print, last_ok, last_status = None, 0.0, time.time(), {}
    last_sample = {}
    try:
        while True:
            try:
                body = fetch(a.host, nxt if nxt is not None else 0, timeout=3)
            except (urllib.error.URLError, OSError) as e:
                if time.time() - last_ok > 2:
                    print("\r[no connection: %s] retrying...            " % getattr(e, "reason", e), end="")
                time.sleep(0.5)
                continue
            last_ok = time.time()
            lines = body.split("\n")
            hdr = parse_header(lines[0]) if lines and lines[0].startswith("#") else {}
            total, oldest = int(hdr.get("lines", 0)), int(hdr.get("oldest", 0))
            if nxt is None:  # first contact
                nxt = oldest if a.from_start else total
                if not a.from_start:
                    time.sleep(a.interval)
                    continue
            if total < nxt:  # ESP32 restarted: its counter went back to 0
                print("\nESP32 restarted - continuing from its new buffer")
                nxt = 0
                continue
            first_idx = None
            for ln in lines[1:]:
                if not ln.strip():
                    continue
                parts = ln.split("\t", 2)
                if len(parts) != 3:
                    stats["bad"] += 1
                    continue
                idx, esp_ms, text = int(parts[0]), int(parts[1]), parts[2]
                if first_idx is None:
                    first_idx = idx
                    if idx > nxt:
                        stats["esp_lost"] += idx - nxt
                        print("\nWARNING: %d lines were overwritten on the ESP32 before download" % (idx - nxt))
                nxt = idx + 1
                pc = time.time()
                try:
                    obj = json.loads(text)
                except ValueError:
                    stats["bad"] += 1
                    raw.write(json.dumps({"idx": idx, "esp_ms": esp_ms, "pc": pc, "text": text}) + "\n")
                    continue
                raw.write(json.dumps({"idx": idx, "esp_ms": esp_ms, "pc": pc, "line": obj}) + "\n")
                stats["lines"] += 1
                n = obj.get("N")
                if n == 230 and isinstance(obj.get("d"), list) and len(obj["d"]) >= 15:
                    s = decode_sample(obj["d"])
                    if last_q is not None and s["q"] != (last_q + 1) & 0xFFFF and s["q"] != 0:
                        stats["uno_gaps"] += (s["q"] - last_q - 1) & 0xFFFF
                    last_q = s["q"]
                    row = {"pc_time": round(pc, 3), "esp_idx": idx, "esp_ms": esp_ms}
                    row.update(s)
                    samples.add(row)
                    stats["samples"] += 1
                    last_sample = s
                elif n == 231:
                    try:
                        event = decode_a4_event(obj.get("d"))
                    except (ValueError, TypeError):
                        stats["bad"] += 1
                        continue
                    row = {"pc_time": round(pc,3), "esp_idx": idx, "esp_ms": esp_ms}
                    row.update(event)
                    events.add(row)
                    event_rows.append(event)
                    stats["events"] += 1
                elif n in (221, 210):
                    row = {"pc_time": round(pc, 3), "esp_idx": idx, "esp_ms": esp_ms}
                    flatten("", obj, row)
                    (status if n == 221 else sensors).add(row)
                    if n == 221:
                        last_status = obj
            raw.flush()
            if nxt is not None and nxt < total:
                continue  # more waiting - fetch immediately
            if time.time() - last_print > 1:
                last_print = time.time()
                s = last_sample
                print("\rsamples %d | lost on ESP %d | UNO seq gaps %d | mode %s %s | yaw %s deg | dist %s cm | L/C/R %s/%s/%s | fault %s   " % (
                    stats["samples"], stats["esp_lost"], stats["uno_gaps"], s.get("mode", "-"),
                    STATE_NAMES.get(s.get("follow_state"), "-"), s.get("yaw_deg", "-"), s.get("dist_cm", "-"),
                    s.get("L", "-"), s.get("C", "-"), s.get("R", "-"), last_status.get("FC", "-")), end="")
            time.sleep(a.interval)
    except KeyboardInterrupt:
        pass
    finally:
        raw.close(); samples.close(); status.close(); sensors.close(); events.close()
        runs = summarize_avoidance(event_rows)
        with open(os.path.join(folder, "avoidance_runs.csv"), "w", newline="", encoding="utf-8") as f:
            fields = ["run","outcome","start_ms","end_ms","duration_s",
                      "start_x_cm","start_y_cm","end_x_cm","end_y_cm",
                      "obstacle_x_cm","obstacle_y_cm","start_odometry_cm",
                      "estimated_travel_cm","measured_side_estimate_cm","measured_pass_estimate_cm"]
            writer = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
            writer.writeheader()
            writer.writerows(runs)
        stats["attempts"] = len(runs)
        stats["successes"] = sum(r["outcome"] == "success" for r in runs)
        with open(os.path.join(folder, "log_summary.json"), "w") as f:
            json.dump(stats, f, indent=1)
        print("\nSaved %d lines (%d pose samples, %d obstacle events) in %s" % (stats["lines"], stats["samples"], stats["events"], folder))
        print("Lost on ESP32: %d   UNO sequence gaps: %d" % (stats["esp_lost"], stats["uno_gaps"]))
        print("Next: python draw_map.py \"%s\"" % os.path.join(folder, "samples.csv"))


if __name__ == "__main__":
    main()
