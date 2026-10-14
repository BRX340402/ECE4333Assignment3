from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'ESP32_Assignment2/TrafficPage.h').read_text()
f=s[s.index('function floorThreshold('):s.index('async function applyFloor(')]
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cjs';p.write_text("const assert=require('assert');\n"+f+"\nassert.deepEqual(floorThreshold([100,180,220],[850,980,1023]),[535,1023]);assert.deepEqual(floorThreshold([900,980],[100,200]),[0,550]);assert.throws(()=>floorThreshold([500,650],[600,800]));assert.throws(()=>floorThreshold([],[]));console.log('PASS floor calibration: both polarities, sensor overlap rejection');")
 subprocess.run(['node',str(p)],check=True)
