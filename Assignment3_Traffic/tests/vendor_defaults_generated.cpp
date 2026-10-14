#include <cassert>
#include <atomic>
#include "esp_syntax_stubs/Arduino.h"
#include "esp_syntax_stubs/esp_camera.h"
#include "../ESP32_Assignment2/Config.h"
#include "../ESP32_Assignment2/camera_pins.h"
sensor_t sensor={};camera_config_t received={};int initError=0,frameError=0,sizeCalls=0;bool missingSensor=false;
int esp_camera_init(camera_config_t*c){received=*c;return initError;}
sensor_t*esp_camera_sensor_get(){return missingSensor?nullptr:&sensor;}
int setSize(sensor_t*,int n){assert(n==FRAMESIZE_SVGA);sizeCalls++;return frameError;}
String sent;void sendJSON(String s){sent=s;}
int owner=1;bool sampleAvailable=true,cameraRestartPending=false;uint32_t cameraRestartRequestedAt=999;
std::atomic<bool>detectEnabled{true},colorEnabled{true};
struct Web{int code=0;void send(int n,const char*,const char*){code=n;}}web;

int orientationCalls=0;int setOrientation(sensor_t*,int v){assert(v==0);orientationCalls++;return 0;}
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
 web.send(202,"text/plain","Restoring uploaded camera defaults. ESP32 will restart; reconnect Wi-Fi and refresh this page. Calibration remains saved.");
}
int main(){
 // All ISP setter pointers remain null: any extra image-setting call fails this test.
 sensor.set_framesize=setSize;sensor.set_vflip=setOrientation;sensor.set_hmirror=setOrientation;assert(cameraInit()&&sizeCalls==1);
 assert(received.frame_size==FRAMESIZE_UXGA&&received.pixel_format==PIXFORMAT_JPEG&&received.jpeg_quality==10&&received.fb_count==2&&orientationCalls==2&&received.xclk_freq_hz==10000000);
 initError=-1;assert(!cameraInit()&&sizeCalls==1);initError=0;
 missingSensor=true;assert(!cameraInit());missingSensor=false;
 frameError=-1;assert(!cameraInit());frameError=0;
 restoreCameraDefaults();assert(web.code==202&&cameraRestartPending&&cameraRestartRequestedAt==millis());
 assert(owner==0&&!sampleAvailable&&!detectEnabled&&!colorEnabled&&sent=="{\"N\":100}");
 puts("PASS V12 vendor startup: quality10/two buffers/SVGA/final orientation; no white-balance or exposure overrides, init failures handled, reset stops car and queues reboot.");
}
