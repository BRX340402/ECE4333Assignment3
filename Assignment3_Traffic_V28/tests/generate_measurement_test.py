from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text()
r=(root/'tests/esp_routes_generated.cpp').read_text();stub=r[:r.index('struct DriveSettings {')]
a=s[s.index('int colourRange(const RedRegion &r,float k){'):s.index('String redResults(){')]
b=s[s.index('String liveObjectsJSON(){'):s.index('void captureTask(void*){')]
checks=r'''
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
'''
# The routes harness stub provides a placeholder; replace it with the actual function.
stub=stub.replace('String liveObjectsJSON(){return "{}";}','')
(root/'tests/measurement_generated.cpp').write_text('#include <cmath>\n'+stub+'\n'+a+b+checks)
