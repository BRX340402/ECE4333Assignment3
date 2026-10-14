from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
src=(root/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text()
start=src.index('struct VisionPayload {')
payload=src[start:src.index('\n',start)]
publish=src[src.index('void publish(){'):src.index('String results(){')]
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#define A2_ENABLE_AI 1
#define portMAX_DELAY 0
uint32_t now=5000;uint32_t millis(){return now;}
void xSemaphoreTake(int,int){} void xSemaphoreGive(int){}
int stateMutex=0,visionQueue=0,trafficMode=6,analysisW=400,analysisH=296;
uint16_t sequence=10,frameTrafficRequest=7;
bool frameValid=true,detectEnabled=true;uint32_t frameTime=2500;
struct {bool rs=true;} driveSettings;
struct Box {int x=200,y=100,x0=180,y0=80,x1=220,y1=120;};
struct Detection {Box box;int id=1,distance=-1;float confidence=0.65f;} detections[1];
int objectCount=1;
struct Region {int imageW=400,imageH=296,x=180,y=80,w=40,h=40;};
struct RedStatus {bool valid=true,seen=true;uint32_t captured=2500;Region region;} redStatus;
'''
checks=r'''
int main(){
 // Production publish must preserve a displayed detection after slow inference.
 publish();assert(sent.confidence==650&&sent.request==7&&sent.valid);
 redStatus.valid=false;redStatus.seen=false;publish();assert(sent.confidence==650);
 redStatus.valid=redStatus.seen=true;redStatus.captured=now;redStatus.region.x=0;
 publish();assert(sent.confidence==650); // shifted colour box must not veto this scan
 driveSettings.rs=false;publish();assert(sent.confidence==0); // legacy association retained
 redStatus.region.x=180;publish();assert(sent.confidence==650);
 redStatus.captured=2500;publish();assert(sent.confidence==0);
 driveSettings.rs=true;objectCount=0;publish();assert(sent.confidence==0);
 TrafficLogic t;t.stopOnRed=true;t.cruiseSpeed=60;t.slowSpeed=30;t.start(100);
 t.colour(1,true,true,false,-1,110);assert(!t.moving());
 t.inference(t.request,true,true,200);t.inference(t.request,true,false,250);
 assert(!t.moving());t.inference(t.request,true,true,300);
 assert(t.state==TrafficLogic::CROSS&&t.crossingPWM()==60);
 CrossingControl c;int l=0,r=0;c.arm(0);
 c.step(0,false,false,true,false,300,0,1500,l,r,t.crossingPWM());assert(l==60&&r==60);
 t.colour(2,true,true,false,-1,310);assert(t.state==TrafficLogic::CROSS);
 t.stopOnRed=false;assert(t.slowPWM()==30&&t.crossingPWM()==30);
 t.slowSpeed=0;assert(t.crossingPWM()==60);
 puts("PASS V26 production publish: stale/missing/shifted red cannot veto stop-on-red STOP; legacy match preserved; two-hit release restarts at cruise60, reduced30 retained.");
}
'''
code=prefix+payload+'\nVisionPayload sent;void xQueueOverwrite(int,VisionPayload*v){sent=*v;}\n'+publish+'\n#include "TrafficLogic.h"\n#include "CrossingControl.h"\n'+checks
with tempfile.TemporaryDirectory() as d:
 p=Path(d)/'test.cpp';p.write_text(code)
 subprocess.run(['g++','-std=c++11','-I'+str(root/'UNO_Assignment2'),str(p),'-o',d+'/test'],check=True)
 subprocess.run([d+'/test'],check=True)
