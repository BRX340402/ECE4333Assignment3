#include "ResultTiming.h"
#include <Arduino.h>
#include <atomic>
// ECE 4333 Assignment 3 TEST 1: V27 protocol + independent green detection.
#include <Preferences.h>
#include <cmath>
#include "Config.h"
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include "JpegBuffer.h"
#include "camera_pins.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <esp_heap_caps.h>
#include "CameraDecode.h"
#include "AIAdapter.h"
#include "TrafficPage.h"
#include "TrafficPageGzip.h"
#include "PageTransfer.h"
#include "CalibrationStore.h"
#include "CaptureCheck.h"
#include "RedCamera.h"
#include "RedAction.h"
#include "JpegSize.h"
WebServer web(80);
WebServer previewWeb(81); // JPEG transfer cannot block control/heartbeat on port 80.
int analysisW=400,analysisH=300;
bool inputBGR=true;
ViewMap aiView=makeView(400,300,1,500,500);
int viewMode=1,viewCx=500,viewCy=500;
float aiThreshold=AI_THRESHOLD,bestScore=0;
std::atomic<int> pendingThreshold{-1};
std::atomic<int> pendingResolution{-1};
std::atomic<bool> resolutionBusy{false};
uint32_t resolutionId=0;int requestedResolution=7;
String resolutionState="idle",resolutionMessage="No resolution change requested";
int verifiedWidth=800,verifiedHeight=600;
int stopCount=0,photoW=800,photoH=600;
int calibrationResolution=7;
uint16_t geometryEpoch=0;
String cameraStatus="{}";
bool cameraRestartPending=false;
uint32_t cameraRestartRequestedAt=0;

Preferences calibration;
bool calibrationReady=false;
float distanceK[5]={STOP_K,0,0,0,0}; // stop, red, blue, green, orange
int calibrationSlot(int id){return id==1?0:(id>=6&&id<=9?id-5:-1);}
const char *calibrationKeys[5]={"stop_fomo6","red","blue","green","orange"};
bool usableSize(const ColorBlob &b){return b.pixels && b.x1-b.x0+1>=6 && b.x0>0 && b.y0>0 && b.x1<analysisW-1 && b.y1<analysisH-1;}

struct Detection { ColorBlob box;int color,id,distance;float confidence;uint32_t inference_ms;int aiX,aiY; const char *sizeReason;int borderRays,textPixels;bool sizeOk; } detections[8];
uint8_t *rgb=nullptr;
void prepareEiArena();bool eiArenaInternal();
uint32_t decodeMs=0,cycleMs=0;
std::atomic<bool> previewEnabled{true}; // Browser display flag; capture stays active for detection.
camera_fb_t *frame=nullptr;
uint8_t objectCount=0;uint16_t sequence=0;
bool cameraOK=false,rgbOK=false;int aiError=0;
bool frameValid=false;int lineX=-1;bool lineValid=false;
uint32_t captureMs=0,processMs=0,totalInferenceMs=0,frameTime=0,dspMs=0,nnMs=0;
SemaphoreHandle_t frameMutex,stateMutex,cameraMutex,decoderMutex;
struct RedStatus {bool valid=false,seen=false,greenSeen=false;uint32_t captured=0,ms=0;uint16_t seq=0;RedRegion region,green;};
RedStatus redStatus;
std::atomic<int> redMinPixels{100};
std::atomic<int> trafficMode{0},trafficState{0},trafficRequest{0};
std::atomic<uint32_t> trafficReceived{0},verifySince{0};
std::atomic<float> stopColourK{0},lightColourK{0},greenColourK{0};
struct StopMeasurement {bool valid=false,seen=false,matched=false;uint32_t completed=0,captured=0;int resolution=-1;uint32_t inferenceMs=0,decodeMs=0;RedRegion region,displayRegion;};
StopMeasurement stopMeasurement;
uint8_t *aiRedCells=nullptr;uint16_t *aiRedQueue=nullptr;RedRegion inferenceRed;
uint8_t *previewJpeg=nullptr;size_t previewLen=0,previewCapacity=0;uint32_t previewCaptured=0;
uint16_t previewEpoch=0;
int previewWidth=0,previewHeight=0;
uint32_t cameraFrameCount=0,cameraFrameErrors=0;
std::atomic<int> colourCalResolution{-1};
// Focal length normalized by image width: measure before using bearing fusion.
static const float A3_FX_NORMALIZED=0.0f;
void sendJSON(const String &s);
struct DriveSettings {int sc=25,bl=600,bh=1023,ra=120,sa=360,ct=1500,gs=0,cs=60,rs=0,ds=40,rpx=100;};
DriveSettings driveSettings;int settingsID=1;bool settingsApplied=false;uint32_t settingsSent=0;
String driveSettingsJSON(){char c[160];snprintf(c,sizeof(c),"{\"SC\":%d,\"BL\":%d,\"BH\":%d,\"RA\":%d,\"SA\":%d,\"CT\":%d,\"GS\":%d,\"CS\":%d,\"RS\":%d,\"DS\":%d,\"RP\":%d,\"applied\":%d}",driveSettings.sc,driveSettings.bl,driveSettings.bh,driveSettings.ra,driveSettings.sa,driveSettings.ct,driveSettings.gs,driveSettings.cs,driveSettings.rs,driveSettings.ds,driveSettings.rpx,settingsApplied);return String(c);}
void sendDriveSettings(){char c[150];snprintf(c,sizeof(c),"{\"N\":206,\"ID\":%d,\"SC\":%d,\"BL\":%d,\"BH\":%d,\"RA\":%d,\"SA\":%d,\"CT\":%d,\"GS\":%d,\"CS\":%d,\"RS\":%d,\"DS\":%d}",settingsID,driveSettings.sc,driveSettings.bl,driveSettings.bh,driveSettings.ra,driveSettings.sa,driveSettings.ct,driveSettings.gs,driveSettings.cs,driveSettings.rs,driveSettings.ds);sendJSON(String(c));settingsSent=millis();}
CalibrationStore savedCalibration;
CalibrationSample pendingCalibration;String pendingProfile="";int sampleToken=0;bool sampleAvailable=false;
void syncColourCalibration(){
 int res=esp_camera_sensor_get()->status.framesize;colourCalResolution=res;
 stopColourK=savedCalibration.stop.resolution==res?savedCalibration.stop.k():0;
 lightColourK=savedCalibration.light.resolution==res?savedCalibration.light.k():0;
 greenColourK=savedCalibration.green.resolution==res?savedCalibration.green.k():0;
}
String calibrationJSON(){
 String out="{";
 for(int i=0;i<3;i++){
  const CalibrationSample &v=i==0?savedCalibration.stop:(i==1?savedCalibration.light:savedCalibration.green);char b[200];
  snprintf(b,sizeof(b),"%s\"%s\":{\"valid\":%d,\"width\":%ld,\"image_w\":%ld,\"distance_cm\":%ld,\"framesize\":%ld,\"active\":%d}",i?",":"",(i==0?"stop":(i==1?"light":"green")),v.valid(),(long)v.width,(long)v.imageWidth,(long)v.cm,(long)v.resolution,v.valid()&&v.resolution==esp_camera_sensor_get()->status.framesize);out+=b;

 }
 return out+"}";
}
String trafficTelemetry="{}";int unoFirmware=0;
uint8_t *greenCells=nullptr;
uint8_t *redCells=nullptr;uint16_t *redQueue=nullptr;
camera_fb_t copiedFrame={};size_t copiedCapacity=0;
void releaseFrame(){frame=nullptr;} // AI owns copiedFrame.buf and reuses it.
// Read a private copy while the capture task may replace the shared JPEG cache.
bool copyPreview(camera_fb_t &out,size_t &capacity,uint32_t &captured,uint16_t &epoch,uint32_t after=0){
 bool ok=false;
 xSemaphoreTake(stateMutex,portMAX_DELAY);
 if(!resolutionBusy&&previewJpeg&&previewLen&&previewEpoch==geometryEpoch&&
    millis()-previewCaptured<1200&&(!after||previewCaptured!=after)&&
    reserveJpeg(out.buf,capacity,previewLen)){
  memcpy(out.buf,previewJpeg,previewLen);out.len=previewLen;
  out.width=previewWidth;out.height=previewHeight;out.format=PIXFORMAT_JPEG;
  captured=previewCaptured;epoch=previewEpoch;ok=true;
 }
 xSemaphoreGive(stateMutex);return ok;
}
uint32_t copiedCaptured=0;
camera_fb_t *copyCameraFrame(){
 uint16_t epoch=0;
 return copyPreview(copiedFrame,copiedCapacity,copiedCaptured,epoch)?&copiedFrame:nullptr;
}

QueueHandle_t visionQueue;
String cachedResults="{\"seq\":0,\"ai\":0,\"objects\":[]}",unoStatus="{}",lastReply="";
uint32_t lastUno=0;
std::atomic<bool> detectEnabled{true},colorEnabled{true};

String unoRx;
uint32_t appLastSeen=0,appHeartbeat=0;
int owner=0; // 0 none/local, 1 browser, 2 official APP
struct VisionPayload { uint16_t seq;bool ai,valid;int confidence,distance,cx,cy,w,h;uint32_t captured;bool observed;uint32_t completed;uint16_t request; };
void sendJSON(const String &s){Serial2.println(s);}
uint16_t frameTrafficRequest=0;
void publish(){
  VisionPayload v={sequence,(bool)A2_ENABLE_AI,frameValid,0,-1,-1,-1,0,0,frameTime,false,millis(),0};
  v.request=frameTrafficRequest;
  if(!detectEnabled)v.ai=false;
  for(int i=0;i<objectCount;i++){
    Detection &d=detections[i];
    if(d.id==1 && d.confidence*1000>v.confidence){
      v.observed=frameValid&&detectEnabled;
      v.confidence=(int)(d.confidence*1000);v.distance=d.distance;
      v.cx=d.box.x;v.cy=d.box.y;v.w=d.box.x1-d.box.x0+1;v.h=d.box.y1-d.box.y0+1;
    }
  }
  // Stop-on-red already latched the red event. Count accepted STOP detections
  // from this scan even when inference outlives the colour frame or its box jitters.
  if(trafficMode==6&&!driveSettings.rs){
   xSemaphoreTake(stateMutex,portMAX_DELAY);RedStatus r=redStatus;xSemaphoreGive(stateMutex);
   float nx=v.cx/(float)analysisW,ny=v.cy/(float)analysisH;
   bool matched=r.valid&&r.seen&&millis()-r.captured<1200&&r.region.imageW>0&&r.region.imageH>0&&
    nx>=r.region.x/(float)r.region.imageW&&nx<=(r.region.x+r.region.w)/(float)r.region.imageW&&
    ny>=r.region.y/(float)r.region.imageH&&ny<=(r.region.y+r.region.h)/(float)r.region.imageH;
   if(!matched)v.confidence=0;
  }
  xQueueOverwrite(visionQueue,&v);
}
String results(){
  String s;s.reserve(3500);
  s="{\"seq\":"+String(sequence)+",\"device_ms\":"+String(frameTime)+",\"ai\":"+String(A2_ENABLE_AI)+",\"valid\":"+String(frameValid);
  s+=",\"frame_age_ms\":"+String(millis()-frameTime)+",\"capture_ms\":"+String(captureMs)+",\"process_ms\":"+String(processMs);
  s+=",\"dsp_ms\":"+String(dspMs)+",\"nn_ms\":"+String(nnMs);
  s+=",\"inference_ms\":"+String(totalInferenceMs)+",\"heap_free\":"+String(ESP.getFreeHeap())+",\"psram_free\":"+String(ESP.getFreePsram());
  s+=",\"decode_ms\":"+String(decodeMs)+",\"cycle_ms\":"+String(cycleMs)+",\"arena_internal\":"+String(eiArenaInternal())+",\"preview_enabled\":"+String((bool)previewEnabled);
  s+=",\"line_x\":"+String(lineX)+",\"line_valid\":"+String(lineValid)+",\"objects\":[";
  for(int i=0;i<objectCount;i++){
    Detection &d=detections[i];ColorBlob &b=d.box;if(i)s+=',';
    s+="{\"index\":"+String(i)+",\"color\":"+String(d.color)+",\"id\":"+String(d.id)+",\"label\":\""+className(d.id)+"\",\"confidence\":"+String(d.confidence,4);
    s+=",\"cx\":"+String(b.x)+",\"cy\":"+String(b.y)+",\"x\":"+String(b.x0)+",\"y\":"+String(b.y0)+",\"w\":"+String(b.x1-b.x0+1)+",\"h\":"+String(b.y1-b.y0+1);
    bool measured=d.sizeOk;int slot=calibrationSlot(d.id);
    const char *reason=!measured?(d.sizeReason?d.sizeReason:"no_matched_red_region"):(!usableSize(b)?"clipped_or_too_small":(slot<0||(d.id==1?stopColourK.load():distanceK[slot])<=0?"not_calibrated":(d.distance<0?"out_of_range":"ok")));
    s+=",\"ai_cx\":"+String(d.aiX)+",\"ai_cy\":"+String(d.aiY);
    s+=",\"size_source\":\""+String(d.id==1?("FOMO_centroid_plus_red_region"):"colour_region")+"\",\"size_valid\":"+String(d.sizeOk&&usableSize(b));
    s+=",\"distance_valid\":"+String(d.distance>=0)+",\"distance_reason\":\""+reason+"\"";
    s+=",\"border_rays\":"+String(d.borderRays)+",\"text_pixels\":"+String(d.textPixels);
    s+=",\"pixels\":"+String(b.pixels)+",\"distance_cm\":"+String(d.distance)+",\"inference_ms\":"+String(d.inference_ms)+"}";
  }
  s+="],\"camera_ok\":"+String(cameraOK)+",\"rgb_ok\":"+String(rgbOK)+",\"ai_error\":"+String(aiError);
  s+=",\"heap_largest\":"+String(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
  s+=",\"image_w\":"+String(analysisW)+",\"image_h\":"+String(analysisH)+",\"photo_w\":"+String(photoW)+",\"photo_h\":"+String(photoH);
  s+=",\"best_score\":"+String(bestScore,4)+",\"threshold\":"+String(aiThreshold,2)+",\"model_min_score\":0.5,\"stop_count\":"+String(stopCount);
  s+=",\"raw_diagnostic\":{\"available\":"+String(rawDiagnostic.available&&frameValid&&detectEnabled)+",\"count\":"+String(rawDiagnostic.count);
  s+=",\"score\":"+String(rawDiagnostic.score,6)+",\"x\":"+String(rawDiagnostic.x,3)+",\"y\":"+String(rawDiagnostic.y,3);
  s+=",\"w\":"+String(rawDiagnostic.w,3)+",\"h\":"+String(rawDiagnostic.h,3)+",\"space\":\"model_96x96\",\"before_score_filter_and_merging\":true,\"kind\":\"fomo_cell\"}";
  s+=",\"roi\":{\"x\":"+String(aiView.x)+",\"y\":"+String(aiView.y)+",\"w\":"+String(aiView.w)+",\"h\":"+String(aiView.h)+"}";
  s+=",\"framesize\":"+String((int)esp_camera_sensor_get()->status.framesize)+",\"epoch\":"+String(geometryEpoch)+",\"calibration_k\":[";
  for(int i=0;i<5;i++){if(i)s+=',';s+=String(distanceK[i],2);}
  return s+"]}";
}
int colourRange(const RedRegion &r,float k);
void acquire(){
 uint32_t start=millis();frameTrafficRequest=(uint16_t)trafficRequest.load();frameValid=false;objectCount=0;lineValid=false;lineX=-1;sequence++;
 decodeMs=0;totalInferenceMs=0;dspMs=nnMs=0;aiError=0;cameraOK=false;rgbOK=false;bestScore=0;stopCount=0;rawDiagnostic={};inferenceRed={};
 releaseFrame();
 frame=copyCameraFrame();frameTime=frame?copiedCaptured:millis();captureMs=millis()-start;
 cameraOK=frame && frame->format==PIXFORMAT_JPEG && frame->len>0;
 if(cameraOK){photoW=frame->width;photoH=frame->height;}
 uint32_t ds=millis();
 xSemaphoreTake(decoderMutex,portMAX_DELAY);
 bool decoded=cameraOK && decodeModelInput(frame,rgb,analysisW,analysisH,aiView,viewMode,viewCx,viewCy,aiRedCells,aiRedQueue,&inferenceRed,redMinPixels.load());
 xSemaphoreGive(decoderMutex);
 if(!decoded){processMs=millis()-start;xSemaphoreTake(stateMutex,portMAX_DELAY);stopMeasurement={};xSemaphoreGive(stateMutex);publish();return;}
 decodeMs=millis()-ds;rgbOK=true;frameValid=true;aiPixels=rgb;
 StopPrediction stop={};
 if(detectEnabled){if(!inferFrame(rgb,stop))frameValid=false;totalInferenceMs=stop.duration;dspMs=stop.dspMs;nnMs=stop.nnMs;aiError=stop.error;bestScore=stop.best;}
 stopCount=stop.count;
 for(int i=0;i<stop.count;i++){
  Detection &d=detections[objectCount++];d.box=stop.box[i];d.color=0;d.id=1;d.confidence=stop.confidence[i];d.inference_ms=stop.duration;d.distance=-1;d.aiX=stop.box[i].x;d.aiY=stop.box[i].y;
  d.sizeOk=inferenceRed.components==1&&inferenceRed.pixels>=redMinPixels.load()&&
   d.aiX>=inferenceRed.x&&d.aiX<inferenceRed.x+inferenceRed.w&&d.aiY>=inferenceRed.y&&d.aiY<inferenceRed.y+inferenceRed.h;
  d.sizeReason=d.sizeOk?"matched_red_region":"no_matching_red_region";
  d.borderRays=d.textPixels=0;
  if(d.sizeOk){const RedRegion&r=inferenceRed;d.box={d.aiX,d.aiY,r.x,r.y,r.x+r.w-1,r.y+r.h-1,(uint32_t)r.pixels};d.distance=colourRange(r,stopColourK.load());}
 }
 StopMeasurement measured;measured.valid=frameValid&&detectEnabled;measured.seen=measured.valid&&stop.count>0;
 measured.completed=millis();measured.captured=frameTime;measured.resolution=frame->width>0?esp_camera_sensor_get()->status.framesize:-1;
 measured.region=inferenceRed;measured.inferenceMs=totalInferenceMs;measured.decodeMs=decodeMs;
 if(objectCount){const ColorBlob &b=detections[0].box;RedRegion &v=measured.displayRegion;v.x=b.x0;v.y=b.y0;v.w=b.x1-b.x0+1;v.h=b.y1-b.y0+1;v.imageW=analysisW;v.imageH=analysisH;}
 for(int i=0;i<objectCount;i++)if(detections[i].sizeOk){measured.matched=true;measured.displayRegion=inferenceRed;break;}
 xSemaphoreTake(stateMutex,portMAX_DELAY);stopMeasurement=measured;xSemaphoreGive(stateMutex);
 processMs=millis()-start;publish();
}

bool cameraInit(){
  camera_config_t c={};c.ledc_channel=LEDC_CHANNEL_0;c.ledc_timer=LEDC_TIMER_0;
  c.pin_d0=Y2_GPIO_NUM;c.pin_d1=Y3_GPIO_NUM;c.pin_d2=Y4_GPIO_NUM;c.pin_d3=Y5_GPIO_NUM;
  c.pin_d4=Y6_GPIO_NUM;c.pin_d5=Y7_GPIO_NUM;c.pin_d6=Y8_GPIO_NUM;c.pin_d7=Y9_GPIO_NUM;
  c.pin_xclk=XCLK_GPIO_NUM;c.pin_pclk=PCLK_GPIO_NUM;c.pin_vsync=VSYNC_GPIO_NUM;c.pin_href=HREF_GPIO_NUM;
  c.pin_sscb_sda=SIOD_GPIO_NUM;c.pin_sscb_scl=SIOC_GPIO_NUM;c.pin_pwdn=PWDN_GPIO_NUM;c.pin_reset=RESET_GPIO_NUM;
  // Profile from the supplied ESP32-WROVER-Camera.zip, not FAST19/FullUI.
  c.xclk_freq_hz=10000000;c.pixel_format=PIXFORMAT_JPEG;
  if(psramFound()){c.frame_size=FRAMESIZE_UXGA;c.jpeg_quality=10;c.fb_count=2;}
  else{c.frame_size=FRAMESIZE_SVGA;c.jpeg_quality=12;c.fb_count=1;}
  if(esp_camera_init(&c)!=ESP_OK)return false;
  sensor_t*s=esp_camera_sensor_get();
  // V16: remain at V12 runtime resolution; user lowers it from the web UI.
  if(!s||s->set_framesize(s,FRAMESIZE_SVGA)!=0)return false;
  // Vendor's final orientation (its intermediate mirror=1 is overwritten).
  if(s->set_vflip(s,0)!=0||s->set_hmirror(s,0)!=0)return false;
  // No extra AWB, WB mode, exposure, gain, saturation or effect writes.
  return true;
}
void restoreCameraDefaults(){
 sendJSON("{\"N\":100}");owner=0;sampleAvailable=false;
 detectEnabled=false;colorEnabled=false;
 cameraRestartRequestedAt=millis();cameraRestartPending=true;
 web.send(202,"text/plain","Restoring SVGA camera defaults. ESP32 will restart; reconnect Wi-Fi and refresh this page. Calibration remains saved.");
}
void loadCalibration(int resolution){
 calibrationResolution=resolution;for(int i=0;i<5;i++)distanceK[i]=0;
 if(calibrationReady && calibration.getInt("res",-1)==resolution)for(int i=0;i<5;i++){float k=calibration.getFloat(calibrationKeys[i],0);if(std::isfinite(k)&&k>=0&&k<=1000)distanceK[i]=k;}
}
String makeCameraStatus(){
 sensor_t *s=esp_camera_sensor_get();String j="{";
 j+="\"framesize\":"+String((int)s->status.framesize);
 j+=",\"quality\":"+String((int)s->status.quality);
 j+=",\"brightness\":"+String((int)s->status.brightness);
 j+=",\"contrast\":"+String((int)s->status.contrast);
 j+=",\"saturation\":"+String((int)s->status.saturation);
 j+=",\"special_effect\":"+String((int)s->status.special_effect);
 j+=",\"wb_mode\":"+String((int)s->status.wb_mode);
 j+=",\"awb\":"+String((int)s->status.awb);
 j+=",\"awb_gain\":"+String((int)s->status.awb_gain);
 j+=",\"aec\":"+String((int)s->status.aec);
 j+=",\"aec2\":"+String((int)s->status.aec2);
 j+=",\"ae_level\":"+String((int)s->status.ae_level);
 j+=",\"aec_value\":"+String((int)s->status.aec_value);
 j+=",\"agc\":"+String((int)s->status.agc);
 j+=",\"agc_gain\":"+String((int)s->status.agc_gain);
 j+=",\"gainceiling\":"+String((int)s->status.gainceiling);
 j+=",\"bpc\":"+String((int)s->status.bpc);
 j+=",\"wpc\":"+String((int)s->status.wpc);
 j+=",\"raw_gma\":"+String((int)s->status.raw_gma);
 j+=",\"lenc\":"+String((int)s->status.lenc);
 j+=",\"vflip\":"+String((int)s->status.vflip);
 j+=",\"hmirror\":"+String((int)s->status.hmirror);
 j+=",\"dcw\":"+String((int)s->status.dcw);
 j+=",\"colorbar\":"+String((int)s->status.colorbar);
 j+=",\"detect\":"+String((bool)detectEnabled)+",\"input_bgr\":"+String(inputBGR)+",\"view_mode\":"+String(viewMode)+",\"view_cx\":"+String(viewCx)+",\"view_cy\":"+String(viewCy)+",\"threshold\":"+String((int)(aiThreshold*100+0.5f));
 j+=",\"sensor_pid\":"+String(s->id.PID)+",\"firmware\":\"A3 Test 1 / V27 baseline / STOP 96x96 / no face\"}";return j;
}
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

void printSerialResult(const String &json){
#if A2_SERIAL_MODE == 1
 int best=-1;for(int i=0;i<objectCount;i++)if(detections[i].id==1&&(best<0||detections[i].confidence>detections[best].confidence))best=i;
 // One labelled numeric row per completed inference, also when no STOP is found.
 bool found=frameValid&&best>=0;
 Serial.printf("Valid:%d\tSTOP:%d\tConfidence_pct:%.1f\tRaw_pct:%.1f\tCellWidth_px:%d\tCellHeight_px:%d\tDistance_cm:%d\tInference_ms:%lu\tAI_error:%d\n",
  frameValid,found,found?detections[best].confidence*100:0,
  frameValid&&rawDiagnostic.available?rawDiagnostic.score*100:0,
  found?detections[best].box.x1-detections[best].box.x0+1:0,
  found?detections[best].box.y1-detections[best].box.y0+1:0,
  -1,(unsigned long)totalInferenceMs,aiError);
#elif A2_SERIAL_MODE == 2
 Serial.println(json);
#else
 Serial.printf("\nFrame %u | STOP %d | AI error %d\nCapture %lu ms | Decode %lu ms | DSP %lu ms | NN %lu ms | Total %lu ms | Arena %s\n",sequence,stopCount,aiError,(unsigned long)captureMs,(unsigned long)decodeMs,(unsigned long)dspMs,(unsigned long)nnMs,(unsigned long)processMs,eiArenaInternal()?"internal":"PSRAM");
 for(int i=0;i<objectCount;i++){Detection &d=detections[i];Serial.printf("STOP %.1f%% x=%d y=%d w=%d h=%d distance=%d cm\n",d.confidence*100,d.box.x0,d.box.y0,d.box.x1-d.box.x0+1,d.box.y1-d.box.y0+1,d.distance);}

#endif
}

// Snapshot storage is separate from preview and updated only on explicit request.
std::atomic<bool> snapshotPending{false};
uint32_t snapshotId=0;
uint8_t *snapshotJpeg=nullptr,*snapshotBmp=nullptr;
size_t snapshotJpegLen=0;
String snapshotMeta="{}",snapshotError="No snapshot yet";
void bmp32(uint8_t*p,uint32_t v){for(int i=0;i<4;i++)p[i]=(v>>(8*i))&255;}
void saveRequestedSnapshot(const String &result){
 if(!snapshotPending)return;
 uint8_t *jpg=nullptr,*bmp=nullptr;size_t n=0;String failure="";int reference=-1;
 if(!cameraOK||!frame||!rgbOK)failure="Camera/decode failed; retry";
 else{
  n=frame->len;jpg=(uint8_t*)ps_malloc(n);bmp=(uint8_t*)ps_malloc(54+MODEL_RGB_BYTES);
  if(!jpg||!bmp){free(jpg);free(bmp);jpg=bmp=nullptr;failure="Snapshot allocation failed";}
  else{
   memcpy(jpg,frame->buf,n);memset(bmp,0,54);bmp[0]='B';bmp[1]='M';bmp32(bmp+2,54+MODEL_RGB_BYTES);bmp32(bmp+10,54);bmp32(bmp+14,40);bmp32(bmp+18,MODEL_SIDE);bmp32(bmp+22,MODEL_SIDE);bmp[26]=1;bmp[28]=24;
   for(int y=0;y<MODEL_SIDE;y++)for(int x=0;x<MODEL_SIDE;x++){uint32_t c=packedPixel(rgb+3*(y*MODEL_SIDE+x));int k=54+3*((MODEL_SIDE-1-y)*MODEL_SIDE+x);bmp[k]=c&255;bmp[k+1]=(c>>8)&255;bmp[k+2]=(c>>16)&255;}
   xSemaphoreTake(decoderMutex,portMAX_DELAY);reference=checkSnapshotInput(frame,rgb,analysisW,analysisH,aiView);xSemaphoreGive(decoderMutex);
  }
 }
 xSemaphoreTake(stateMutex,portMAX_DELAY);
 free(snapshotJpeg);free(snapshotBmp);snapshotJpeg=jpg;snapshotBmp=bmp;snapshotJpegLen=jpg?n:0;snapshotId++;snapshotError=failure;
 snapshotMeta=result;if(snapshotMeta.endsWith("}"))snapshotMeta.remove(snapshotMeta.length()-1);
 snapshotMeta+=",\"snapshot_id\":"+String(snapshotId)+",\"input_reference_match\":"+String(reference)+",\"input_bgr\":"+String(inputBGR)+",\"view_mode\":"+String(viewMode)+"}";
 snapshotPending=false;xSemaphoreGive(stateMutex);
}
void requestSnapshot(){
 if(resolutionBusy){web.send(409,"text/plain","Wait for the resolution change to finish before capture");return;}
 sendJSON("{\"N\":100}");owner=0;
 xSemaphoreTake(stateMutex,portMAX_DELAY);snapshotPending=true;uint32_t target=snapshotId+1;xSemaphoreGive(stateMutex);
 web.send(202,"application/json","{\"id\":"+String(target)+"}");
}
void snapshotState(){
 xSemaphoreTake(stateMutex,portMAX_DELAY);String s="{\"id\":"+String(snapshotId)+",\"pending\":"+String((bool)snapshotPending)+",\"ready\":"+String(snapshotJpeg&&snapshotBmp)+",\"error\":\""+snapshotError+"\"}";xSemaphoreGive(stateMutex);
 web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",s);
}
void sendSnapshot(int kind){
 uint8_t *copy=nullptr;size_t n=0;String meta;uint32_t id=0;bool mismatch=false;
 xSemaphoreTake(stateMutex,portMAX_DELAY);id=snapshotId;
 mismatch=web.hasArg("id")&&web.arg("id")!=String(id);
 if(!mismatch&&snapshotJpeg){
  meta=snapshotMeta;
  if(kind<2){n=kind==0?snapshotJpegLen:(54+MODEL_RGB_BYTES);copy=(uint8_t*)ps_malloc(n);if(copy)memcpy(copy,kind==0?snapshotJpeg:snapshotBmp,n);}
 }
 xSemaphoreGive(stateMutex);
 if(mismatch){web.send(409,"text/plain","Snapshot changed; request a new capture");return;}
 if(meta==""){if(!web.hasArg("id"))snapshotPending=true;web.send(503,"text/plain","Waiting for capture; retry shortly or use the Capture button");return;}
 if(kind<2&&!copy){web.send(503,"text/plain","Snapshot copy allocation failed");return;}
 web.sendHeader("Cache-Control","no-store");web.sendHeader("X-Snapshot-Id",String(id));
 if(kind==2){if(web.arg("download")=="1")web.sendHeader("Content-Disposition",String("attachment; filename=\"esp32_")+String(id)+".json\"");web.send(200,"application/json",meta);return;}
 if(web.arg("download")=="1")web.sendHeader("Content-Disposition",String("attachment; filename=\"esp32_")+String(id)+(kind==0?".jpg\"":"_input.bmp\""));
 web.setContentLength(n);web.send(200,kind==0?"image/jpeg":"image/bmp","");web.client().write(copy,n);free(copy);
}
int colourRange(const RedRegion &r,float k){
 if(k<=0||r.w<8||r.x<=0||r.y<=0||r.x+r.w>=r.imageW||r.y+r.h>=r.imageH||colourCalResolution.load()!=esp_camera_sensor_get()->status.framesize)return -1;
 float d=k*r.imageW/r.w;return std::isfinite(d)&&d<=10000?(int)(d+0.5f):-1;
}
String redResults(){
 xSemaphoreTake(stateMutex,portMAX_DELAY);RedStatus r=redStatus;xSemaphoreGive(stateMutex);
 bool enabled=colorEnabled.load();bool fresh=!resolutionBusy&&r.valid&&(millis()-r.captured)<1200;
 String j="{\"red_regions\":"+String(r.region.components)+",\"green_regions\":"+String(r.green.components)+",\"green_seen\":"+String(enabled&&fresh&&r.greenSeen)+",\"stop_distance_cm\":"+String(colourRange(r.region,stopColourK.load()))+",\"light_distance_cm\":"+String(colourRange(r.region,lightColourK.load()))+",\"stop_k\":"+String(stopColourK.load(),4)+",\"light_k\":"+String(lightColourK.load(),4)+",\"enabled\":"+String(enabled)+",\"valid\":"+String(fresh&&enabled)+",\"seen\":"+String(enabled&&fresh&&r.seen);
 j+=",\"pixels\":"+String(r.region.pixels)+",\"min_pixels\":"+String(redMinPixels.load())+",\"age_ms\":"+String(millis()-r.captured)+",\"processing_ms\":"+String(r.ms);
 j+=",\"x\":"+String(r.region.x)+",\"y\":"+String(r.region.y)+",\"w\":"+String(r.region.w)+",\"h\":"+String(r.region.h)+",\"image_w\":"+String(r.region.imageW)+",\"image_h\":"+String(r.region.imageH)+"}";return j;
}
String liveObjectsJSON(){
 xSemaphoreTake(stateMutex,portMAX_DELAY);RedStatus r=redStatus;StopMeasurement stop=stopMeasurement;xSemaphoreGive(stateMutex);
 bool fresh=colorEnabled&&!resolutionBusy&&r.valid&&millis()-r.captured<1200;
 bool sf=detectEnabled&&!resolutionBusy&&stop.valid&&millis()-stop.completed<5000&&stop.resolution==esp_camera_sensor_get()->status.framesize;
 bool associated=sf&&fresh&&r.seen&&r.region.components==1&&stop.region.imageW==r.region.imageW&&
  abs((r.region.x+r.region.w/2)-(stop.region.x+stop.region.w/2))<max(12,stop.region.w/2)&&
  abs((r.region.y+r.region.h/2)-(stop.region.y+stop.region.h/2))<max(12,stop.region.h/2);
 bool stopSeen=(colorEnabled?associated:sf)&&stop.seen;
 String out="{";
 for(int i=0;i<3;i++){
  const RedRegion &v=i==1?r.green:(i==2?stop.region:r.region);
  bool valid=i==2?(colorEnabled?(detectEnabled&&fresh&&(!r.seen||associated)):sf):fresh;
  bool seen=valid&&(i==0?r.seen:(i==1?r.greenSeen:stopSeen));
  int distance=seen&&(i!=2||stop.matched)?colourRange(v,i==0?lightColourK.load():(i==1?greenColourK.load():stopColourK.load())):-1;
  char b[200];snprintf(b,sizeof(b),"%s\"%s\":{\"valid\":%d,\"seen\":%d,\"pixels\":%d,\"width\":%d,\"distance_cm\":%d}",i?",":"",i==0?"red":(i==1?"green":"stop"),valid,seen,seen&&(i!=2||stop.matched)?v.pixels:0,seen&&(i!=2||stop.matched)?v.w:0,distance);out+=b;
  const RedRegion &box=i==2?(associated?r.region:stop.displayRegion):v;
  bool boxValid=i==2?(sf&&stop.seen&&(stop.matched&&colorEnabled?associated:true)):seen;
  out.remove(out.length()-1);
  out+=",\"box_valid\":"+String(boxValid)+",\"x\":"+String(box.x)+",\"y\":"+String(box.y)+",\"w\":"+String(box.w)+",\"h\":"+String(box.h)+",\"image_w\":"+String(box.imageW)+",\"image_h\":"+String(box.imageH)+",\"captured\":"+String(i==2?(associated?r.captured:stop.captured):r.captured)+",\"fomo_cell\":"+String(i==2&&!stop.matched)+",\"inference_ms\":"+String(i==2?stop.inferenceMs:0)+",\"decode_ms\":"+String(i==2?stop.decodeMs:r.ms)+"}";
 }
 return out+"}";
}
// The only normal caller of esp_camera_fb_get. No JPEG decode or AI here.
void captureTask(void*){
 uint8_t *staging=nullptr;size_t capacity=0;
 for(;;){
  if(!resolutionBusy){
   xSemaphoreTake(cameraMutex,portMAX_DELAY);
   if(resolutionBusy){xSemaphoreGive(cameraMutex);vTaskDelay(pdMS_TO_TICKS(20));continue;}
   camera_fb_t *rf=esp_camera_fb_get();uint32_t stamp=millis();int w=0,h=0;
   bool ok=rf&&rf->format==PIXFORMAT_JPEG&&jpegSize(rf->buf,rf->len,w,h)&&
    w==(int)rf->width&&h==(int)rf->height;
   size_t n=ok?rf->len:0;
   ok=ok&&reserveJpeg(staging,capacity,n);
   if(ok)memcpy(staging,rf->buf,n);
   if(rf)esp_camera_fb_return(rf); // Return driver storage before any processing.
   xSemaphoreTake(stateMutex,portMAX_DELAY);
   if(ok){
    uint8_t *old=previewJpeg;size_t oldCapacity=previewCapacity;
    previewJpeg=staging;previewCapacity=capacity;previewLen=n;
    previewCaptured=stamp;previewWidth=w;previewHeight=h;previewEpoch=geometryEpoch;
    staging=old;capacity=oldCapacity;cameraFrameCount++;
   }else cameraFrameErrors++;
   xSemaphoreGive(stateMutex);xSemaphoreGive(cameraMutex);
  }
  vTaskDelay(pdMS_TO_TICKS(100));
 }
}
void colourTask(void*){
 camera_fb_t local={};size_t capacity=0;uint32_t lastStamp=0;uint16_t seq=0;
 for(;;){
  uint32_t start=millis(),captured=0;uint16_t epoch=0;
  if(!colorEnabled||!copyPreview(local,capacity,captured,epoch,lastStamp)){
   vTaskDelay(pdMS_TO_TICKS(40));continue;
  }
  lastStamp=captured;RedStatus next;next.seq=++seq;next.captured=captured;
  xSemaphoreTake(decoderMutex,portMAX_DELAY);
  next.valid=decodeRed(&local,redCells,redQueue,next.region,greenCells,&next.green,redMinPixels.load());
  xSemaphoreGive(decoderMutex);
  next.seen=next.valid&&next.region.pixels>=redMinPixels.load();
  next.greenSeen=next.valid&&next.green.pixels>=redMinPixels.load();next.ms=millis()-start;
  xSemaphoreTake(stateMutex,portMAX_DELAY);
  // A settings change must not republish a result from the previous geometry.
  if(!resolutionBusy&&colorEnabled&&epoch==geometryEpoch)redStatus=next;
  xSemaphoreGive(stateMutex);
  vTaskDelay(pdMS_TO_TICKS(100));
 }
}
void sendLivePreview(){
 static uint8_t *copy=nullptr;static size_t capacity=0;
 size_t n=0;uint32_t stamp=0,age=0;bool same=false,ready=false;
 xSemaphoreTake(stateMutex,portMAX_DELAY);
 if(!resolutionBusy&&previewJpeg&&previewEpoch==geometryEpoch&&millis()-previewCaptured<1500){
  stamp=previewCaptured;age=millis()-stamp;
  same=previewWeb.hasArg("after")&&previewWeb.arg("after")==String(stamp);
  if(!same){n=previewLen;ready=reserveJpeg(copy,capacity,n);if(ready)memcpy(copy,previewJpeg,n);}
 }
 xSemaphoreGive(stateMutex);
 previewWeb.sendHeader("Cache-Control","no-store");
 // Only the read-only JPEG endpoint is cross-origin; control stays on port 80.
 previewWeb.sendHeader("Access-Control-Allow-Origin","*");
 previewWeb.sendHeader("Access-Control-Expose-Headers","X-Frame-Stamp,X-Frame-Age");
 previewWeb.sendHeader("X-Frame-Stamp",String(stamp));previewWeb.sendHeader("X-Frame-Age",String(age));
 if(same){previewWeb.send(204,"text/plain","");return;}
 if(!ready){previewWeb.send(503,"text/plain","Waiting for a fresh camera frame");return;}
 previewWeb.setContentLength(n);previewWeb.send(200,"image/jpeg","");
 auto client=previewWeb.client();size_t sent=0;uint32_t started=millis();
 while(sent<n&&client.connected()&&millis()-started<5000){
  size_t wrote=client.write(copy+sent,min((size_t)1460,n-sent));
  if(!wrote)break;
  sent+=wrote;vTaskDelay(pdMS_TO_TICKS(1));
 }
 if(sent<n)client.stop(); // Do not leave a partial JPEG response hanging.
}
void previewTask(void*){for(;;){previewWeb.handleClient();vTaskDelay(pdMS_TO_TICKS(5));}}
void resolutionProgress(const char *state,const char *message){
 xSemaphoreTake(stateMutex,portMAX_DELAY);resolutionState=state;resolutionMessage=message;xSemaphoreGive(stateMutex);
}
bool checkCameraSize(int id){
 int expectedW,expectedH;if(!resolutionSize(id,expectedW,expectedH))return false;
 for(int i=0;i<4;i++){
  camera_fb_t *test=esp_camera_fb_get();int w=0,h=0;
  bool ok=test&&test->format==PIXFORMAT_JPEG&&jpegSize(test->buf,test->len,w,h)&&w==expectedW&&h==expectedH;
  if(test)esp_camera_fb_return(test);
  // Discard the first two frames after the register change.
  if(i>=2&&ok){xSemaphoreTake(stateMutex,portMAX_DELAY);verifiedWidth=w;verifiedHeight=h;xSemaphoreGive(stateMutex);return true;}
  vTaskDelay(pdMS_TO_TICKS(20));
 }
 return false;
}
void applyPendingResolution(){
 int target=pendingResolution.exchange(-1);if(target<0)return;
 resolutionProgress("applying","Pausing red analysis and switching camera resolution");
 if(xSemaphoreTake(cameraMutex,pdMS_TO_TICKS(3000))!=pdTRUE){resolutionProgress("failed","Camera busy; previous resolution retained. Retry or check serial output.");resolutionBusy=false;return;}
 sensor_t *sensor=esp_camera_sensor_get();int old=(int)sensor->status.framesize;
 releaseFrame();objectCount=0;stopCount=0;frameValid=false;cameraOK=false;rgbOK=false;rawDiagnostic={};
 xQueueReset(visionQueue);
 xSemaphoreTake(stateMutex,portMAX_DELAY);geometryEpoch++;redStatus={};stopMeasurement={};free(previewJpeg);previewJpeg=nullptr;previewLen=previewCapacity=0;cachedResults="{\"seq\":0,\"valid\":0,\"objects\":[]}";xSemaphoreGive(stateMutex);
 bool success=sensor->set_framesize(sensor,(framesize_t)target)==0;
 if(success){resolutionProgress("verifying","Waiting for a JPEG with the requested dimensions");success=checkCameraSize(target);}
 if(success){loadCalibration(target);resolutionProgress("applied","Resolution verified from a new JPEG; analysis resumes automatically");}
 else {
  bool restored=sensor->set_framesize(sensor,(framesize_t)old)==0;
  restored=restored&&checkCameraSize(old);
  resolutionProgress("failed",restored?"Requested resolution failed; previous resolution restored":"Camera did not produce a valid frame. Check serial output and power-cycle if needed");
 }
 xSemaphoreTake(stateMutex,portMAX_DELAY);cameraStatus=makeCameraStatus();xSemaphoreGive(stateMutex);
 xSemaphoreGive(cameraMutex);resolutionBusy=false;
 Serial.printf("Resolution request %d: %s; JPEG %dx%d\n",target,success?"verified":"failed",verifiedWidth,verifiedHeight);
}
String resolutionStatus(){
 xSemaphoreTake(stateMutex,portMAX_DELAY);
 String j="{\"id\":"+String(resolutionId)+",\"requested\":"+String(requestedResolution)+",\"state\":\""+resolutionState+"\",\"message\":\""+resolutionMessage+"\",\"width\":"+String(verifiedWidth)+",\"height\":"+String(verifiedHeight)+"}";
 xSemaphoreGive(stateMutex);return j;
}
void visionTask(void*){
 uint32_t previousStart=0;
 for(;;){
  uint32_t cycleStart=millis();cycleMs=previousStart?cycleStart-previousStart:0;previousStart=cycleStart;
  if(xSemaphoreTake(frameMutex,portMAX_DELAY)==pdTRUE){
   applyPendingResolution();
   int nextThreshold=pendingThreshold.exchange(-1);if(nextThreshold>=50&&nextThreshold<=99)aiThreshold=nextThreshold/100.0f;
   bool captureThisFrame=snapshotPending.load();
   bool a3=trafficMode.load()==6;
   xSemaphoreTake(stateMutex,portMAX_DELAY);RedStatus gate=redStatus;xSemaphoreGive(stateMutex);
   bool redCandidate=colorEnabled&&gate.valid&&gate.seen&&millis()-gate.captured<1200;
   bool needAI=detectEnabled&&(a3?((trafficState.load()==4||(driveSettings.rs&&trafficState.load()==5))&&millis()-trafficReceived.load()<1500):(!colorEnabled||redCandidate));
   if(!needAI&&!captureThisFrame){
    xSemaphoreGive(frameMutex);vTaskDelay(pdMS_TO_TICKS(40));continue;
   }
   acquire();String result=results();
   result.remove(result.length()-1);result+=",\"detect_enabled\":"+String((bool)detectEnabled)+",\"color_enabled\":"+String((bool)colorEnabled)+"}";
   xSemaphoreTake(stateMutex,portMAX_DELAY);cachedResults=result;cameraStatus=makeCameraStatus();xSemaphoreGive(stateMutex);
   if(captureThisFrame)saveRequestedSnapshot(result);
   releaseFrame();
   xSemaphoreGive(frameMutex);printSerialResult(result);
  }
  vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
 }
}
bool strictInt(const String &s,int lo,int hi,int &v){
 if(!s.length()||s.length()>6)return false;
 for(size_t i=0;i<s.length();i++)if(s[i]<'0'||s[i]>'9')return false;
 v=s.toInt();return v>=lo&&v<=hi;
}
void webCommand(){
 String action=web.arg("action");String j;
 int value;
 if(action=="preview"){previewEnabled=web.arg("value")=="1";web.send(200,"text/plain","Preview updated");return;}
 if(action=="red_min"){if(!strictInt(web.arg("value"),2,2000,value)){web.send(400,"text/plain","Red minimum must be 2-2000 pixels");return;}redMinPixels=value;web.send(200,"text/plain","Red minimum updated");return;}
 if(action=="gyro_navigation"){
  if(trafficMode!=0||millis()-trafficReceived.load()>1500){web.send(409,"text/plain","Stop car and wait for fresh OFF telemetry");return;}
  if(!strictInt(web.arg("value"),0,2,value)){web.send(400,"text/plain","Use 0, 1 or 2");return;}
  sendJSON(String("{\"N\":213,\"D1\":")+String(value)+"}");web.send(200,"text/plain","Requested; wait for UNO confirmation");return;
 }
 if(action=="team_pd"){if(trafficMode!=0){web.send(409,"text/plain","Stop car before changing controller");return;}if(!strictInt(web.arg("value"),0,1,value)){web.send(400,"text/plain","Use 0 or 1");return;}j=String("{\"N\":208,\"D1\":")+String(value)+"}";}
 else if(action=="estop"){j="{\"N\":207,\"D1\":1}";owner=0;}
 else if(action=="clear_estop"){j="{\"N\":207,\"D1\":0}";owner=0;}
 else if(action=="stop"){j="{\"N\":100}";owner=0;}
 else if(action=="ping"){if(owner==1)sendJSON("{\"N\":103}");web.send(200,"text/plain","ok");return;}
 else if(action=="move"){
  int speed;if(!strictInt(web.arg("dir"),1,9,value)||!strictInt(web.arg("speed"),0,180,speed)){web.send(400,"text/plain","Bad direction/speed");return;}
  j="{\"N\":102,\"D1\":"+String(value)+",\"D2\":"+String(speed)+"}";owner=value==9?0:1;
 }else if(action=="mode"){
  if(!strictInt(web.arg("value"),0,4,value)){web.send(400,"text/plain","Bad mode");return;}
  if(value==4&&(!A2_ENABLE_AI||!detectEnabled)){web.send(409,"text/plain","Import model and enable AI first");return;}
  j="{\"N\":101,\"D1\":"+String(value)+"}";owner=value==0?0:1;
 }else if(action=="speed"){
  if(!strictInt(web.arg("value"),0,180,value)){web.send(400,"text/plain","Bad speed");return;}
  j="{\"N\":104,\"D1\":"+String(value)+"}";
 }else if(action=="servo"){
  if(!strictInt(web.arg("value"),0,180,value)){web.send(400,"text/plain","Bad servo angle");return;}
  owner=0;j="{\"N\":5,\"D1\":1,\"D2\":"+String(value)+"}";
 }else if(action=="led"){
  int r,g,b;if(!strictInt(web.arg("r"),0,255,r)||!strictInt(web.arg("g"),0,255,g)||!strictInt(web.arg("b"),0,255,b)){web.send(400,"text/plain","Bad RGB");return;}
  j="{\"N\":8,\"D1\":0,\"D2\":"+String(r)+",\"D3\":"+String(g)+",\"D4\":"+String(b)+"}";
 }else if(action=="guard"){
  if(!strictInt(web.arg("value"),0,1,value)){web.send(400,"text/plain","Bad guard flag");return;}
  if(value&&(!A2_ENABLE_AI||!detectEnabled)){web.send(409,"text/plain","No AI model");return;}
  j="{\"N\":201,\"D1\":"+String(value)+"}";owner=0;
 }else if(action=="reset_stop"){j="{\"N\":202}";owner=0;}
 else if(action=="detect"||action=="color"){
  if(!strictInt(web.arg("value"),0,1,value)){web.send(400,"text/plain","Bad flag");return;}
  if(action=="detect"&&value&&!A2_ENABLE_AI){web.send(409,"text/plain","Firmware has no model");return;}
  // Changing either analysis setting stops the car first.
  sendJSON("{\"N\":100}");owner=0;
  sampleAvailable=false;if(action=="detect")detectEnabled=value;else colorEnabled=value;
  xSemaphoreTake(stateMutex,portMAX_DELAY);geometryEpoch++;redStatus={};stopMeasurement={};xSemaphoreGive(stateMutex);
  web.send(200,"text/plain","Changed; vehicle stopped");return;
 }else{web.send(400,"text/plain","Unknown action");return;}
 sendJSON(j);web.send(200,"application/json",j);
}
void calibrateDistance(){
 if(web.arg("clear")!="1"){web.send(409,"text/plain","FOMO provides centroid regions, not object dimensions. Size calibration is unavailable.");return;}
 int id,width,cm,referenceW,resolution,epoch;int slot;
 if(!strictInt(web.arg("id"),1,1,id)||(slot=calibrationSlot(id))<0){web.send(400,"text/plain","Invalid object type");return;}
 bool clear=web.arg("clear")=="1";
 if(!clear && (!strictInt(web.arg("width"),6,ANALYSIS_MAX_W-2,width)||!strictInt(web.arg("cm"),5,1000,cm)||!strictInt(web.arg("image_w"),96,400,referenceW)||!strictInt(web.arg("framesize"),0,10,resolution)||!strictInt(web.arg("epoch"),0,65535,epoch))){web.send(400,"text/plain","Need sampled width and known distance 5-1000 cm");return;}
 sendJSON("{\"N\":100}");owner=0;
 if(xSemaphoreTake(frameMutex,pdMS_TO_TICKS(20))!=pdTRUE){web.send(503,"text/plain","Vision busy; press Save again");return;}
 if(!clear && (resolution!=(int)esp_camera_sensor_get()->status.framesize || epoch!=geometryEpoch || referenceW!=analysisW)){xSemaphoreGive(frameMutex);web.send(409,"text/plain","Camera settings changed. Sample again.");return;}
 float k=clear?0.0f:(float)width*cm/referenceW;
 bool saved=calibrationReady;
 if(saved){if(calibration.getInt("res",-1)!=calibrationResolution){for(int i=0;i<5;i++)saved=(calibration.putFloat(calibrationKeys[i],0)==sizeof(float))&&saved;}saved=(calibration.putInt("res",calibrationResolution)==sizeof(int))&&saved;saved=(calibration.putFloat(calibrationKeys[slot],k)==sizeof(float))&&saved;}
 if(saved)distanceK[slot]=k;
 xSemaphoreGive(frameMutex);
 if(!saved){web.send(500,"text/plain","Calibration could not be stored");return;}
 web.send(200,"text/plain",String(clear?"Calibration cleared. ":"Calibration saved. ")+"K="+String(k,2)+" cm (normalized width); vehicle stopped");
}
void bridge(){
 while(Serial2.available()){
  char c=Serial2.read();if(c=='{')unoRx="";
  if(unoRx.length()<288)unoRx+=c;else unoRx="";
  if(c=='}'){
   if(unoRx.startsWith("{\"N\":210,")){unoStatus=unoRx;lastUno=millis();}
   else if(unoRx.startsWith("{\"N\":221,")){
    unsigned m,t,id;if(sscanf(unoRx.c_str(),"{\"N\":221,\"M\":%u,\"TS\":%u,\"ID\":%u",&m,&t,&id)==3){if(m==6&&t==4&&(trafficMode!=6||trafficState!=4||trafficRequest!=(int)id))verifySince=millis();trafficMode=m;trafficState=t;trafficRequest=id;trafficReceived=millis();trafficTelemetry=unoRx;const char *uv=strstr(unoRx.c_str(),"\"UV\":");unoFirmware=uv?atoi(uv+5):0;}
   }else if(unoRx.startsWith("{\"N\":222,")){
    int id,ok,cs,rs,ds;if(sscanf(unoRx.c_str(),"{\"N\":222,\"ID\":%d,\"OK\":%d,\"CS\":%d,\"RS\":%d,\"DS\":%d",&id,&ok,&cs,&rs,&ds)==5&&id==settingsID&&ok==1&&cs==driveSettings.cs&&rs==driveSettings.rs&&ds==driveSettings.ds)settingsApplied=true;
   }else lastReply=unoRx;

   unoRx="";
  }
 }
}
void sendHomePage(){
 bool plain=web.hasArg("plain")&&web.arg("plain")=="1";
 const uint8_t *body=plain?(const uint8_t*)TRAFFIC_PAGE:TRAFFIC_PAGE_GZ;
 size_t length=plain?sizeof(TRAFFIC_PAGE)-1:sizeof(TRAFFIC_PAGE_GZ);
 Serial.printf("UI request: %s, %u bytes, free heap %u\n",plain?"plain":"gzip",(unsigned)length,(unsigned)ESP.getFreeHeap());
 web.sendHeader("Cache-Control","no-store, max-age=0");
 if(!plain)web.sendHeader("Content-Encoding","gzip");
 web.setContentLength(length);web.send(200,"text/html; charset=utf-8","");
 auto client=web.client();
 size_t sent=writePageBody(client,body,length,[]{return millis();},[]{vTaskDelay(pdMS_TO_TICKS(1));});
 Serial.printf("UI body queued: %u/%u bytes %s\n",(unsigned)sent,(unsigned)length,sent==length?"OK":"INCOMPLETE");
}
void setup(){
 Serial.begin(115200);Serial2.begin(9600,SERIAL_8N1,UART_RX,UART_TX);sendJSON("{\"N\":100}");
 frameMutex=xSemaphoreCreateMutex();stateMutex=xSemaphoreCreateMutex();cameraMutex=xSemaphoreCreateMutex();decoderMutex=xSemaphoreCreateMutex();visionQueue=xQueueCreate(1,sizeof(VisionPayload));
 if(!frameMutex||!stateMutex||!visionQueue||!cameraMutex||!decoderMutex){Serial.println("ERROR synchronization allocation");while(true)delay(1000);}
#if A2_ENABLE_AI
 if(strcmp(ei_classifier_inferencing_categories[0],"3")){
  Serial.println("ERROR: expected model target class 3");while(true)delay(1000);
 }
#endif
 if(!psramFound()){Serial.println("ERROR: enable PSRAM");while(true)delay(1000);}
 prepareEiArena();
 redCells=(uint8_t*)ps_malloc(10000);redQueue=(uint16_t*)ps_malloc(20000);
 rgb=(uint8_t*)ps_malloc(MODEL_RGB_BYTES);
 greenCells=(uint8_t*)ps_malloc(10000);
 aiRedCells=(uint8_t*)ps_malloc(10000);aiRedQueue=(uint16_t*)ps_malloc(20000);
 if(!rgb||!redCells||!redQueue||!greenCells||!aiRedCells||!aiRedQueue||!cameraInit()){Serial.println("ERROR: camera/memory");while(true)delay(1000);}
 calibrationReady=calibration.begin("a2-cam-v2",false);
 if(calibrationReady){
  if(calibration.getBytesLength("a3drive24")==sizeof(driveSettings))calibration.getBytes("a3drive24",&driveSettings,sizeof(driveSettings));
  else if(calibration.getBytesLength("a3drive23")==10*sizeof(int))calibration.getBytes("a3drive23",&driveSettings,10*sizeof(int));
  else if(calibration.getBytesLength("a3drive21")==9*sizeof(int))calibration.getBytes("a3drive21",&driveSettings,9*sizeof(int));
  else if(calibration.getBytesLength("a3drive2")==7*sizeof(int))calibration.getBytes("a3drive2",&driveSettings,7*sizeof(int));
 }
 if(driveSettings.cs<50||driveSettings.cs>150)driveSettings.cs=100;
 if(driveSettings.ds<0||driveSettings.ds>driveSettings.cs)driveSettings.ds=min(60,driveSettings.cs);
 if(driveSettings.rpx<0||driveSettings.rpx>160000)driveSettings.rpx=100;
 if(driveSettings.rs<0||driveSettings.rs>1)driveSettings.rs=0;
 if(calibrationReady&&calibration.getBytesLength("a3cal5")==sizeof(savedCalibration)){
  CalibrationStore stored;if(calibration.getBytes("a3cal5",&stored,sizeof(stored))==sizeof(stored)&&stored.magic==0xA3C50001UL)savedCalibration=stored;
 }else if(calibrationReady&&calibration.getBytesLength("a3cal3")==sizeof(LegacyCalibrationStore)){
  LegacyCalibrationStore old;if(calibration.getBytes("a3cal3",&old,sizeof(old))==sizeof(old)&&old.magic==0xA3C30001UL){savedCalibration.stop=old.stop;savedCalibration.light=old.light;}
 }
 // One-time migration requested for V15: known STOP reference and wider search.
 if(!calibrationReady||calibration.getInt("a3v15",0)!=1){
  savedCalibration.stop.width=96;savedCalibration.stop.imageWidth=400;savedCalibration.stop.cm=10;savedCalibration.stop.resolution=5;
  driveSettings.ra=150;driveSettings.sa=300;
  if(calibrationReady){
   bool ok=calibration.putBytes("a3cal5",&savedCalibration,sizeof(savedCalibration))==sizeof(savedCalibration);
   ok=(calibration.putBytes("a3drive24",&driveSettings,sizeof(driveSettings))==sizeof(driveSettings))&&ok;
   if(ok)calibration.putInt("a3v15",1);
  }
 }
 // V18 replaces the saved 150/300 scan once, including custom legacy angles.
 if(!calibrationReady||calibration.getInt("a3v18",0)!=1){
  driveSettings.ra=90;driveSettings.sa=180;
  if(calibrationReady&&calibration.putBytes("a3drive24",&driveSettings,sizeof(driveSettings))==sizeof(driveSettings))calibration.putInt("a3v18",1);
 }
 // A3 update: only migrate the original 90/180 default to the new 120/240.
 // Existing custom angles remain untouched. This runs once per ESP32 device.
 if(!calibrationReady||calibration.getInt("a3v27scan",0)!=1){
  if(driveSettings.ra==90&&driveSettings.sa==180){driveSettings.ra=120;driveSettings.sa=240;}
  if(calibrationReady&&calibration.putBytes("a3drive24",&driveSettings,sizeof(driveSettings))==sizeof(driveSettings))calibration.putInt("a3v27scan",1);
 }
 if(calibrationReady&&calibration.getInt("a3v35scan",0)!=1){
  driveSettings.ra=120;driveSettings.sa=360;
  if(calibration.putBytes("a3drive24",&driveSettings,sizeof(driveSettings))==sizeof(driveSettings))calibration.putInt("a3v35scan",1);
 }
 syncColourCalibration();
 loadCalibration((int)esp_camera_sensor_get()->status.framesize);
 cameraStatus=makeCameraStatus();
 detectEnabled=A2_ENABLE_AI;
 WiFi.mode(WIFI_AP);WiFi.softAP(AP_SSID,AP_PASSWORD,AP_CHANNEL);
 web.on("/",HTTP_GET,sendHomePage);
 web.on("/index.html",HTTP_GET,sendHomePage);
 web.on("/resolution-status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",resolutionStatus());});
 web.on("/red",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",redResults());});
 web.on("/results",HTTP_GET,[]{
  xSemaphoreTake(stateMutex,portMAX_DELAY);String s=cachedResults;xSemaphoreGive(stateMutex);
  if(s.endsWith("}"))s.remove(s.length()-1);
  s+=",\"uno\":"+unoStatus+",\"uno_age_ms\":"+String(millis()-lastUno)+",\"detect_enabled\":"+String((bool)detectEnabled)+",\"color_enabled\":"+String((bool)colorEnabled)+"}";
  web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",s);
 });
 web.on("/frame",HTTP_GET,[]{web.sendHeader("Location","http://192.168.4.1:81/preview");web.send(302,"text/plain","");});
 web.on("/snapshot",HTTP_POST,requestSnapshot);
 web.on("/snapshot",HTTP_GET,snapshotState);
 web.on("/capture",HTTP_GET,[]{sendSnapshot(0);});
 web.on("/model-input",HTTP_GET,[]{sendSnapshot(1);});
 web.on("/snapshot-meta",HTTP_GET,[]{sendSnapshot(2);});
 web.on("/status",HTTP_GET,[]{
  xSemaphoreTake(stateMutex,portMAX_DELAY);String s=cameraStatus;uint32_t captured=previewCaptured;bool available=previewJpeg!=nullptr;uint32_t count=cameraFrameCount,errors=cameraFrameErrors;xSemaphoreGive(stateMutex);
  if(s.endsWith("}"))s.remove(s.length()-1);
  s+=",\"uptime_ms\":"+String(millis())+",\"preview_age_ms\":"+(available?String(millis()-captured):String(-1))+",\"camera_frames\":"+String(count)+",\"camera_errors\":"+String(errors)+"}";
  web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",s);
 });
 web.on("/camera-defaults",HTTP_POST,restoreCameraDefaults);
 web.on("/control",HTTP_GET,cameraControl);web.on("/control",HTTP_POST,cameraControl);
 web.on("/cmd",HTTP_POST,webCommand);
 web.on("/traffic-settings",HTTP_GET,[]{web.send(200,"application/json",driveSettingsJSON());});
 web.on("/traffic-settings",HTTP_POST,[]{
  if(trafficMode!=0||millis()-trafficReceived.load()>1500){sendJSON("{\"N\":100}");owner=0;web.send(409,"text/plain","Stop requested. Wait for UNO OFF, then save again.");return;}
  DriveSettings next;
  if(!strictInt(web.arg("SC"),5,150,next.sc)||!strictInt(web.arg("BL"),0,1022,next.bl)||!strictInt(web.arg("BH"),1,1023,next.bh)||next.bl>=next.bh||!strictInt(web.arg("RA"),30,120,next.ra)||!strictInt(web.arg("SA"),60,360,next.sa)||next.sa<next.ra||!strictInt(web.arg("CT"),300,5000,next.ct)||!strictInt(web.arg("GS"),0,1,next.gs)||!strictInt(web.arg("CS"),50,150,next.cs)||!strictInt(web.arg("RS"),0,1,next.rs)||!strictInt(web.arg("DS"),0,next.cs,next.ds)||!strictInt(web.arg("RP"),0,160000,next.rpx)){web.send(400,"text/plain","Invalid settings; black low must be less than black high.");return;}
  if(!calibrationReady||calibration.putBytes("a3drive24",&next,sizeof(next))!=sizeof(next)){web.send(500,"text/plain","Save failed");return;}
  driveSettings=next;settingsID=settingsID%30000+1;settingsApplied=false;sendDriveSettings();web.send(200,"text/plain","Saved. Waiting for UNO acknowledgement; check Applied status.");
 });
 web.on("/traffic",HTTP_GET,sendHomePage);
 web.on("/traffic-status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",String("{\"esp_ms\":")+String(millis())+",\"sensors\":"+unoStatus+",\"sensors_age_ms\":"+String(millis()-lastUno)+",\"vision_health\":"+redResults()+",\"uno\":"+trafficTelemetry+",\"age_ms\":"+String(millis()-trafficReceived.load())+",\"calibration\":"+calibrationJSON()+",\"settings\":"+driveSettingsJSON()+",\"framesize\":"+String(esp_camera_sensor_get()->status.framesize)+",\"objects\":"+liveObjectsJSON()+",\"detect_enabled\":"+String((bool)detectEnabled)+",\"color_enabled\":"+String((bool)colorEnabled)+"}");});
 web.on("/traffic-start",HTTP_POST,[]{
  if(esp_camera_sensor_get()->status.colorbar){web.send(409,"text/plain","Turn off sensor test colour bars before driving");return;}
  if(trafficMode!=0){web.send(409,"text/plain","Already active. Use STOP CAR before starting a new run.");return;}
  if(driveSettings.rs&&(!detectEnabled||!colorEnabled)){web.send(409,"text/plain","Enable STOP AI and red/green detection for Stop on red mode");return;}
  if(!settingsApplied||millis()-trafficReceived.load()>1500){web.send(409,"text/plain","Wait for fresh UNO telemetry and settings acknowledgement");return;}
  if(unoFirmware!=37){web.send(409,"text/plain","Upload the matching Test 7 UNO (UV=37) before starting");return;}
  if(trafficTelemetry.indexOf("\"ES\":1")>=0){web.send(409,"text/plain","Emergency stop is latched. Clear it, then Start separately.");return;}
  sendJSON("{\"N\":101,\"D1\":6}");owner=1;web.send(200,"text/plain","Start requested; check UNO state acknowledgement");
 });
 web.on("/traffic-sample",HTTP_POST,[]{
  sampleAvailable=false;
  if(trafficMode!=0||millis()-trafficReceived.load()>1500){sendJSON("{\"N\":100}");owner=0;web.send(409,"text/plain","Stop requested. Wait for UNO OFF, then scan again.");return;}
  String profile=web.arg("profile");if(profile!="stop"&&profile!="light"&&profile!="green"){web.send(400,"text/plain","Select target type");return;}
  xSemaphoreTake(stateMutex,portMAX_DELAY);RedStatus r=redStatus;StopMeasurement stop=stopMeasurement;xSemaphoreGive(stateMutex);
  RedRegion sample=profile=="green"?r.green:r.region;
  bool seen=profile=="green"?r.greenSeen:r.seen;
  if(profile=="stop"){
   if(r.region.components!=1||!detectEnabled||!stop.valid||!stop.seen||!stop.matched||millis()-stop.completed>5000||stop.resolution!=esp_camera_sensor_get()->status.framesize){web.send(409,"text/plain","Wait for a confirmed STOP sign with a matching red region");return;}
   if(abs((r.region.x+r.region.w/2)-(stop.region.x+stop.region.w/2))>=max(12,stop.region.w/2)||abs((r.region.y+r.region.h/2)-(stop.region.y+stop.region.h/2))>=max(12,stop.region.h/2)){web.send(409,"text/plain","Target moved; keep STOP still and scan again");return;}
   sample=stop.region; // exact frame classified by FOMO
  }
  if(resolutionBusy||!colorEnabled||!r.valid||!seen||sample.components!=1||millis()-r.captured>1000||sample.w<8||sample.x<=0||sample.y<=0||sample.x+sample.w>=sample.imageW||sample.y+sample.h>=sample.imageH){web.send(409,"text/plain","Need one fresh and complete target of the selected colour; do not crop its edges");return;}
  pendingCalibration.width=sample.w;pendingCalibration.imageWidth=sample.imageW;pendingCalibration.resolution=esp_camera_sensor_get()->status.framesize;
  pendingProfile=profile;sampleToken=sampleToken%30000+1;sampleAvailable=true;
  char b[150];snprintf(b,sizeof(b),"{\"token\":%d,\"width\":%ld,\"image_w\":%ld,\"framesize\":%ld}",sampleToken,(long)pendingCalibration.width,(long)pendingCalibration.imageWidth,(long)pendingCalibration.resolution);web.send(200,"application/json",b);
 });
 web.on("/traffic-calibrate",HTTP_POST,[]{
  if(trafficMode!=0||millis()-trafficReceived.load()>1500){sendJSON("{\"N\":100}");owner=0;web.send(409,"text/plain","Stop requested. Wait for UNO OFF and retry");return;}
  int cm,token;String profile=web.arg("profile");
  if(!sampleAvailable||profile!=pendingProfile||!strictInt(web.arg("token"),1,30000,token)||token!=sampleToken||!strictInt(web.arg("cm"),5,300,cm)){web.send(400,"text/plain","Scan this target first, then enter its measured distance (5-300 cm)");return;}
  if(resolutionBusy||pendingCalibration.resolution!=esp_camera_sensor_get()->status.framesize){sampleAvailable=false;web.send(409,"text/plain","Resolution changed. Scan the target again");return;}
  CalibrationStore next=savedCalibration;CalibrationSample v=pendingCalibration;v.cm=cm;
  if(profile=="stop")next.stop=v;else if(profile=="green")next.green=v;else next.light=v;
  if(!calibrationReady||calibration.putBytes("a3cal5",&next,sizeof(next))!=sizeof(next)){web.send(500,"text/plain","Save failed; previous calibration remains active");return;}
  savedCalibration=next;sampleAvailable=false;syncColourCalibration();
  web.send(200,"text/plain",String("Saved scanned width ")+String(v.width)+" px at "+String(v.cm)+" cm. See Saved calibration below.");
 });
 web.on("/traffic-clear",HTTP_POST,[]{
  if(trafficMode!=0||millis()-trafficReceived.load()>1500){web.send(409,"text/plain","Stop the car first");return;}
  String p=web.arg("profile");CalibrationStore next=savedCalibration;
  if(p=="stop")next.stop={};else if(p=="light")next.light={};else if(p=="green")next.green={};else{web.send(400,"text/plain","Unknown target");return;}
  if(!calibrationReady||calibration.putBytes("a3cal5",&next,sizeof(next))!=sizeof(next)){web.send(500,"text/plain","Clear failed");return;}
  savedCalibration=next;sampleAvailable=false;syncColourCalibration();web.send(200,"text/plain","Selected calibration cleared");
 });
 web.on("/calibrate",HTTP_POST,calibrateDistance);
 web.begin();
 previewWeb.on("/preview",HTTP_GET,sendLivePreview);previewWeb.begin();
 if(xTaskCreatePinnedToCore(previewTask,"preview",6144,nullptr,1,nullptr,0)!=pdPASS){Serial.println("ERROR preview task");while(true)delay(1000);}
 if(xTaskCreatePinnedToCore(captureTask,"capture",4096,nullptr,1,nullptr,0)!=pdPASS){Serial.println("ERROR capture task");while(true)delay(1000);}
 if(xTaskCreatePinnedToCore(colourTask,"colour",8192,nullptr,1,nullptr,0)!=pdPASS){Serial.println("ERROR colour task");while(true)delay(1000);}
 if(xTaskCreatePinnedToCore(visionTask,"vision",16384,nullptr,1,nullptr,1)!=pdPASS){Serial.println("ERROR vision task");while(true)delay(1000);}
 Serial.println("ECE 4333 A3 Test 1 (ESP32) http://192.168.4.1 ; live preview ON");
}
void loop(){
 if(cameraRestartPending){
  // Do not accept drive commands between the stop and the requested restart.
  sendJSON("{\"N\":100}");
  if(millis()-cameraRestartRequestedAt>=750)ESP.restart();
  delay(20);return;
 }
 web.handleClient();if(cameraRestartPending)return;bridge();syncColourCalibration();
 if(millis()-trafficReceived.load()>1800)settingsApplied=false;
 // Re-send while OFF, so a UNO reset cannot silently lose the saved settings.
 if(trafficMode==0&&millis()-trafficReceived.load()<1500&&millis()-settingsSent>1200)sendDriveSettings();
 VisionPayload v;
 if(xQueueReceive(visionQueue,&v,0)==pdTRUE){
  char s[160];snprintf(s,sizeof(s),"{\"N\":200,\"S\":%u,\"A\":%d,\"V\":%d,\"P\":%d,\"R\":%d,\"X\":%d,\"Y\":%d,\"W\":%d,\"H\":%d,\"O\":%d}",
   v.seq,v.ai,(v.valid&&millis()-v.captured<=2000),v.confidence,v.distance,v.cx,v.cy,v.w,v.h,stopIndicatorReady(v.observed,v.completed,millis(),detectEnabled,resolutionBusy));
  sendJSON(s);
  if(trafficMode==6&&(trafficState==4||(driveSettings.rs&&trafficState==5))&&v.request==trafficRequest.load()){
   char ai[100];snprintf(ai,sizeof(ai),"{\"N\":205,\"S\":%u,\"ID\":%u,\"V\":%u,\"P\":%d}",v.seq,v.request,(unsigned)(v.valid&&detectEnabled&&!resolutionBusy&&millis()-v.completed<1000),v.confidence);sendJSON(ai);
  }
 }
 static uint32_t lastRedSend=0;
 if(millis()-lastRedSend>=200){
  lastRedSend=millis();xSemaphoreTake(stateMutex,portMAX_DELAY);RedStatus r=redStatus;xSemaphoreGive(stateMutex);
  bool ok=!resolutionBusy&&colorEnabled&&r.valid&&millis()-r.captured<1200;
  sendJSON(String("{\"N\":203,\"R\":")+String(ok&&r.seen)+"}");
  static uint16_t sentColour=0;
  if(r.seq!=sentColour){sentColour=r.seq;
   int sd=colourRange(r.region,stopColourK.load()),ld=colourRange(r.region,lightColourK.load());
   bool redAction=redActionTriggered(r.seen,r.region.pixels,driveSettings.rpx);
   bool singleRed=r.region.components==1;
   int d=singleRed?ld:-1; // Red-light distance is independent of STOP calibration.
   if(!singleRed)sd=-1;
   bool lightCalibrated=lightColourK.load()>0;
   // Test 1: recognize green anywhere in the image. Green need not appear
   // where red was seen. UNO ignores green in TRACK; in WAIT_RED/VERIFY it
   // needs three new, valid green-only frames before resuming.
   bool g=r.greenSeen;
   float beta=(A3_FX_NORMALIZED>0&&r.region.imageW)?atan((0.5f-(r.region.x+r.region.w*0.5f)/r.region.imageW)/A3_FX_NORMALIZED):0;
   char c[130];snprintf(c,sizeof(c),"{\"N\":204,\"S\":%u,\"V\":%u,\"R\":%u,\"G\":%u,\"D\":%d,\"SD\":%d,\"B\":%d,\"LC\":%u}",r.seq,(unsigned)ok,(unsigned)(ok&&redAction),(unsigned)(ok&&g),d,sd,(int)(beta*1000),(unsigned)lightCalibrated);sendJSON(c);
  }
 }
 delay(1);
}
