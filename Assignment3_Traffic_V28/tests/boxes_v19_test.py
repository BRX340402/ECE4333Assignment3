from pathlib import Path
import tempfile,subprocess,json
p=Path(__file__).resolve().parents[1]
s=(p/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text()
fn=s[s.index('String liveObjectsJSON(){'):s.index('// The only normal caller')]
prefix=r'''
#include "Arduino.h"
#include "esp_camera.h"
#include <atomic>
#include <iostream>
struct RedRegion{int components=0,pixels=0,x=0,y=0,w=0,h=0,imageW=0,imageH=0;};
struct RedStatus{bool valid=true,seen=true,greenSeen=true;uint32_t captured=0,ms=10;RedRegion region,green;}redStatus;
struct StopMeasurement{bool valid=true,seen=true,matched=true;uint32_t completed=0,captured=0,inferenceMs=800,decodeMs=20;int resolution=5;RedRegion region,displayRegion;}stopMeasurement;
bool colorEnabled=true,detectEnabled=true,resolutionBusy=false;int stateMutex=0;
std::atomic<float> lightColourK{0},greenColourK{0},stopColourK{0};
int colourRange(const RedRegion&,float){return -1;}
sensor_t sensor={};sensor_t*esp_camera_sensor_get(){return &sensor;}
'''
main=r'''
int main(){sensor.status.framesize=5;auto &r=redStatus.region;r.components=1;r.pixels=100;r.x=40;r.y=20;r.w=80;r.h=60;r.imageW=400;r.imageH=296;redStatus.green=r;redStatus.green.x=200;stopMeasurement.region=r;stopMeasurement.displayRegion=r;
std::cout<<liveObjectsJSON().s<<"\n";
resolutionBusy=true;std::cout<<liveObjectsJSON().s<<"\n";resolutionBusy=false;
colorEnabled=false;std::cout<<liveObjectsJSON().s<<"\n";
stopMeasurement.matched=false;std::cout<<liveObjectsJSON().s<<"\n";
detectEnabled=false;std::cout<<liveObjectsJSON().s<<"\n";
}
'''
with tempfile.TemporaryDirectory() as d:
 f=Path(d)/'test.cpp';f.write_text(prefix+fn+main);exe=Path(d)/'test'
 subprocess.run(['g++','-std=c++11','-I'+str(p/'tests/esp_syntax_stubs'),str(f),'-o',str(exe)],check=True)
 rows=[json.loads(x) for x in subprocess.check_output([str(exe)],text=True).splitlines()]
a,b,c,d,e=rows
assert all(a[k]['box_valid'] for k in ['red','green','stop'])
assert a['red']['x']==40 and a['green']['x']==200 and a['stop']['inference_ms']==800
assert not any(b[k]['box_valid'] for k in b)
assert c['stop']['box_valid'] and not c['red']['box_valid']
assert d['stop']['box_valid'] and d['stop']['fomo_cell'] and not e['stop']['box_valid']
print('PASS V19 production box JSON: normalized-source coordinates, three classes, resolution guards, colour-off STOP, FOMO fallback, AI-off hiding.')
