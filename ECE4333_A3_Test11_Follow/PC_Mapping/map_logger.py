#!/usr/bin/env python3
"""Test10 mapping logger - saves EVERY line the UNO sends (no third-party packages).

Connect the PC to the car hotspot (ECE4333_A2_V2), then run:
    python map_logger.py                 # default http://192.168.4.1
    python map_logger.py --host 192.168.4.1 --out logs
Start the car from the web page as usual. Press Ctrl+C to finish.

Output folder logs/scan_<time>/ :
    raw.jsonl     every UNO line + ESP32 index/receive time + PC time (nothing dropped)
    samples.csv   decoded N230 samples (10 Hz while driving) -> input for draw_map.py
    status.csv    N221 status lines (1 Hz), sensors.csv  N210 (battery/ultrasonic, 0.5 Hz)
The ESP32 keeps ~2000 lines (~3 min) in PSRAM, so a short Wi-Fi drop loses nothing:
the logger resumes from the last index it received.
"""
import argparse, csv, json, os, sys, time, urllib.request, urllib.error

SAMPLE_FIELDS = ["q", "t_ms", "L", "C", "R", "PL", "PR", "gyro_rate_dps", "yaw_deg", "dist_cm",
                 "x_cm", "y_cm", "ultrasonic_cm", "mode", "follow_state", "gyro_ok",
                 "L_seen", "C_seen", "R_seen", "all_black", "ev_cross", "ev_turn", "ev_search", "ev_estop"]
STATE_NAMES = {0: "IDLE", 1: "FOLLOW", 2: "CROSS", 3: "TURN", 4: "SEARCH", 5: "FAIL"}


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
    print("Logging to", folder, "- Ctrl+C to stop")

    nxt = None
    stats = {"lines": 0, "samples": 0, "esp_lost": 0, "uno_gaps": 0, "bad": 0}
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
        raw.close(); samples.close(); status.close(); sensors.close()
        with open(os.path.join(folder, "log_summary.json"), "w") as f:
            json.dump(stats, f, indent=1)
        print("\nSaved %d lines (%d samples) in %s" % (stats["lines"], stats["samples"], folder))
        print("Lost on ESP32: %d   UNO sequence gaps: %d" % (stats["esp_lost"], stats["uno_gaps"]))
        print("Next: python draw_map.py \"%s\"" % os.path.join(folder, "samples.csv"))


if __name__ == "__main__":
    main()
