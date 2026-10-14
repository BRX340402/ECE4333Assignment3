#include <Arduino.h>
// ECE 4333 Assignment 3 TEST 3: Test 2 tracking + light safety/pose/logging.
#include <avr/pgmspace.h>
#include <Servo.h>
#include <FastLED.h>
#include "IRremote.h"
#include "IntegerCommand.h"
#include "Hardware.h"
#include "Logic.h"
#include "LedLogic.h"
#include "TrafficLogic.h"
#include "Ex3Tracking.h"
#include "CrossingControl.h"
#include "PoseEstimate.h"
#include "Test3Options.h"
TrafficLogic traffic; Ex3Tracking ex3; PoseEstimate pose;
CrossingControl crossing;TrafficLogic::State previousTraffic=TrafficLogic::OFF;
bool settingsReady=false;int configReply=-1;
int stopRange=-1,bearingMilli=0;
uint16_t aiResultSeq=0; bool aiResultSeen=false;
uint32_t lastTrafficTx=0;
int freeRam(){
#if defined(__AVR__)
 extern char __heap_start;extern char *__brkval;char top;
 return &top-(__brkval?__brkval:&__heap_start);
#else
 return -1;
#endif
}
IndicatorState indicator;
Servo pan;CRGB led[1];IRrecv remote(IR_PIN);decode_results irResult;
VisionState vision;
bool stopObserved=false;
bool eStopLatched=false; uint8_t eStopReason=0; // 1=web, 2=ultrasonic
// LED: confirmed STOP solid red > red region blinking red > manual colour (off by default).
CRGB manualLed=CRGB::Black;
uint8_t manualBrightness=20;
void updateStopLed(){
 uint8_t ledMode=indicator.mode(millis());
 CRGB target=ledMode?(indicator.on(millis())?CRGB(255,0,0):CRGB::Black):manualLed;
 uint8_t brightness=ledMode?max((uint8_t)20,manualBrightness):manualBrightness;
 if(led[0].r!=target.r||led[0].g!=target.g||led[0].b!=target.b||FastLED.getBrightness()!=brightness){
  led[0]=target;FastLED.setBrightness(brightness);FastLED.show();
 }
}
// mode:0 standby,1 floor-sensor tracking,2 ultrasonic avoidance,3 range following,
// 4 floor tracking with AI stop guard,5 manual/rocker.
uint8_t mode=0,reason=0;int speedSetting=90,manualDir=9,panAngle=90;
bool aiGuard=false,remoteOwned=false,irOwned=false;uint32_t lastRemote=0,lastTelemetry=0;
int lineL=0,lineM=0,lineR=0,rangeCm=-1,batteryMv=0,leftPWM=0,rightPWM=0;
char input[160];uint8_t inputPos=0;bool receiving=false;
uint8_t avoidPhase=0;uint32_t avoidAt=0;int rangeLeft=-1,rangeRight=-1;
void setPan(int angle){panAngle=constrain(angle,0,180);pan.write(panAngle);}
void motorOutput(int left,int right){
 // Final hardware gate: no action can restart a latched software E-stop.
 if(eStopLatched)left=right=0;
 left=constrain(left,-DRIVE_LIMIT,DRIVE_LIMIT);right=constrain(right,-DRIVE_LIMIT,DRIVE_LIMIT);
 // Optional right-wheel trim for low-speed torque testing. Preserve sign/zero.
 if(right!=0){int a=abs(right)+RIGHT_PWM_TRIM;if(a>DRIVE_LIMIT)a=DRIVE_LIMIT;right=right>0?a:-a;}
 digitalWrite(MOTOR_STBY,(left||right)?HIGH:LOW);
 digitalWrite(MOTOR_LEFT_DIR,left>=0?HIGH:LOW);digitalWrite(MOTOR_RIGHT_DIR,right>=0?HIGH:LOW);
 analogWrite(MOTOR_LEFT_PWM,abs(left));analogWrite(MOTOR_RIGHT_PWM,abs(right));leftPWM=left;rightPWM=right;
}
void standby(){traffic.stop(millis());ex3.reset();irOwned=false;mode=0;manualDir=9;avoidPhase=0;remoteOwned=false;motorOutput(0,0);}
void latchEStop(uint8_t why){standby();eStopLatched=true;eStopReason=why;traffic.fail(TrafficLogic::UNKNOWN_FAULT,millis());motorOutput(0,0);}
void chooseMode(uint8_t m,bool fromRemote){
 if(eStopLatched)return;
 motorOutput(0,0);traffic.stop(millis());ex3.reset();irOwned=false;mode=m;manualDir=9;avoidPhase=0;remoteOwned=fromRemote;lastRemote=millis();
 if(m>=1&&m<=4)setPan(90);
 if(m==4)aiGuard=true;
 if(m==6){aiGuard=false;setPan(90);pose.x=pose.y=pose.heading=pose.kinX=pose.kinY=pose.kinHeading=0;pose.corrected=false;traffic.start(millis());}
}
char txBuffer[256];uint8_t txPos=0,txLen=0;
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
int8_t pendingReply=-1;
void reply(bool ok){pendingReply=ok?1:0;}
void command(char *text){
 IntegerCommand doc;
 if(!doc.begin(text))return;
 if(!doc[PSTR("N")].isInt16())return;
 const char *optional[]={PSTR("D1"),PSTR("D2"),PSTR("D3"),PSTR("D4")};
 for(uint8_t i=0;i<4;i++)if(doc.containsKey(optional[i])&&!doc[optional[i]].isInt16())return;
 int n=doc[PSTR("N")],d=doc[PSTR("D1")].orDefault(0);
 if(n==207){ // Browser E-stop or explicit reset. Always requires a separate Start.
  if(d==1){latchEStop(1);reply(true);return;}
  if(d==0){
   if(rangeCm>=0&&rangeCm<=FORWARD_ESTOP_CM){reply(false);return;}
   eStopLatched=false;eStopReason=0;standby();reply(true);return;
  }
  reply(false);return;
 }
 if(n==206){
  const char *keys[]={PSTR("ID"),PSTR("SC"),PSTR("BL"),PSTR("BH"),PSTR("RA"),PSTR("SA"),PSTR("CT"),PSTR("GS"),PSTR("CS"),PSTR("RS"),PSTR("DS")};
  for(uint8_t i=0;i<11;i++)if(!doc[keys[i]].isInt16())return;
  int id=doc[PSTR("ID")],sc=doc[PSTR("SC")],bl=doc[PSTR("BL")],bh=doc[PSTR("BH")],ra=doc[PSTR("RA")],sa=doc[PSTR("SA")],ct=doc[PSTR("CT")],gs=doc[PSTR("GS")],cs=doc[PSTR("CS")],rs=doc[PSTR("RS")],ds=doc[PSTR("DS")];
  if(mode!=0||id<1||id>30000||sc<5||sc>150||bl<0||bh>950||bl>=bh||ra<30||ra>120||sa<60||sa>240||sa>ra+120||ct<300||ct>5000||gs<0||gs>1||cs<50||cs>150||rs<0||rs>1||ds<0||ds>cs)return;
  traffic.slowSpeed=ds;traffic.cruiseSpeed=cs;traffic.stopOnRed=rs;traffic.stopCm=sc;ex3.blackLow=bl;ex3.blackHigh=bh;ex3.rightDegrees=ra;ex3.sweepDegrees=sa;traffic.crossMinimumMs=ct;pose.gyroSign=gs?1.0f:-1.0f;ex3.reset();settingsReady=true;configReply=id;return;
 }
 if(n==204){
  if(!doc[PSTR("S")].isUInt16()||!doc[PSTR("V")].isInt16()||!doc[PSTR("R")].isInt16()||!doc[PSTR("G")].isInt16()||!doc[PSTR("D")].isInt16()||!doc[PSTR("SD")].isInt16()||!doc[PSTR("B")].isInt16())return;
  int v=doc[PSTR("V")],r=doc[PSTR("R")],g=doc[PSTR("G")],d=doc[PSTR("D")],sd=doc[PSTR("SD")],b=doc[PSTR("B")];
  if(v<0||v>1||r<0||r>1||g<0||g>1||d< -1||d>10000||sd< -1||sd>10000||b< -1600||b>1600)return;
  if(doc.containsKey(PSTR("LC"))&&(!doc[PSTR("LC")].isInt16()||(long)doc[PSTR("LC")]<0||(long)doc[PSTR("LC")]>1))return;
  bool calibrated=doc.containsKey(PSTR("LC"))&&(long)doc[PSTR("LC")]==1;
  // N=203 (sent before N=204) reports ANY detected red region, while
  // N=204.R only reports a larger red region above the stop-action threshold.
  // During a red-light wait, never release on a green-looking background while
  // small-but-real red paper is still visible.
  bool anyRecentRed = indicator.red && uint32_t(millis()-indicator.redAt)<500;
  bool releaseRed = r || (v && anyRecentRed);
  traffic.colour(doc[PSTR("S")],v,releaseRed,g,d,millis(),calibrated);
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
void sensors(){
 lineL=analogRead(LINE_LEFT_PIN);lineM=analogRead(LINE_MIDDLE_PIN);lineR=analogRead(LINE_RIGHT_PIN);
 batteryMv=(int)(analogRead(BATTERY_PIN)*BATTERY_SCALE*1000);
 static uint32_t lastPing=0;
 if(millis()-lastPing>=100){
  lastPing=millis();digitalWrite(US_TRIG_PIN,LOW);delayMicroseconds(2);
  digitalWrite(US_TRIG_PIN,HIGH);delayMicroseconds(10);digitalWrite(US_TRIG_PIN,LOW);
  unsigned long duration=pulseIn(US_ECHO_PIN,HIGH,15000);rangeCm=duration?(int)(duration/58):-1;
 }
}
bool onBlack(int value){return BLACK_IS_HIGH?value>=LINE_THRESHOLD:value<=LINE_THRESHOLD;}
void floorTrack(int &left,int &right){
 bool l=onBlack(lineL),m=onBlack(lineM),r=onBlack(lineR);
 if(m){left=right=speedSetting;}
 else if(l&&!r){left=0;right=speedSetting;}
 else if(r&&!l){left=speedSetting;right=0;}
 else{left=right=0;reason=4;}
}
void avoid(int &left,int &right){
 uint32_t now=millis();
 if(avoidPhase==0){
  if(rangeCm<0){reason=8;return;}
  if(rangeCm>25){left=right=speedSetting;return;}
  setPan(150);avoidPhase=1;avoidAt=now;return;
 }
 if(avoidPhase==1&&now-avoidAt>=500){rangeLeft=rangeCm;setPan(30);avoidPhase=2;avoidAt=now;return;}
 if(avoidPhase==2&&now-avoidAt>=500){rangeRight=rangeCm;setPan(90);avoidPhase=3;avoidAt=now;return;}
 if(avoidPhase==3&&now-avoidAt>=400){
  if(rangeLeft<0&&rangeRight<0){avoidPhase=0;reason=8;return;}
  avoidPhase=(rangeLeft>=rangeRight)?4:5;avoidAt=now;
 }
 if(avoidPhase==4||avoidPhase==5){
  if(now-avoidAt<350)directionPWM(avoidPhase==4?3:4,speedSetting,left,right);
  else{avoidPhase=0;left=right=0;}
 }
}
void physicalInputs(){
 static bool raw=HIGH,stable=HIGH;static uint32_t keyAt=0;static uint8_t keyMode=0;
 bool k=digitalRead(KEY_PIN);if(k!=raw){raw=k;keyAt=millis();}
 if(millis()-keyAt>40&&stable!=raw){stable=raw;if(stable==LOW){keyMode=(keyMode+1)%4;if(!keyMode)standby();else chooseMode(keyMode,false);}}
 if(remote.decode(&irResult)){
  unsigned long code=irResult.value;remote.resume();
  if(code==0xFFFFFFFF){if(irOwned&&mode==5)lastRemote=millis();return;} // movement expires without a new full command
  if(code==16712445){standby();return;}
  if(code==16738455){chooseMode(1,false);return;}
  if(code==16750695){chooseMode(2,false);return;}
  if(code==16756815){chooseMode(3,false);return;}
  int d=0;if(code==16736925)d=1;if(code==16754775)d=2;if(code==16720605)d=3;if(code==16761405)d=4;
  if(d){irOwned=true;mode=5;manualDir=d;remoteOwned=true;lastRemote=millis();}
 }
}
void setup(){
 Serial.begin(9600);pinMode(MOTOR_STBY,OUTPUT);pinMode(MOTOR_LEFT_PWM,OUTPUT);pinMode(MOTOR_RIGHT_PWM,OUTPUT);
 pinMode(MOTOR_LEFT_DIR,OUTPUT);pinMode(MOTOR_RIGHT_DIR,OUTPUT);motorOutput(0,0);
 pinMode(KEY_PIN,INPUT_PULLUP);pinMode(US_TRIG_PIN,OUTPUT);pinMode(US_ECHO_PIN,INPUT);
 pan.attach(SERVO_PAN_PIN);setPan(90);FastLED.addLeds<NEOPIXEL,RGB_PIN>(led,1);FastLED.setBrightness(20);led[0]=CRGB::Black;FastLED.show();remote.enableIRIn();pose.begin();
}
void loop(){
 serialInput();pumpTelemetry();pose.step(leftPWM,rightPWM,millis());serialInput();physicalInputs();sensors();serialInput();
 if(mode==6){
  traffic.tick(millis());
  if(traffic.state==TrafficLogic::CROSS&&previousTraffic!=TrafficLogic::CROSS){crossing.arm(pose.yawDegrees);ex3.reset();}
  previousTraffic=traffic.state;
 }else previousTraffic=TrafficLogic::OFF;
 if(txPos>=txLen&&configReply>=0){
  int n=snprintf_P(txBuffer,sizeof(txBuffer),PSTR("{\"N\":222,\"ID\":%d,\"OK\":1,\"CS\":%d,\"RS\":%u,\"DS\":%d}\n"),configReply,traffic.cruiseSpeed,(unsigned)traffic.stopOnRed,traffic.slowSpeed);txPos=0;txLen=n;configReply=-1;
 }
 if(txPos>=txLen&&pendingReply>=0&&Serial.availableForWrite()>=24){Serial.println(pendingReply?F("{\"N\":211,\"OK\":1}"):F("{\"N\":211,\"OK\":0}"));pendingReply=-1;}
 pumpTelemetry();vision.evaluate(aiGuard,millis());updateStopLed();reason=mode?1:0;
 int left=0,right=0;
 if(remoteOwned&&mode!=6&&millis()-lastRemote>=(irOwned?300:REMOTE_TIMEOUT_MS)){standby();reason=2;}
 else if(aiGuard&&mode&&!vision.fresh(millis())){standby();reason=3;}
 else if(aiGuard&&vision.latched){reason=5;}
 else if(mode==6){
  if(traffic.state==TrafficLogic::CROSS){
   bool lineVisible=ex3.black(lineL)||ex3.black(lineM)||ex3.black(lineR);
   if(ex3.searching||(!lineVisible&&millis()-traffic.at>=traffic.crossMinimumMs)){
    ex3.step(lineL,lineM,lineR,traffic.crossingPWM(),millis(),pose.yawDegrees,pose.gyro,left,right,rangeCm);
   }else if(lineL>950&&lineM>950&&lineR>950){
    ex3.step(lineL,lineM,lineR,traffic.crossingPWM(),millis(),pose.yawDegrees,pose.gyro,left,right,rangeCm);
   }else{
    crossing.step(pose.yawDegrees,pose.gyro,ex3.black(lineL),ex3.black(lineM),ex3.black(lineR),millis(),millis()-traffic.at,traffic.crossMinimumMs,left,right,traffic.crossingPWM());
    traffic.crossReady=crossing.cleared(millis());
   }
  }else if(traffic.moving()){
   ex3.step(lineL,lineM,lineR,traffic.state==TrafficLogic::TRACK?traffic.cruisePWM():traffic.slowPWM(),millis(),pose.yawDegrees,pose.gyro,left,right,rangeCm);
  }else ex3.reset();
  if(ex3.failed){
   TrafficLogic::Fault why=ex3.failure==Ex3Tracking::LIFTED?TrafficLogic::LINE_LIFTED:(ex3.failure==Ex3Tracking::TIMED_OUT?TrafficLogic::SEARCH_TIMEOUT:(ex3.failure==Ex3Tracking::BLOCKED?TrafficLogic::SEARCH_BLOCKED:TrafficLogic::SEARCH_EXHAUSTED));
   traffic.fail(why,millis());left=right=0;
  }
  // Cap crossing corrections as well as tracking; 50..150 is PWM, not cm/s.
  int cap=traffic.state==TrafficLogic::CROSS?traffic.crossingPWM():(traffic.state==TrafficLogic::TRACK?traffic.cruisePWM():traffic.slowPWM());left=constrain(left,-cap,cap);right=constrain(right,-cap,cap);
  reason=20+(uint8_t)traffic.state;
 }
 else if(mode==1||mode==4)floorTrack(left,right);
 else if(mode==2)avoid(left,right);
 else if(mode==3){if(rangeCm>=15&&rangeCm<=50)left=right=speedSetting;else reason=8;}
 else if(mode==5)directionPWM(manualDir,speedSetting,left,right);
 // A genuinely measured nearby forward obstacle is an automatic stop.
 // Only forward travel: turning-in-place has different ultrasonic geometry.
 if(!eStopLatched&&mode==6&&traffic.moving()&&left>0&&right>0 &&
    rangeCm>=0&&rangeCm<=FORWARD_ESTOP_CM){latchEStop(2);left=right=0;}
 motorOutput(left,right);
 if(txPos>=txLen&&millis()-lastTrafficTx>=400){
  lastTrafficTx=millis();
  int n=snprintf_P(txBuffer,sizeof(txBuffer),PSTR("{\"N\":221,\"M\":%u,\"TS\":%u,\"ID\":%u,\"X\":%d,\"Y\":%d,\"H\":%d,\"KX\":%d,\"KY\":%d,\"KH\":%d,\"L\":%d,\"C\":%d,\"R\":%d,\"GY\":%u,\"YA\":%d,\"UV\":32,\"FR\":%d,\"UT\":%lu,\"FC\":%u,\"FS\":%u,\"SE\":%u,\"SP\":%u,\"GC\":%u,\"PL\":%d,\"PR\":%d,\"ES\":%u,\"ER\":%u}\n"),mode,(unsigned)traffic.state,traffic.request,(int)pose.x,(int)pose.y,(int)(pose.heading*180/PI),(int)pose.kinX,(int)pose.kinY,(int)(pose.kinHeading*180/PI),lineL,lineM,lineR,(unsigned)pose.gyro,(int)pose.yawDegrees,freeRam(),(unsigned long)millis(),(unsigned)traffic.fault,(unsigned)traffic.faultState,(unsigned)ex3.searching,(unsigned)ex3.phase,(unsigned)traffic.greens,leftPWM,rightPWM,(unsigned)eStopLatched,(unsigned)eStopReason);
  if(n>0&&n<(int)sizeof(txBuffer)){txPos=0;txLen=n;}
 }
 if(millis()-lastTelemetry>=1000){lastTelemetry=millis();telemetry();}
}
