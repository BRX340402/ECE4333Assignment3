#!/usr/bin/env python3
"""ECE 4333 A3: save ESP32 /traffic-status pose snapshots to Excel-compatible CSV.

Only Python standard library is needed. Does not control the motors.
Usage: python a3_wifi_logger.py --seconds 120 --out run01
       python a3_wifi_logger.py --url http://192.168.4.1/traffic-status --seconds 60 --out test
"""
import argparse
import csv
import json
import time
from pathlib import Path
from urllib.request import Request, urlopen

FIELDS = [
    'pc_elapsed_s', 'run_t_s', 'uno_ms', 'mode', 'traffic_state',
    'pred_x_cm', 'pred_y_cm', 'pred_heading_deg',
    'fused_x_cm', 'fused_y_cm', 'fused_heading_deg',
    'gyro_yaw_deg', 'gyro_available', 'pose_calibrated', 'camera_correction',
    'motor_left_pwm', 'motor_right_pwm', 'line_left_adc', 'line_middle_adc',
    'line_right_adc', 'estop_latched', 'fault_code', 'search_phase',
    'red_seen', 'green_seen', 'stop_sign_seen', 'telemetry_age_ms',
    'red_x_px', 'red_y_px', 'green_x_px', 'green_y_px',
    'stop_x_px', 'stop_y_px', 'stop_width_px', 'stop_image_w_px', 'stop_distance_cm'
]
EVENT_FIELDS = ['pc_elapsed_s','run_t_s','uno_ms','event','mode','traffic_state','fault_code','estop_latched']

def sample_to_row(payload, elapsed, run_start):
    u = payload.get('uno') or {}
    if not isinstance(u, dict) or 'UT' not in u:
        return None, run_start
    ut = int(u['UT'])
    if int(u.get('M', 0)) == 6 and run_start is None:
        run_start = ut
    objs = payload.get('objects') or {}
    def detected(key):
        q = objs.get(key) or {}
        return int(bool(q.get('valid') and q.get('seen')))
    row = dict(
        pc_elapsed_s=round(elapsed, 3),
        run_t_s='' if run_start is None else round((ut-run_start)/1000, 3),
        uno_ms=ut, mode=u.get('M',''), traffic_state=u.get('TS',''),
        pred_x_cm=u.get('KX',''), pred_y_cm=u.get('KY',''), pred_heading_deg=u.get('KH',''),
        fused_x_cm=u.get('X',''), fused_y_cm=u.get('Y',''), fused_heading_deg=u.get('H',''),
        gyro_yaw_deg=u.get('YA',''), gyro_available=u.get('GY',''), pose_calibrated=u.get('PV',''),
        camera_correction=u.get('CF',''), motor_left_pwm=u.get('PL',''), motor_right_pwm=u.get('PR',''),
        line_left_adc=u.get('L',''), line_middle_adc=u.get('C',''), line_right_adc=u.get('R',''),
        estop_latched=u.get('ES',''), fault_code=u.get('FC',''), search_phase=u.get('SP',''),
        red_seen=detected('red'), green_seen=detected('green'), stop_sign_seen=detected('stop'),
        telemetry_age_ms=payload.get('age_ms',''),
        red_x_px=(objs.get('red') or {}).get('x',''),
        red_y_px=(objs.get('red') or {}).get('y',''),
        green_x_px=(objs.get('green') or {}).get('x',''),
        green_y_px=(objs.get('green') or {}).get('y',''),
        stop_x_px=(objs.get('stop') or {}).get('x',''),
        stop_y_px=(objs.get('stop') or {}).get('y',''),
        stop_width_px=(objs.get('stop') or {}).get('w',''),
        stop_image_w_px=(objs.get('stop') or {}).get('image_w',''),
        stop_distance_cm=(objs.get('stop') or {}).get('distance_cm','')
    )
    return row, run_start

def fetch_json(url):
    with urlopen(Request(url, headers={'Cache-Control':'no-cache'}), timeout=3) as response:
        return json.load(response)

def record(seconds, interval, url, prefix, fetch=fetch_json):
    prefix = Path(prefix)
    prefix.parent.mkdir(parents=True,exist_ok=True)
    pose_path=prefix.with_name(prefix.name+'_pose.csv')
    events_path=prefix.with_name(prefix.name+'_events.csv')
    raw_path=prefix.with_name(prefix.name+'_raw.jsonl')
    started=time.monotonic()
    count=0;run_start=None;last_uno=None;last_event=None
    with pose_path.open('w',newline='',encoding='utf-8-sig') as pf, events_path.open('w',newline='',encoding='utf-8-sig') as ef, raw_path.open('w',encoding='utf-8') as rf:
        p=csv.DictWriter(pf,fieldnames=FIELDS);p.writeheader()
        e=csv.DictWriter(ef,fieldnames=EVENT_FIELDS);e.writeheader()
        while time.monotonic()-started<seconds:
            elapsed=time.monotonic()-started
            try:
                data=fetch(url)
                row,run_start=sample_to_row(data,elapsed,run_start)
                if row is None:
                    print('Waiting for UNO telemetry...',flush=True)
                elif row['uno_ms']!=last_uno and (row['telemetry_age_ms']=='' or int(row['telemetry_age_ms'])<1500):
                    p.writerow(row);pf.flush()
                    rf.write(json.dumps(data,ensure_ascii=False)+'\n');rf.flush()
                    count+=1;last_uno=row['uno_ms']
                    event=(row['traffic_state'],row['estop_latched'],row['fault_code'])
                    if event!=last_event:
                        e.writerow(dict(pc_elapsed_s=row['pc_elapsed_s'],run_t_s=row['run_t_s'],uno_ms=row['uno_ms'],event='STATE_OR_SAFETY_CHANGE',mode=row['mode'],traffic_state=row['traffic_state'],fault_code=row['fault_code'],estop_latched=row['estop_latched']))
                        ef.flush();last_event=event
                    if count%10==1:
                        print('Saved',count,'samples; state',row['traffic_state'],'time',row['run_t_s'],'s',flush=True)
            except (OSError,ValueError,KeyError) as error:
                print('Connection/telemetry warning:',error,flush=True)
            time.sleep(interval)
    return pose_path,events_path,raw_path,count

def main():
    ap=argparse.ArgumentParser(description='Log ECE4333 robot telemetry over ESP32 Wi-Fi to CSV')
    ap.add_argument('--seconds',type=float,default=120)
    ap.add_argument('--interval',type=float,default=0.45,help='Polling interval in seconds')
    ap.add_argument('--url',default='http://192.168.4.1/traffic-status')
    ap.add_argument('--out',default='run01',help='Output prefix, not .csv filename')
    a=ap.parse_args()
    if a.seconds<=0 or a.interval<0.2:ap.error('seconds must be positive, interval >=0.2 seconds')
    paths=record(a.seconds,a.interval,a.url,a.out)
    print('Finished:',paths[3],'unique records')
    for path in paths[:3]:print(' ',path)

if __name__=='__main__':main()
