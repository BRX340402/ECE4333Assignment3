from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text()
fn=s[s.index('bool cameraRange('):s.index('void printSerialResult')]
stub=r'''
#include "esp_syntax_stubs/Arduino.h"
#include "esp_syntax_stubs/esp_camera.h"
#include <atomic>
#include <map>
#include <cassert>
#include <iostream>
struct WebMock{std::map<std::string,String>args;int code=0;String body;String arg(const char*k){return args[k];}void send(int c,const char*,String b){code=c;body=b;}}web;
String sent;void sendJSON(String s){sent=s;}
int owner=1;bool sampleAvailable=true;
std::atomic<int>pendingThreshold{-1},pendingResolution{-1};std::atomic<bool>resolutionBusy{false};
int resolutionId=0,requestedResolution=5,geometryEpoch=0;
String resolutionState,resolutionMessage,cameraStatus;
int stateMutex=0,frameMutex=0,cameraMutex=0;
struct Empty{}redStatus,stopMeasurement;
std::atomic<bool>detectEnabled{true};bool inputBGR=true;int viewMode=1,viewCx=500,viewCy=500;float aiThreshold=.70f;
sensor_t sensor;int calls=0,lastValue=-1;
int setValue(sensor_t*,int value){calls++;lastValue=value;return 0;}
sensor_t*esp_camera_sensor_get(){return &sensor;}String makeCameraStatus(){return "{}";}
'''
checks=r'''
int main(){sensor.set_whitebal=net;return 0;}
'''
checks=r'''
int main(){sensor.set_whitebal=setValue;sensor.set_awb_gain=setValue;sensor.set_wb_mode=setValue;sensor.set_brightness=setValue;sensor.set_gain_ctrl=setValue;
 for(const char*k:{"awb","awb_gain","wb_mode","brightness","agc"}){web.args={{"var",k},{"val","1"}};int before=calls;cameraControl();assert(web.code==200&&calls==before+1&&lastValue==1&&!sampleAvailable&&owner==0);}
 int before=calls;web.args={{"var","brightness"},{"val","9"}};cameraControl();assert(web.code==400&&calls==before);
 web.args={{"var","threshold"},{"val","80"}};cameraControl();assert(web.code==202&&pendingThreshold==80);
 web.args={{"var","framesize"},{"val","7"}};cameraControl();assert(web.code==202&&resolutionBusy&&pendingResolution==7&&requestedResolution==7);
 cameraControl();assert(web.code==409);
 std::cout<<"PASS V8 restored camera endpoint: AWB, WB gain/mode, brightness, AGC, validation, threshold and asynchronous resolution request.\n";
}
'''
(root/'tests/camera_control_generated.cpp').write_text(stub+fn+checks)
