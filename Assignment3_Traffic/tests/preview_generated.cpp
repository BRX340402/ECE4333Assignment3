
#include <string>
#include <cstdint>
#include <map>
#include <vector>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <iostream>
using std::min;
struct String{std::string s;String(const char*x):s(x){}String(uint32_t x):s(std::to_string(x)){}bool operator==(String x)const{return s==x.s;}};
uint32_t now=1000;uint32_t millis(){return now;}
const int portMAX_DELAY=0;int stateMutex=0;void xSemaphoreTake(int,int){}void xSemaphoreGive(int){}
#define pdMS_TO_TICKS(x) (x)
void vTaskDelay(int n){now+=n;}
void *ps_malloc(size_t n){return malloc(n);}
#include "../ESP32_Assignment2/JpegBuffer.h"
uint16_t previewEpoch=0,geometryEpoch=0;
bool resolutionBusy=false;uint8_t *previewJpeg=nullptr;size_t previewLen=0;uint32_t previewCaptured=900;
std::vector<uint8_t> received;bool stopped=false,fail=false;
struct Client{bool connected(){return !stopped;}void stop(){stopped=true;}size_t write(const uint8_t*p,size_t n){if(fail)return 0;n=min(n,(size_t)37);received.insert(received.end(),p,p+n);return n;}};
struct Server{int status=0;size_t length=0;bool hasAfter=false;std::string after;std::map<std::string,std::string> headers;
bool hasArg(const char*){return hasAfter;}String arg(const char*){return after.c_str();}void sendHeader(const char*k,String v){headers[k]=v.s;}void send(int n,const char*,const char*){status=n;}void setContentLength(size_t n){length=n;}Client client(){return {};}}previewWeb;
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

int main(){std::vector<uint8_t> jpeg(4100,42);previewJpeg=jpeg.data();previewLen=jpeg.size();
 sendLivePreview();assert(previewWeb.status==200&&received==jpeg&&!stopped);assert(previewWeb.headers["Access-Control-Allow-Origin"]=="*");
 received.clear();previewWeb.hasAfter=true;previewWeb.after="900";sendLivePreview();assert(previewWeb.status==204&&received.empty());
 previewWeb.hasAfter=false;now=3000;sendLivePreview();assert(previewWeb.status==503&&received.empty());
 now=1000;fail=true;sendLivePreview();assert(stopped&&received.empty());
 std::cout<<"PASS V12 preview: partial socket writes completed; duplicate204; stale503; failure closes connection; JPEG CORS headers.\n";
}
