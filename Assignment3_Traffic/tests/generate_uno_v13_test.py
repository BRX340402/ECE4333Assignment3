from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'UNO_Assignment2/UNO_Assignment2.ino').read_text()
stubs=root/'tests/uno_parser_stubs/avr';stubs.mkdir(parents=True,exist_ok=True)
(stubs/'pgmspace.h').write_text('#pragma once\n#define PSTR(x) x\n#define snprintf_P snprintf\n#define pgm_read_byte(p) (*(const unsigned char*)(p))\n')
prefix=r'''
#include <cassert>
#include <iostream>
#include <deque>
#include <string>
#include <vector>
#include <algorithm>
using std::min;using std::max;
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#include "../UNO_Assignment2/IntegerCommand.h"
#include "../UNO_Assignment2/Logic.h"
#include "../UNO_Assignment2/LedLogic.h"
#include "../UNO_Assignment2/TrafficLogic.h"
#include "../UNO_Assignment2/Ex3Tracking.h"
uint32_t now=100;uint32_t millis(){return now;}
TrafficLogic traffic;Ex3Tracking ex3;VisionState vision;IndicatorState indicator;
struct Pose{bool gyro=true;float gyroSign=-1;void camera(float,float){}}pose;
struct CRGB{int r,g,b;CRGB(int R=0,int G=0,int B=0):r(R),g(G),b(B){}}manualLed;
uint8_t manualBrightness=20,mode=0,reason=0;int speedSetting=90,manualDir=9,panAngle=90;
bool aiGuard=false,remoteOwned=false,irOwned=false,settingsReady=false,stopObserved=false,aiResultSeen=false;
int configReply=-1,stopRange=-1,bearingMilli=0;uint16_t aiResultSeq=0;uint32_t lastRemote=0;
int lineL=750,lineM=751,lineR=752,rangeCm=-1,batteryMv=4350,leftPWM=0,rightPWM=0;
char input[160],txBuffer[256];uint8_t inputPos=0,txPos=0,txLen=0;bool receiving=false;int replyValue=-1;
void motorOutput(int l,int r){leftPWM=l;rightPWM=r;}
void standby(){mode=0;remoteOwned=false;traffic.stop(now);}
void chooseMode(uint8_t m,bool owned){mode=m;remoteOwned=owned;if(m==6)traffic.start(now);}
void setPan(int angle){panAngle=angle;}
void updateStopLed(){}
void reply(bool ok){replyValue=ok;}
struct SerialMock{std::deque<char>rx;std::string sent;int available(){return rx.size();}int read(){int c=rx.front();rx.pop_front();return c;}int availableForWrite(){return 7;}void write(char c){sent+=c;}}Serial;
'''
telemetry=s[s.index('void telemetry(){'):s.index('int8_t pendingReply=')]
functions=s[s.index('void command(char *text){'):s.index('void sensors(){')]
checks=r'''
void call(std::string text){std::vector<char>b(text.begin(),text.end());b.push_back(0);command(b.data());}
void feed(std::string text){for(char c:text)Serial.rx.push_back(c);while(Serial.available()){auto before=Serial.rx.size();serialInput();assert(before-Serial.rx.size()<=64);}}
int main(){
 IntegerCommand d;
 assert(d.begin(" {\"N\":200,\"S\":65535,\"D\":-1,\"MIN\":-2147483648,\"MAX\":2147483647} "));
 assert(d[PSTR("S")].isUInt16()&&!d[PSTR("S")].isInt16());assert((long)d[PSTR("MIN")]==-2147483648L);
 assert(!d.containsKey(PSTR("MISS"))&&d[PSTR("MISS")].orDefault(90)==90);
 for(const char*t:{"","{","}","{\"N\":1,}","{\"N\":1}x","{\"N\":1,\"N\":2}","{\"N\":true}","{\"N\":1.0}","{\"N\":+1}","{\"N\":01}","{\"N\":2147483648}","{\"N\":-2147483649}","{\"N\":{}}","{\"N\":null}","{\"N\":1e3}","{\"N\":\"1\"}"})assert(!d.begin(t));
 std::string sample="{\"N\":206,\"ID\":3,\"SC\":35,\"BL\":600,\"BH\":850,\"RA\":90,\"SA\":180,\"CT\":1500,\"GS\":1,\"CS\":60,\"RS\":0,\"DS\":40}";
 for(size_t n=0;n<sample.size();n++){auto t=sample.substr(0,n);assert(!d.begin(t.c_str()));}
 call(sample);assert(traffic.cruiseSpeed==60&&!traffic.stopOnRed);assert(settingsReady&&configReply==3&&traffic.stopCm==35&&pose.gyroSign==1);
 call("{\"N\":204,\"S\":65535,\"V\":1,\"R\":0,\"G\":1,\"D\":-1,\"SD\":-1,\"B\":0}");assert(traffic.fresh(now));
 pose.gyro=false;traffic.seen=false;call("{\"N\":101,\"D1\":6}");assert(mode==6&&traffic.state==TrafficLogic::TRACK);

 call("{\"N\":204,\"S\":1,\"V\":1,\"R\":1,\"G\":0,\"D\":10,\"SD\":10,\"B\":0,\"LC\":0}");assert(!traffic.lightCalibrated&&traffic.moving());
 call("{\"N\":204,\"S\":2,\"V\":1,\"R\":1,\"G\":0,\"D\":10,\"SD\":10,\"B\":0,\"LC\":1}");assert(traffic.lightCalibrated&&!traffic.moving());
 call("{\"N\":100}");assert(mode==0);
 call("{\"N\":102,\"D1\":1,\"D2\":60}");assert(mode==5&&speedSetting==60);
 call("{\"N\":102,\"D1\":65537,\"D2\":60}");assert(mode==5&&speedSetting==60);
 call("{\"N\":100}");assert(mode==0);
 for(int i=0;i<10000;i++){
  unsigned seq=(65535u+i)&65535u;
  std::string packet="{\"N\":200,\"S\":"+std::to_string(seq)+",\"A\":1,\"V\":1,\"P\":900,\"R\":30,\"X\":120,\"Y\":90,\"W\":20,\"H\":20,\"O\":1}";
  assert(packet.size()<sizeof(input));feed(packet);assert(vision.seq==seq&&vision.confidence==900&&stopObserved);
  now+=10;
 }
 auto sequence=vision.seq;call("{\"N\":200,\"S\":1}");assert(vision.seq==sequence);
 feed(std::string(250,'x')+"{\"N\":103}");assert(lastRemote==now);
 feed("{"+std::string(200,'x')+"}{\"N\":100}");assert(mode==0&&!receiving);
 feed("{\"N\":20{\"N\":103}");assert(lastRemote==now);
 telemetry();assert(txLen>0&&txLen<sizeof(txBuffer));while(txPos<txLen)pumpTelemetry();assert(Serial.sent.find("{\"N\":210,")==0&&Serial.sent.back()=='\n');

 call("{\"N\":100}");
 call("{\"N\":206,\"ID\":4,\"SC\":35,\"BL\":600,\"BH\":850,\"RA\":90,\"SA\":180,\"CT\":1500,\"GS\":1,\"CS\":60,\"RS\":1,\"DS\":40}");
 assert(traffic.stopOnRed&&traffic.cruiseSpeed==60);traffic.seen=false;
 call("{\"N\":101,\"D1\":6}");leftPWM=rightPWM=60;
 call("{\"N\":204,\"S\":1,\"V\":1,\"R\":1,\"G\":0,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");
 assert(traffic.state==TrafficLogic::VERIFY&&!traffic.moving()&&leftPWM==0&&rightPWM==0);
 aiResultSeen=false;std::string req=std::to_string(traffic.request);
 auto ai=[&](int seq,int score){call("{\"N\":205,\"S\":"+std::to_string(seq)+",\"ID\":"+req+",\"V\":1,\"P\":"+std::to_string(score)+"}");};
 ai(100,900);ai(100,900);assert(traffic.positives==1&&!traffic.moving());
 ai(101,0);ai(102,0);assert(traffic.state==TrafficLogic::WAIT_RED&&traffic.positives==1);
 ai(103,650);assert(traffic.state==TrafficLogic::CROSS&&traffic.moving());
 assert(traffic.crossingPWM()==60);

 puts("PASS V13: strict integer JSON and overflow/truncation/duplicate rejection; current settings, colour, mode, vision, heartbeat commands; 10000 sequential camera packets across sequence wrap; RX quota, overlength recovery and chunked telemetry.");
}
'''
(root/'tests/uno_v13_generated.cpp').write_text(prefix+telemetry+functions+checks)
