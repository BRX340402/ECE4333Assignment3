
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
bool cameraRange(const String &key,int val){
 if(key=="framesize")return val==0||(val>=3&&val<=10);
 if(key=="quality")return val>=10&&val<=63;
 if(key=="brightness"||key=="contrast"||key=="saturation"||key=="ae_level")return val>=-2&&val<=2;
 if(key=="agc_gain")return val>=0&&val<=30;
 if(key=="aec_value")return val>=0&&val<=1200;
 if(key=="special_effect"||key=="gainceiling")return val>=0&&val<=6;
 if(key=="wb_mode")return val>=0&&val<=4;
 if(key=="view_mode")return val>=0&&val<=3;
 if(key=="view_cx"||key=="view_cy")return val>=0&&val<=1000;
 if(key=="threshold")return val>=50&&val<=99;
 return val==0||val==1;
}
void cameraControl(){
 String key=web.arg("var");if(key.startsWith("face_")||key=="color_detect"){web.send(410,"text/plain","Not included: red, green and STOP only");return;}String value=web.arg("val");char *end=nullptr;long n=strtol(value.c_str(),&end,10);
 if(!value.length()||value.length()>6||*end||!cameraRange(key,n)){web.send(400,"text/plain","Invalid setting/value");return;}
 sendJSON("{\"N\":100}");owner=0;sampleAvailable=false;
 if(key=="threshold"){pendingThreshold.store((int)n);web.send(202,"text/plain","Queued for next frame; check Applied threshold");return;}
 if(key=="framesize"){
  if(resolutionBusy.exchange(true)){web.send(409,"text/plain","A resolution change is already pending");return;}
  xSemaphoreTake(stateMutex,portMAX_DELAY);resolutionId++;requestedResolution=n;resolutionState="queued";resolutionMessage="Waiting for current inference to finish";uint32_t id=resolutionId;xSemaphoreGive(stateMutex);
  pendingResolution=n;web.send(202,"application/json",String("{\"id\":")+String(id)+"}");return;
 }
 if(resolutionBusy){web.send(503,"text/plain","Resolution change pending; retry shortly");return;}
 if(xSemaphoreTake(frameMutex,pdMS_TO_TICKS(20))!=pdTRUE){web.send(503,"text/plain","Vision busy; retry");return;}
 if(xSemaphoreTake(cameraMutex,pdMS_TO_TICKS(20))!=pdTRUE){xSemaphoreGive(frameMutex);web.send(503,"text/plain","Camera busy; retry shortly");return;}
 sensor_t *s=esp_camera_sensor_get();int val=n,res=0;
 if(key=="quality")res=s->set_quality(s,val);
 else if(key=="contrast")res=s->set_contrast(s,val);
 else if(key=="brightness")res=s->set_brightness(s,val);
 else if(key=="saturation")res=s->set_saturation(s,val);
 else if(key=="gainceiling")res=s->set_gainceiling(s,(gainceiling_t)val);
 else if(key=="colorbar")res=s->set_colorbar(s,val);
 else if(key=="awb")res=s->set_whitebal(s,val);
 else if(key=="agc")res=s->set_gain_ctrl(s,val);
 else if(key=="aec")res=s->set_exposure_ctrl(s,val);
 else if(key=="hmirror")res=s->set_hmirror(s,val);
 else if(key=="vflip")res=s->set_vflip(s,val);
 else if(key=="awb_gain")res=s->set_awb_gain(s,val);
 else if(key=="agc_gain")res=s->set_agc_gain(s,val);
 else if(key=="aec_value")res=s->set_aec_value(s,val);
 else if(key=="aec2")res=s->set_aec2(s,val);
 else if(key=="dcw")res=s->set_dcw(s,val);
 else if(key=="bpc")res=s->set_bpc(s,val);
 else if(key=="wpc")res=s->set_wpc(s,val);
 else if(key=="raw_gma")res=s->set_raw_gma(s,val);
 else if(key=="lenc")res=s->set_lenc(s,val);
 else if(key=="special_effect")res=s->set_special_effect(s,val);
 else if(key=="wb_mode")res=s->set_wb_mode(s,val);
 else if(key=="ae_level")res=s->set_ae_level(s,val);

 else if(key=="detect")detectEnabled=val;
 else if(key=="input_bgr")inputBGR=val;
 else if(key=="view_mode")viewMode=val;
 else if(key=="view_cx")viewCx=val;
 else if(key=="view_cy")viewCy=val;
 else if(key=="threshold")aiThreshold=val/100.0f;
 else res=-1;
 if(!res){xSemaphoreTake(stateMutex,portMAX_DELAY);geometryEpoch++;redStatus={};stopMeasurement={};xSemaphoreGive(stateMutex);}
 xSemaphoreTake(stateMutex,portMAX_DELAY);cameraStatus=makeCameraStatus();xSemaphoreGive(stateMutex);
 xSemaphoreGive(cameraMutex);
 xSemaphoreGive(frameMutex);
 web.send(res?400:200,"text/plain",res?"Unsupported setting / sensor rejected value":"Applied; vehicle stopped");
}


int main(){sensor.set_whitebal=setValue;sensor.set_awb_gain=setValue;sensor.set_wb_mode=setValue;sensor.set_brightness=setValue;sensor.set_gain_ctrl=setValue;
 for(const char*k:{"awb","awb_gain","wb_mode","brightness","agc"}){web.args={{"var",k},{"val","1"}};int before=calls;cameraControl();assert(web.code==200&&calls==before+1&&lastValue==1&&!sampleAvailable&&owner==0);}
 int before=calls;web.args={{"var","brightness"},{"val","9"}};cameraControl();assert(web.code==400&&calls==before);
 web.args={{"var","threshold"},{"val","80"}};cameraControl();assert(web.code==202&&pendingThreshold==80);
 web.args={{"var","framesize"},{"val","7"}};cameraControl();assert(web.code==202&&resolutionBusy&&pendingResolution==7&&requestedResolution==7);
 cameraControl();assert(web.code==409);
 std::cout<<"PASS V8 restored camera endpoint: AWB, WB gain/mode, brightness, AGC, validation, threshold and asynchronous resolution request.\n";
}
