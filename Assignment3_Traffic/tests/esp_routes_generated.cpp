
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
struct RedStatus{bool valid=true,seen=true,greenSeen=true;uint32_t captured=1000;RedRegion region,green;};RedStatus redStatus;bool resolutionBusy=false,wbBusy=false;int stateMutex=0;const int portMAX_DELAY=0;void xSemaphoreTake(int,int){}void xSemaphoreGive(int){}
struct StopMeasurement {bool valid=true,seen=true,matched=true;uint32_t completed=1000,captured=1000;int resolution=5;RedRegion region;};StopMeasurement stopMeasurement;
String liveObjectsJSON(){return "{}";}
bool calibrationReady=true,detectEnabled=true,colorEnabled=true;int owner=0;
std::atomic<int> trafficMode{0};std::atomic<uint32_t> trafficReceived{1000};std::atomic<float> stopColourK{0},lightColourK{0},greenColourK{0};std::atomic<int> colourCalResolution{-1};
int unoFirmware=24;String trafficTelemetry="{}";String redResults(){return "{}";}
struct Sensor {struct {int framesize=5,colorbar=0;}status;}sensor;Sensor*esp_camera_sensor_get(){return &sensor;}
const char *TRAFFIC_PAGE="";enum{HTTP_GET,HTTP_POST};
struct Web {std::map<std::string,std::function<void()>> handlers;std::map<std::string,String> args;int code=0;String body;
 template<class F>void on(const char*p,int m,F f){handlers[std::string(p)+std::to_string(m)]=f;}
 void sendHeader(const char*,const char*){}
 void send(int c,const char*,String b){code=c;body=b;}void send_P(int c,const char*t,const char*b){send(c,t,b);}String arg(const char*k){return args[k];}
 void run(const char*p){handlers.at(std::string(p)+std::to_string(HTTP_POST))();}
}web;
bool strictInt(const String&s,int lo,int hi,int&v){if(s.s.empty()||s.s.size()>6)return false;for(char c:s.s)if(c<'0'||c>'9')return false;v=std::stoi(s.s);return v>=lo&&v<=hi;}

struct DriveSettings {int sc=25,bl=600,bh=850,ra=120,sa=240,ct=1500,gs=0,cs=60,rs=0,ds=40,rpx=100;};
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

void install(){
 web.on("/traffic-settings",HTTP_GET,[]{web.send(200,"application/json",driveSettingsJSON());});
 web.on("/traffic-settings",HTTP_POST,[]{
  if(trafficMode!=0||millis()-trafficReceived.load()>1500){sendJSON("{\"N\":100}");owner=0;web.send(409,"text/plain","Stop requested. Wait for UNO OFF, then save again.");return;}
  DriveSettings next;
  if(!strictInt(web.arg("SC"),5,150,next.sc)||!strictInt(web.arg("BL"),0,949,next.bl)||!strictInt(web.arg("BH"),1,950,next.bh)||next.bl>=next.bh||!strictInt(web.arg("RA"),30,120,next.ra)||!strictInt(web.arg("SA"),60,240,next.sa)||next.sa>next.ra+120||!strictInt(web.arg("CT"),300,5000,next.ct)||!strictInt(web.arg("GS"),0,1,next.gs)||!strictInt(web.arg("CS"),50,150,next.cs)||!strictInt(web.arg("RS"),0,1,next.rs)||!strictInt(web.arg("DS"),0,next.cs,next.ds)||!strictInt(web.arg("RP"),0,160000,next.rpx)){web.send(400,"text/plain","Invalid settings; black low must be less than black high.");return;}
  if(!calibrationReady||calibration.putBytes("a3drive24",&next,sizeof(next))!=sizeof(next)){web.send(500,"text/plain","Save failed");return;}
  driveSettings=next;settingsID=settingsID%30000+1;settingsApplied=false;sendDriveSettings();web.send(200,"text/plain","Saved. Waiting for UNO acknowledgement; check Applied status.");
 });
 web.on("/traffic",HTTP_GET,[]{web.send_P(200,"text/html",TRAFFIC_PAGE);});
 web.on("/traffic-status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",String("{\"vision_health\":")+redResults()+",\"uno\":"+trafficTelemetry+",\"age_ms\":"+String(millis()-trafficReceived.load())+",\"calibration\":"+calibrationJSON()+",\"settings\":"+driveSettingsJSON()+",\"framesize\":"+String(esp_camera_sensor_get()->status.framesize)+",\"objects\":"+liveObjectsJSON()+",\"detect_enabled\":"+String((bool)detectEnabled)+",\"color_enabled\":"+String((bool)colorEnabled)+"}");});
 web.on("/traffic-start",HTTP_POST,[]{
  if(esp_camera_sensor_get()->status.colorbar){web.send(409,"text/plain","Turn off sensor test colour bars before driving");return;}
  if(trafficMode!=0){web.send(409,"text/plain","Already active. Use STOP CAR before starting a new run.");return;}
  if(driveSettings.rs&&(!detectEnabled||!colorEnabled)){web.send(409,"text/plain","Enable STOP AI and red/green detection for Stop on red mode");return;}
  if(!settingsApplied||millis()-trafficReceived.load()>1500){web.send(409,"text/plain","Wait for fresh UNO telemetry and settings acknowledgement");return;}
  if(unoFirmware<24){web.send(409,"text/plain","Upload UNO V24 first; mode and speed must be acknowledged by UNO");return;}
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

}

int main(){install();web.args={{"SC","35"},{"BL","650"},{"BH","850"},{"RA","90"},{"SA","180"},{"CT","2000"},{"GS","1"},{"CS","60"},{"RS","0"},{"DS","40"},{"RP","100"}};
 web.run("/traffic-settings");assert(web.code==200&&driveSettings.sc==35&&driveSettings.bl==650&&!settingsApplied);assert(sent.s.find("\"SC\":35")!=std::string::npos);
 assert(driveSettings.cs==60&&!driveSettings.rs&&sent.s.find("\"CS\":60")!=std::string::npos);
 web.args["RS"]="1";web.run("/traffic-settings");assert(web.code==200&&driveSettings.rs==1&&sent.s.find("\"RS\":1")!=std::string::npos);
 web.args["CS"]="49";web.run("/traffic-settings");assert(web.code==400&&driveSettings.cs==60);
 web.args["CS"]="60";web.args["RS"]="0";web.run("/traffic-settings");assert(web.code==200&&!driveSettings.rs);
 web.args["DS"]="0";web.args["RP"]="160000";web.run("/traffic-settings");assert(web.code==200&&driveSettings.ds==0&&driveSettings.rpx==160000);
 web.args["RP"]="160001";web.run("/traffic-settings");assert(web.code==400);
 web.args["DS"]="40";web.args["RP"]="100";web.run("/traffic-settings");assert(web.code==200);
 web.args["BH"]="500";web.run("/traffic-settings");assert(web.code==400&&driveSettings.bh==850);
 trafficMode=6;web.args["BH"]="850";web.run("/traffic-settings");assert(web.code==409&&sent.s=="{\"N\":100}");trafficMode=0;
 redStatus.region.components=1;redStatus.region.x=50;redStatus.region.y=50;redStatus.region.w=40;redStatus.region.h=40;redStatus.region.imageW=400;redStatus.region.imageH=296;
 stopMeasurement.region=redStatus.region;
 web.args={{"profile","stop"}};stopMeasurement.seen=false;web.run("/traffic-sample");assert(web.code==409);stopMeasurement.seen=true;web.run("/traffic-sample");assert(web.code==200&&pendingCalibration.width==40&&sampleToken==1);
 // Live width changes after sampling: saving MUST still use the frozen 40 px.
 redStatus.region.w=80;web.args={{"profile","stop"},{"cm","30"},{"token","1"}};web.run("/traffic-calibrate");assert(web.code==200&&stopColourK==3.0f&&savedCalibration.stop.width==40&&savedCalibration.stop.cm==30&&!sampleAvailable);
 assert(calibrationJSON().s.find(std::string(1, char(34))+"width"+char(34)+":40")!=std::string::npos);
 CalibrationStore reboot;auto bytes=calibration.data.at("a3cal5");memcpy(&reboot,bytes.data(),sizeof(reboot));assert(reboot.stop.width==40&&reboot.stop.cm==30&&reboot.stop.valid());
 web.run("/traffic-calibrate");assert(web.code==400); // no reuse of a consumed sample
 redStatus.region.w=40;
 web.args={{"profile","stop"}};web.run("/traffic-sample");sensor.status.framesize=4;web.args={{"profile","stop"},{"cm","30"},{"token","2"}};web.run("/traffic-calibrate");assert(web.code==409&&savedCalibration.stop.width==40);sensor.status.framesize=5;
 web.args={{"profile","stop"}};web.run("/traffic-sample");web.args={{"profile","stop"},{"cm","50"},{"token","3"}};calibration.fail=true;web.run("/traffic-calibrate");assert(web.code==500&&savedCalibration.stop.cm==30);calibration.fail=false;
 web.run("/traffic-start");assert(web.code==409);settingsApplied=true;web.run("/traffic-start");assert(web.code==200&&sent.s=="{\"N\":101,\"D1\":6}");
 detectEnabled=colorEnabled=false;web.run("/traffic-start");assert(web.code==200);unoFirmware=13;web.run("/traffic-start");assert(web.code==409);unoFirmware=24;detectEnabled=colorEnabled=true;
 trafficMode=6;sent="unchanged";web.run("/traffic-start");assert(web.code==409&&sent.s=="unchanged");trafficMode=0;
 // Green uses its own region / width and stores independently of red and STOP.
 redStatus.green=redStatus.region;redStatus.green.w=32;redStatus.region.w=40;
 web.args={{"profile","green"}};web.run("/traffic-sample");assert(web.code==200&&pendingCalibration.width==32);int greenToken=sampleToken;
 web.args={{"profile","green"},{"cm","50"},{"token",String(greenToken)}};web.run("/traffic-calibrate");assert(web.code==200&&savedCalibration.green.width==32&&greenColourK==4.0f&&savedCalibration.stop.width==40);
 web.args={{"profile","light"}};web.run("/traffic-sample");assert(web.code==200);int redToken=sampleToken;
 web.args={{"profile","light"},{"cm","60"},{"token",String(redToken)}};web.run("/traffic-calibrate");assert(web.code==200&&lightColourK==6.0f&&savedCalibration.green.cm==50&&savedCalibration.stop.cm==30);
 web.args={{"profile","green"}};web.run("/traffic-clear");assert(web.code==200&&!savedCalibration.green.valid()&&greenColourK==0&&savedCalibration.light.valid()&&savedCalibration.stop.valid());
 std::cout<<"PASS V5 routes: three independent target profiles, STOP requires confirmation, selected clear; active-run restart rejected; frozen scanned width, persisted width/distance, sample token, resolution change, save failure; settings validation and serial payload, reject changes while driving, manual distance formula, stale resolution rejection, start requires acknowledgement.\n";
}
