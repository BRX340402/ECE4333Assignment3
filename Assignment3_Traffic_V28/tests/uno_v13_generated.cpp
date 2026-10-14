
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
void telemetry(){
 if(txPos<txLen)return;
 int n=snprintf_P(txBuffer,sizeof(txBuffer),PSTR("{\"N\":210,\"M\":%u,\"B\":%u,\"G\":%u,\"T\":%u,\"U\":%d,\"L\":%d,\"C\":%d,\"R\":%d,\"V\":%d,\"S\":%d,\"Q\":%u,\"A\":%d,\"D\":%d,\"AI\":%u,\"VF\":%u,\"P\":%d,\"VD\":%d,\"LED\":%u}\n"),
  mode,reason,aiGuard,vision.latched,rangeCm,lineL,lineM,lineR,batteryMv,panAngle,vision.seq,leftPWM,rightPWM,(unsigned int)vision.ai,(unsigned int)vision.fresh(millis()),vision.confidence,vision.distance,(unsigned int)indicator.mode(millis()));
 if(n<=0||n>=(int)sizeof(txBuffer)){txPos=txLen=0;return;}txLen=(uint8_t)n;
 txPos=0;
}
void pumpTelemetry(){
 int available=Serial.availableForWrite();
 while(txPos<txLen&&available-->0)Serial.write(txBuffer[txPos++]);
}
void command(char *text){
 IntegerCommand doc;
 if(!doc.begin(text))return;
 if(!doc[PSTR("N")].isInt16())return;
 const char *optional[]={PSTR("D1"),PSTR("D2"),PSTR("D3"),PSTR("D4")};
 for(uint8_t i=0;i<4;i++)if(doc.containsKey(optional[i])&&!doc[optional[i]].isInt16())return;
 int n=doc[PSTR("N")],d=doc[PSTR("D1")].orDefault(0);
 if(n==206){
  const char *keys[]={PSTR("ID"),PSTR("SC"),PSTR("BL"),PSTR("BH"),PSTR("RA"),PSTR("SA"),PSTR("CT"),PSTR("GS"),PSTR("CS"),PSTR("RS"),PSTR("DS")};
  for(uint8_t i=0;i<11;i++)if(!doc[keys[i]].isInt16())return;
  int id=doc[PSTR("ID")],sc=doc[PSTR("SC")],bl=doc[PSTR("BL")],bh=doc[PSTR("BH")],ra=doc[PSTR("RA")],sa=doc[PSTR("SA")],ct=doc[PSTR("CT")],gs=doc[PSTR("GS")],cs=doc[PSTR("CS")],rs=doc[PSTR("RS")],ds=doc[PSTR("DS")];
  if(mode!=0||id<1||id>30000||sc<5||sc>150||bl<0||bh>950||bl>=bh||ra<30||ra>90||sa<90||sa>360||ct<300||ct>5000||gs<0||gs>1||cs<50||cs>150||rs<0||rs>1||ds<0||ds>cs)return;
  traffic.slowSpeed=ds;traffic.cruiseSpeed=cs;traffic.stopOnRed=rs;traffic.stopCm=sc;ex3.blackLow=bl;ex3.blackHigh=bh;ex3.rightDegrees=ra;ex3.sweepDegrees=sa;traffic.crossMinimumMs=ct;pose.gyroSign=gs?1.0f:-1.0f;ex3.reset();settingsReady=true;configReply=id;return;
 }
 if(n==204){
  if(!doc[PSTR("S")].isUInt16()||!doc[PSTR("V")].isInt16()||!doc[PSTR("R")].isInt16()||!doc[PSTR("G")].isInt16()||!doc[PSTR("D")].isInt16()||!doc[PSTR("SD")].isInt16()||!doc[PSTR("B")].isInt16())return;
  int v=doc[PSTR("V")],r=doc[PSTR("R")],g=doc[PSTR("G")],d=doc[PSTR("D")],sd=doc[PSTR("SD")],b=doc[PSTR("B")];
  if(v<0||v>1||r<0||r>1||g<0||g>1||d< -1||d>10000||sd< -1||sd>10000||b< -1600||b>1600)return;
  if(doc.containsKey(PSTR("LC"))&&(!doc[PSTR("LC")].isInt16()||(long)doc[PSTR("LC")]<0||(long)doc[PSTR("LC")]>1))return;
  bool calibrated=doc.containsKey(PSTR("LC"))&&(long)doc[PSTR("LC")]==1;
  traffic.colour(doc[PSTR("S")],v,r,g,d,millis(),calibrated);
  if(mode==6&&traffic.stopOnRed&&!traffic.moving())motorOutput(0,0);
  stopRange=sd;bearingMilli=b;return;
 }
 if(n==205){
  if(mode!=6||!doc[PSTR("S")].isUInt16()||!doc[PSTR("ID")].isUInt16()||!doc[PSTR("V")].isInt16()||!doc[PSTR("P")].isInt16())return;
  uint16_t sq=doc[PSTR("S")],id=doc[PSTR("ID")];int v=doc[PSTR("V")],p=doc[PSTR("P")];
  if(v<0||v>1||p<0||p>1000||id!=traffic.request||(traffic.state!=TrafficLogic::VERIFY&&!(traffic.stopOnRed&&traffic.state==TrafficLogic::WAIT_RED)))return;
  if(aiResultSeen){uint16_t delta=sq-aiResultSeq;if(!delta||delta>=32768)return;}
  aiResultSeen=true;aiResultSeq=sq;
  traffic.inference(id,v,traffic.stopOnRed?p>0:p>=800,millis(),stopRange);
  if(traffic.state==TrafficLogic::STOP_HOLD)pose.camera(stopRange,bearingMilli*0.001f);
  return;
 }
 if(n==200){
  const char *keys[]={PSTR("S"),PSTR("A"),PSTR("V"),PSTR("P"),PSTR("R"),PSTR("X"),PSTR("Y"),PSTR("W"),PSTR("H")};
  for(uint8_t i=0;i<9;i++)if(!doc[keys[i]].isInt32())return;
  long seq=doc[PSTR("S")],a=doc[PSTR("A")],v=doc[PSTR("V")],p=doc[PSTR("P")],dist=doc[PSTR("R")],x=doc[PSTR("X")],y=doc[PSTR("Y")],w=doc[PSTR("W")],h=doc[PSTR("H")];
  if(seq<0||seq>65535||a<0||a>1||v<0||v>1||p<0||p>1000||dist< -1||dist>10000||x< -1||x>=400||y< -1||y>=400||w<0||w>400||h<0||h>400)return;
  if(p>0&&(x<0||y<0||w<1||h<1))return;
  if(doc.containsKey(PSTR("O"))&&(!doc[PSTR("O")].isInt16()||((int)doc[PSTR("O")]!=0&&(int)doc[PSTR("O")]!=1)))return;
  if(vision.update(seq,a,v,p,dist,x,y,w,h,millis())){stopObserved=a&&p>0&&(doc.containsKey(PSTR("O"))?(int)doc[PSTR("O")]!=0:v!=0);indicator.setStop(stopObserved,millis());}
  return;
 }
 if(n==203){if(!doc[PSTR("R")].isInt16())return;int r=doc[PSTR("R")];if(r<0||r>1)return;indicator.setRed(r!=0,millis());return;}
 if(n==103){lastRemote=millis();return;}
 if(n==100||n==110){standby();reply(true);return;}
 if(n==101){
  if(d<0||d>6||d==5){reply(false);return;}
  if(d==6&&mode!=0){reply(false);return;} // Start only from standby, never reset an active traffic stop.
  if(d==6&&!settingsReady){reply(false);return;}
  if(d==4&&!vision.fresh(millis())){reply(false);return;}
  if(d==0)standby();else chooseMode(d,true);reply(true);return;
 }
 if(n==102){
  irOwned=false;
  if(d<1||d>9){reply(false);return;}
  if(d==9){standby();return;}
  if(doc.containsKey(PSTR("D2"))){int sp=doc[PSTR("D2")];if(sp<0||sp>DRIVE_LIMIT)return;speedSetting=sp;}
  mode=5;manualDir=d;remoteOwned=true;lastRemote=millis();return;
 }
 if(n==104){if(d<0||d>DRIVE_LIMIT)return;speedSetting=d;reply(true);return;}
 if(n==5){int angle=doc[PSTR("D2")].orDefault(90);if(d!=1||angle<0||angle>180){reply(false);return;}standby();setPan(angle);reply(true);return;}
 if(n==106){if(d!=1&&d!=3&&d!=5){reply(false);return;}standby();setPan(d==5?90:panAngle+(d==1?15:-15));reply(true);return;}
 if(n==8){ // same original D1 selector/D2,D3,D4 RGB layout, single LED
  int r=doc[PSTR("D2")].orDefault(0),g=doc[PSTR("D3")].orDefault(0),b=doc[PSTR("D4")].orDefault(0);
  if(r<0||r>255||g<0||g>255||b<0||b>255)return;
  manualLed=CRGB(r,g,b);updateStopLed();reply(true);return;
 }
 if(n==105){if(d==1)manualBrightness=min(100,(int)manualBrightness+5);else if(d==2)manualBrightness=max(0,(int)manualBrightness-5);updateStopLed();return;}
 if(n==201){
  standby();
  if(d!=0&&d!=1){reply(false);return;}
  if(d&&!vision.fresh(millis())){reply(false);return;}
  aiGuard=d;reply(true);return;
 }
 if(n==202){standby();reply(vision.reset(millis()));return;}
 if(n==21||n==22||n==210){telemetry();return;}
 reply(false);
}
void serialInput(){
 uint8_t budget=64; // Bound each pass so continuous RX cannot starve motor/watchdog/TX work.
 while(budget--&&Serial.available()){
  char c=Serial.read();if(c=='{'){inputPos=0;receiving=true;}
  if(!receiving)continue;
  if(inputPos>=sizeof(input)-1){receiving=false;continue;}
  input[inputPos++]=c;
  if(c=='}'){input[inputPos]=0;receiving=false;command(input);}
 }
}

void call(std::string text){std::vector<char>b(text.begin(),text.end());b.push_back(0);command(b.data());}
void feed(std::string text){for(char c:text)Serial.rx.push_back(c);while(Serial.available()){auto before=Serial.rx.size();serialInput();assert(before-Serial.rx.size()<=64);}}
int main(){
 IntegerCommand d;
 assert(d.begin(" {\"N\":200,\"S\":65535,\"D\":-1,\"MIN\":-2147483648,\"MAX\":2147483647} "));
 assert(d[PSTR("S")].isUInt16()&&!d[PSTR("S")].isInt16());assert((long)d[PSTR("MIN")]==-2147483648L);
 assert(!d.containsKey(PSTR("MISS"))&&d[PSTR("MISS")].orDefault(90)==90);
 for(const char*t:{"","{","}","{\"N\":1,}","{\"N\":1}x","{\"N\":1,\"N\":2}","{\"N\":true}","{\"N\":1.0}","{\"N\":+1}","{\"N\":01}","{\"N\":2147483648}","{\"N\":-2147483649}","{\"N\":{}}","{\"N\":null}","{\"N\":1e3}","{\"N\":\"1\"}"})assert(!d.begin(t));
 std::string sample="{\"N\":206,\"ID\":3,\"SC\":35,\"BL\":600,\"BH\":850,\"RA\":90,\"SA\":360,\"CT\":1500,\"GS\":1,\"CS\":60,\"RS\":0,\"DS\":40}";
 for(size_t n=0;n<sample.size();n++){auto t=sample.substr(0,n);assert(!d.begin(t.c_str()));}
 call(sample);assert(traffic.cruiseSpeed==60&&!traffic.stopOnRed);assert(ex3.sweepDegrees==360);assert(settingsReady&&configReply==3&&traffic.stopCm==35&&pose.gyroSign==1);
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
 call("{\"N\":206,\"ID\":4,\"SC\":35,\"BL\":600,\"BH\":850,\"RA\":90,\"SA\":360,\"CT\":1500,\"GS\":1,\"CS\":60,\"RS\":1,\"DS\":40}");
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
