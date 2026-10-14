#include <cassert>
#include <atomic>
#include "esp_syntax_stubs/Arduino.h"
#include "esp_syntax_stubs/esp_camera.h"
#include "../ESP32_Assignment2/Config.h"
#include "../ESP32_Assignment2/camera_pins.h"
sensor_t sensor={};camera_config_t received={};int initError=0,frameError=0,sizeCalls=0;bool missingSensor=false;
int esp_camera_init(camera_config_t*c){received=*c;return initError;}
sensor_t*esp_camera_sensor_get(){return missingSensor?nullptr:&sensor;}
int setSize(sensor_t*,int n){assert(n==FRAMESIZE_QVGA);sizeCalls++;return frameError;}
String sent;void sendJSON(String s){sent=s;}
int owner=1;bool sampleAvailable=true,cameraRestartPending=false;uint32_t cameraRestartRequestedAt=999;
std::atomic<bool>detectEnabled{true},colorEnabled{true};
struct Web{int code=0;void send(int n,const char*,const char*){code=n;}}web;
bool cameraInit(){
  camera_config_t c={};c.ledc_channel=LEDC_CHANNEL_0;c.ledc_timer=LEDC_TIMER_0;
  c.pin_d0=Y2_GPIO_NUM;c.pin_d1=Y3_GPIO_NUM;c.pin_d2=Y4_GPIO_NUM;c.pin_d3=Y5_GPIO_NUM;
  c.pin_d4=Y6_GPIO_NUM;c.pin_d5=Y7_GPIO_NUM;c.pin_d6=Y8_GPIO_NUM;c.pin_d7=Y9_GPIO_NUM;
  c.pin_xclk=XCLK_GPIO_NUM;c.pin_pclk=PCLK_GPIO_NUM;c.pin_vsync=VSYNC_GPIO_NUM;c.pin_href=HREF_GPIO_NUM;
  c.pin_sscb_sda=SIOD_GPIO_NUM;c.pin_sscb_scl=SIOC_GPIO_NUM;c.pin_pwdn=PWDN_GPIO_NUM;c.pin_reset=RESET_GPIO_NUM;
  c.xclk_freq_hz=10000000;c.pixel_format=PIXFORMAT_JPEG;c.frame_size=FRAMESIZE_UXGA;c.jpeg_quality=12;c.fb_count=1;
  if(esp_camera_init(&c)!=ESP_OK)return false;
  // Keep the driver's image-processing defaults. Only select output dimensions
  // after allocating the large JPEG buffer required by the resolution controls.
  sensor_t*s=esp_camera_sensor_get();
  return s && s->set_framesize(s,FRAMESIZE_QVGA)==0;
}
void restoreCameraDefaults(){
 sendJSON("{\"N\":100}");owner=0;sampleAvailable=false;
 detectEnabled=false;colorEnabled=false;
 cameraRestartRequestedAt=millis();cameraRestartPending=true;
 web.send(202,"text/plain","Restoring driver defaults. ESP32 will restart; reconnect Wi-Fi and refresh this page. Calibration remains saved.");
}

int main(){
 // All ISP setter pointers remain null: any extra image-setting call fails this test.
 sensor.set_framesize=setSize;assert(cameraInit()&&sizeCalls==1);
 assert(received.frame_size==FRAMESIZE_UXGA&&received.pixel_format==PIXFORMAT_JPEG&&received.jpeg_quality==12&&received.fb_count==1&&received.xclk_freq_hz==10000000);
 initError=-1;assert(!cameraInit()&&sizeCalls==1);initError=0;
 missingSensor=true;assert(!cameraInit());missingSensor=false;
 frameError=-1;assert(!cameraInit());frameError=0;
 restoreCameraDefaults();assert(web.code==202&&cameraRestartPending&&cameraRestartRequestedAt==millis());
 assert(owner==0&&!sampleAvailable&&!detectEnabled&&!colorEnabled&&sent=="{\"N\":100}");
 puts("PASS V11 driver defaults: no ISP overrides, init failures handled, reset stops car and queues reboot.");
}
