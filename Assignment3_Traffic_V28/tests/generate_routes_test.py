from pathlib import Path
root=Path(__file__).resolve().parents[1]
src=(root/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text()
settings=src[src.index('struct DriveSettings {'):src.index('String trafficTelemetry=')]
routes=src[src.index(' web.on("/traffic-settings"'):src.index(' web.on("/calibrate",HTTP_POST,calibrateDistance);')]
stub=r'''
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
'''
tests=r'''
int main(){install();web.args={{"SC","35"},{"BL","650"},{"BH","850"},{"RA","90"},{"SA","180"},{"CT","2000"},{"GS","1"},{"CS","60"},{"RS","0"},{"DS","40"},{"RP","100"}};
 web.run("/traffic-settings");assert(web.code==200&&driveSettings.sc==35&&driveSettings.bl==650&&!settingsApplied);assert(sent.s.find("\"SC\":35")!=std::string::npos);
 assert(driveSettings.cs==60&&!driveSettings.rs&&sent.s.find("\"CS\":60")!=std::string::npos);
 web.args["SA"]="360";web.run("/traffic-settings");assert(web.code==200&&driveSettings.sa==360);
 web.args["SA"]="361";web.run("/traffic-settings");assert(web.code==400&&driveSettings.sa==360);
 web.args["SA"]="180";
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
'''
(root/'tests/esp_routes_generated.cpp').write_text(stub+'\n'+settings+'\nvoid install(){\n'+routes+'\n}\n'+tests)
