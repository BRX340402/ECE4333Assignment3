#include <cmath>

#include <string>
#include <map>
#include <functional>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdlib>
using std::max;
#include "../ESP32_Assignment2/CalibrationStore.h"
#include "../ESP32_Assignment2/RedDetector.h"
struct String {std::string s;String(){}String(const char*x):s(x){}String(char*x):s(x){}String(std::string x):s(x){}template<class T>String(T x):s(std::to_string(x)){}String(float x,int):s(std::to_string(x)){}String& operator+=(const char*x){s+=x;return *this;}bool operator!=(const String&x)const{return s!=x.s;}bool operator!=(const char*x)const{return s!=x;}bool operator==(const char*x)const{return s==x;}friend String operator+(String a,String b){return a.s+b.s;}};
uint32_t now=1000;uint32_t millis(){return now;}
String sent;void sendJSON(const String&s){sent=s;}
struct Prefs {bool fail=false;std::map<std::string,std::vector<unsigned char>> data;size_t putBytes(const char*k,const void*p,size_t n){if(fail)return 0;const auto*b=(const unsigned char*)p;data[k]={b,b+n};return n;}size_t putFloat(const char*,float){return sizeof(float);}size_t putInt(const char*,int){return sizeof(int);}}calibration;
struct RedStatus{bool valid=true,seen=true,greenSeen=true;uint32_t captured=1000;RedRegion region,green;};RedStatus redStatus;bool resolutionBusy=false;int stateMutex=0;const int portMAX_DELAY=0;void xSemaphoreTake(int,int){}void xSemaphoreGive(int){}
struct StopMeasurement {bool valid=true,seen=true,matched=true;uint32_t completed=1000,captured=1000;int resolution=5;RedRegion region;};StopMeasurement stopMeasurement;

bool calibrationReady=true,detectEnabled=true,colorEnabled=true;int owner=0;
std::atomic<int> trafficMode{0};std::atomic<uint32_t> trafficReceived{1000};std::atomic<float> stopColourK{0},lightColourK{0},greenColourK{0};std::atomic<int> colourCalResolution{-1};
String trafficTelemetry="{}";String redResults(){return "{}";}
struct Sensor {struct {int framesize=5,colorbar=0;}status;}sensor;Sensor*esp_camera_sensor_get(){return &sensor;}
const char *TRAFFIC_PAGE="";enum{HTTP_GET,HTTP_POST};
struct Web {std::map<std::string,std::function<void()>> handlers;std::map<std::string,String> args;int code=0;String body;
 template<class F>void on(const char*p,int m,F f){handlers[std::string(p)+std::to_string(m)]=f;}
 void sendHeader(const char*,const char*){}
 void send(int c,const char*,String b){code=c;body=b;}void send_P(int c,const char*t,const char*b){send(c,t,b);}String arg(const char*k){return args[k];}
 void run(const char*p){handlers.at(std::string(p)+std::to_string(HTTP_POST))();}
}web;
bool strictInt(const String&s,int lo,int hi,int&v){if(s.s.empty()||s.s.size()>5)return false;for(char c:s.s)if(c<'0'||c>'9')return false;v=std::stoi(s.s);return v>=lo&&v<=hi;}


int colourRange(const RedRegion &r,float k){
 if(k<=0||r.w<8||r.x<=0||r.y<=0||r.x+r.w>=r.imageW||r.y+r.h>=r.imageH||colourCalResolution.load()!=esp_camera_sensor_get()->status.framesize)return -1;
 float d=k*r.imageW/r.w;return std::isfinite(d)&&d<=10000?(int)(d+0.5f):-1;
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
 }
 return out+"}";
}
// The only normal caller of esp_camera_fb_get. No JPEG decode or AI here.

int main(){
 colourCalResolution=5;lightColourK=3;greenColourK=4;stopColourK=5;
 redStatus.region.components=1;redStatus.region.x=100;redStatus.region.y=80;redStatus.region.w=40;redStatus.region.h=40;redStatus.region.pixels=600;redStatus.region.imageW=400;redStatus.region.imageH=296;
 redStatus.green=redStatus.region;redStatus.green.w=20;redStatus.green.pixels=200;
 stopMeasurement.region=redStatus.region;
 auto j=liveObjectsJSON().s;
 assert(j.find("\"red\":{\"valid\":1,\"seen\":1,\"pixels\":600,\"width\":40,\"distance_cm\":30}")!=std::string::npos);
 assert(j.find("\"green\":{\"valid\":1,\"seen\":1,\"pixels\":200,\"width\":20,\"distance_cm\":80}")!=std::string::npos);
 assert(j.find("\"stop\":{\"valid\":1,\"seen\":1,\"pixels\":600,\"width\":40,\"distance_cm\":50}")!=std::string::npos);
 colorEnabled=false;j=liveObjectsJSON().s;assert(j.find("\"stop\":{\"valid\":1,\"seen\":1")!=std::string::npos);assert(j.find("\"red\":{\"valid\":0")!=std::string::npos);colorEnabled=true;
 stopMeasurement.matched=false;j=liveObjectsJSON().s;assert(j.find("\"stop\":{\"valid\":1,\"seen\":1,\"pixels\":0,\"width\":0,\"distance_cm\":-1}")!=std::string::npos);
 stopMeasurement.matched=true;now=7000;redStatus.captured=now;j=liveObjectsJSON().s;assert(j.find("\"stop\":{\"valid\":0,\"seen\":0")!=std::string::npos);
 redStatus.seen=false;j=liveObjectsJSON().s;assert(j.find("\"stop\":{\"valid\":1,\"seen\":0")!=std::string::npos);
 now+=1500;j=liveObjectsJSON().s;assert(j.find("\"red\":{\"valid\":0,\"seen\":0,\"pixels\":0,\"width\":0,\"distance_cm\":-1}")!=std::string::npos);
 sensor.status.framesize=4;assert(colourRange(redStatus.green,4)==-1);sensor.status.framesize=5;
 redStatus.green.x=0;assert(colourRange(redStatus.green,4)==-1);
 std::cout<<"PASS V5 live metrics: independent distance scales; matched STOP extent; stale and absent targets; clipped and wrong-resolution distances.\n";
}
