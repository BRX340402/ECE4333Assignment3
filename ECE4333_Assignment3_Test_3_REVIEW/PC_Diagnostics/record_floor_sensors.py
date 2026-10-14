"""Read-only floor-sensor measurements from the Test 1 ESP32 webpage.
Requires Python 3 standard library and laptop connected to robot Wi-Fi.
Does NOT command the motors; keep robot OFF/standby and wheels safe.
"""
import csv
import json
import statistics
import time
from pathlib import Path
from urllib.request import urlopen

URL = 'http://192.168.4.1/traffic-status'
RESULT = Path(__file__).with_name('floor_sensor_samples.csv')
STAGES = [
    ('white_floor', 'Place all sensors over ordinary light floor, away from black tape.'),
    ('black_center', 'Place the CENTER sensor over black tape, others over light floor.'),
    ('lifted', 'Lift the car carefully so all three sensors face empty space; secure it first.'),
]

def one():
    with urlopen(URL, timeout=4) as response:
        data = json.load(response)
    u = data.get('uno') or {}
    if data.get('age_ms', 999999) > 1500:
        raise RuntimeError('UNO telemetry stale; check power and UART connection')
    if u.get('TS') != 0 or u.get('M') != 0:
        raise RuntimeError('Robot is not OFF/standby. Press STOP CAR before measuring.')
    return u


def main():
    print('Read-only ECE4333 A3 Test 1 floor-sensor calibration')
    print('Connect laptop to ECE4333_A2_V2; keep car stopped and secure.')
    records = []
    for stage, instructions in STAGES:
        print('\n' + instructions)
        input('When safely positioned, press ENTER to record 10 samples...')
        triples=[]
        for i in range(10):
            u=one()
            v=(int(u['L']), int(u['C']), int(u['R']))
            triples.append(v)
            records.append([stage, i+1, int(u.get('UT', 0)), *v])
            print(f'  {stage} {i+1:02}: L={v[0]:4} C={v[1]:4} R={v[2]:4}')
            time.sleep(.35)
        avg=[round(statistics.mean(row[k] for row in triples),1) for k in range(3)]
        print('  Mean L/C/R:', *avg)
    with RESULT.open('w', newline='', encoding='utf-8') as handle:
        writer=csv.writer(handle)
        writer.writerow(['surface','sample','uno_ms','L','C','R'])
        writer.writerows(records)
    print('\nSaved:', RESULT)
    print('Open the file in Excel. Share it before changing BL/BH thresholds.')

if __name__=='__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, RuntimeError) as exc:
        print('ERROR:', exc)
        print('No motion commands were sent. Check Wi-Fi, UNO standby and telemetry.')
