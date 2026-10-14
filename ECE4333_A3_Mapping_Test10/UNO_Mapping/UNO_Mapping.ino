#include <Arduino.h>
// ECE 4333 Assignment 3 - TEST10 MAPPING BUILD (UV=40)
// Line following + data collection only. Traffic light / STOP sign / AI logic,
// RGB LED, ultrasonic avoidance and the old floor-tracking modes are removed.
// For traffic tests, upload the Test9 UNO + ESP32 again (different baud and UV).
#include <avr/pgmspace.h>
#include <Servo.h>
#include "MapConfig.h"
#include "Hardware.h"
#include "IntegerCommand.h"
#include "PoseEstimate.h"
#include "LineFollower.h"

PoseEstimate pose;LineFollower follower;Servo pan;
// mode: 0 standby, 5 manual (web arrows), 6 autonomous mapping run
uint8_t mode=0;
bool settingsReady=false,gyroNavigation=true;int configReply=-1;int8_t pendingReply=-1;
int blackLow=600,blackHigh=1023,cruise=60,echoRS=0,echoDS=40;
uint8_t zeroRevision=0;uint16_t runNumber=0;
bool eStopLatched=false;uint8_t eStopReason=0,nearCount=0; // reason 1=web, 2=ultrasonic
int lineL=0,lineM=0,lineR=0,rangeCm=-1,batteryMv=0,leftPWM=0,rightPWM=0;
int manualDir=9,speedSetting=90,panAngle=90;uint32_t lastRemote=0,motorsOffSince=0;

// ---------- N230 sample ring (sampled on time, sent when the UART is free) ----------
struct Sample{uint32_t t;int32_t yaw10,distMm;uint16_t q;int16_t l,c,r,pl,pr,g10,u,x,y;uint8_t st,k;};
static const uint8_t RING=10;Sample ring[RING];uint8_t ringHead=0,ringCount=0;
uint16_t sampleSeq=0,droppedSamples=0;uint8_t eventMask=0;uint32_t lastSample=0;

bool black(int v){return v>=blackLow&&v<=blackHigh;}
// Hysteresis: become black only clearly inside the black range, become white only clearly outside.
bool blackL=false,blackM=false,blackR=false;
bool hyst(bool was,int v){
 if(blackLow>0){int t=blackLow+(was?-LINE_HYST:LINE_HYST);return v>=t&&v<=blackHigh;} // black = high readings
 int t=blackHigh+(was?LINE_HYST:-LINE_HYST);return v<=t;                              // black = low readings
}
int8_t gyroSignLearned=0;uint8_t gyroSignFlips=0; // auto-corrected gyro direction (kept until power-off)
void setPan(int a){panAngle=constrain(a,0,180);pan.write(panAngle);}
void motorOutput(int left,int right){
 if(eStopLatched)left=right=0;
 left=constrain(left,-DRIVE_LIMIT,DRIVE_LIMIT);right=constrain(right,-DRIVE_LIMIT,DRIVE_LIMIT);
 digitalWrite(MOTOR_STBY,(left||right)?HIGH:LOW);
 digitalWrite(MOTOR_LEFT_DIR,left>=0?HIGH:LOW);digitalWrite(MOTOR_RIGHT_DIR,right>=0?HIGH:LOW);
 analogWrite(MOTOR_LEFT_PWM,abs(left));analogWrite(MOTOR_RIGHT_PWM,abs(right));
 if(!left&&!right&&(leftPWM||rightPWM))motorsOffSince=millis();
 leftPWM=left;rightPWM=right;
}
void standby(){follower.stop();mode=0;manualDir=9;motorOutput(0,0);}
void latchEStop(uint8_t why){standby();eStopLatched=true;eStopReason=why;eventMask|=128;}
void startRun(){
 runNumber++;mode=6;setPan(90);pose.zero(millis());
 follower.cruise=cruise;follower.pivot=PIVOT_PWM;follower.crossHold=CROSS_HOLD_MS;follower.maxTurn=MAX_CORNER_DEG;
 follower.wideSweep=WIDE_SEARCH_DEG;follower.degPerMs=NO_GYRO_DEG_PER_MS;follower.blockCm=CREEP_BLOCK_CM;
 follower.start(pose.yawDegrees,millis());
 sampleSeq=0;droppedSamples=0;ringCount=0;lastSample=0;
}
void directionPWM(int dir,int speed,int &left,int &right){
 left=right=0;
 switch(dir){case 1:left=right=speed;break;case 2:left=right=-speed;break;
 case 3:left=-speed;right=speed;break;case 4:left=speed;right=-speed;break;
 case 5:left=speed/2;right=speed;break;case 6:left=-speed/2;right=-speed;break;
 case 7:left=speed;right=speed/2;break;case 8:left=-speed;right=-speed/2;break;}
}

// ---------- serial protocol (same N numbers as Test9 so the web page keeps working) ----------
void reply(bool ok){pendingReply=ok?1:0;}
void command(char *text){
 IntegerCommand doc;
 if(!doc.begin(text)||!doc[PSTR("N")].isInt16())return;
 int n=doc[PSTR("N")],d=doc[PSTR("D1")].orDefault(0);
 if((n>=200&&n<=205)||n==214)return; // camera/traffic messages: ignored in the mapping build
 if(n==213){ // gyro navigation 0/1, 1 also zeroes; 2 = zero only
  if(mode!=0||d<0||d>2){reply(false);return;}
  if(d<2)gyroNavigation=d;
  if(d){if(d==2)pose.startRecal(millis());else{pose.zero(millis());zeroRevision++;}} // 2: re-measure bias (1 s) then zero; ZR increments when done
  reply(true);return;
 }
 if(n==208){reply(false);return;} // teammate PD not in the mapping build
 if(n==207){
  if(d==1){latchEStop(1);reply(true);return;}
  if(d==0){if(rangeCm>=0&&rangeCm<=FORWARD_ESTOP_CM){reply(false);return;}eStopLatched=false;eStopReason=0;nearCount=0;standby();reply(true);return;}
  reply(false);return;
 }
 if(n==206){
  const char *keys[]={PSTR("ID"),PSTR("SC"),PSTR("BL"),PSTR("BH"),PSTR("RA"),PSTR("SA"),PSTR("CT"),PSTR("GS"),PSTR("CS"),PSTR("RS"),PSTR("DS")};
  for(uint8_t i=0;i<11;i++)if(!doc[keys[i]].isInt16())return;
  int id=doc[PSTR("ID")],bl=doc[PSTR("BL")],bh=doc[PSTR("BH")],gs=doc[PSTR("GS")],cs=doc[PSTR("CS")],rs=doc[PSTR("RS")],ds=doc[PSTR("DS")];
  if(mode!=0||id<1||id>30000||bl<0||bh>1023||bl>=bh||gs<0||gs>1||cs<50||cs>150||rs<0||rs>1||ds<0||ds>cs)return;
  // Used here: BL/BH line threshold, CS cruise PWM, GS gyro sign. Others are echoed only.
  blackLow=bl;blackHigh=bh;cruise=cs;pose.gyroSign=gyroSignLearned?(float)gyroSignLearned:(gs?1.0f:-1.0f);echoRS=rs;echoDS=ds;settingsReady=true;configReply=id;return;
 }
 if(n==100||n==110){standby();reply(true);return;}
 if(n==101){
  if(d==0){standby();reply(true);return;}
  // A mapping run without a working gyro cannot reject the old track at corners (Test10 run1): refuse.
  if(d==6&&mode==0&&settingsReady&&!eStopLatched&&!pose.calibrating&&(pose.gyro||!gyroNavigation)){startRun();reply(true);return;}
  reply(false);return;
 }
 if(n==102){
  if(d<1||d>9||mode==6){reply(false);return;}
  if(d==9){standby();return;}
  if(doc.containsKey(PSTR("D2"))){int sp=doc[PSTR("D2")];if(sp<0||sp>DRIVE_LIMIT)return;speedSetting=sp;}
  mode=5;manualDir=d;lastRemote=millis();return;
 }
 if(n==103){lastRemote=millis();return;}
 if(n==104){if(d<0||d>DRIVE_LIMIT)return;speedSetting=d;reply(true);return;}
 if(n==5){int a=doc[PSTR("D2")].orDefault(90);if(d!=1||a<0||a>180){reply(false);return;}standby();setPan(a);reply(true);return;}
 if(n==21||n==22||n==210)return; // status is sent periodically anyway
 reply(false);
}
char input[128];uint8_t inputPos=0;bool receiving=false;
void serialInput(){
 uint8_t budget=64;
 while(budget--&&Serial.available()){
  char c=Serial.read();if(c=='{'){inputPos=0;receiving=true;}
  if(!receiving)continue;
  if(inputPos>=sizeof(input)-1){receiving=false;continue;}
  input[inputPos++]=c;
  if(c=='}'){input[inputPos]=0;receiving=false;command(input);}
 }
}

// ---------- sensors ----------
void sensors(){
 lineL=analogRead(LINE_LEFT_PIN);lineM=analogRead(LINE_MIDDLE_PIN);lineR=analogRead(LINE_RIGHT_PIN);
 static uint32_t lastPing=0,lastBattery=0;
 if(millis()-lastBattery>=500){lastBattery=millis();batteryMv=(int)(analogRead(BATTERY_PIN)*BATTERY_SCALE*1000);}
 if(millis()-lastPing>=100){
  lastPing=millis();digitalWrite(US_TRIG_PIN,LOW);delayMicroseconds(2);
  digitalWrite(US_TRIG_PIN,HIGH);delayMicroseconds(10);digitalWrite(US_TRIG_PIN,LOW);
  unsigned long duration=pulseIn(US_ECHO_PIN,HIGH,12000);rangeCm=duration?(int)(duration/58):-1;
  nearCount=(rangeCm>=0&&rangeCm<=FORWARD_ESTOP_CM)?(nearCount<3?nearCount+1:3):0;
 }
 blackL=hyst(blackL,lineL);blackM=hyst(blackM,lineM);blackR=hyst(blackR,lineR);
 bool l=blackL,m=blackM,r=blackR;
 eventMask|=(l?1:0)|(m?2:0)|(r?4:0)|((l&&m&&r)?8:0);
}
void physicalKey(){
 static bool raw=HIGH,stable=HIGH;static uint32_t keyAt=0;
 bool k=digitalRead(KEY_PIN);if(k!=raw){raw=k;keyAt=millis();}
 if(millis()-keyAt>40&&stable!=raw){stable=raw;if(stable==LOW){if(mode)standby();else if(settingsReady&&!eStopLatched&&(pose.gyro||!gyroNavigation))startRun();}}
}

// ---------- telemetry ----------
char tx[352];uint16_t txPos=0,txLen=0;
int freeRam(){extern char __heap_start;extern char *__brkval;char top;return &top-(__brkval?__brkval:&__heap_start);}
uint8_t trafficState(){ // TS shown by the web page: 0 OFF, 2 TRACK, 7 CROSS, 8 FAULT
 if(mode!=6)return 0;
 switch(follower.state){case LineFollower::CROSS:return 7;case LineFollower::FAIL:return 8;default:return 2;}
}
void takeSample(uint32_t now){
 if(ringCount==RING){droppedSamples++;ringHead=(ringHead+1)%RING;ringCount--;} // overwrite oldest
 Sample &s=ring[(ringHead+ringCount)%RING];ringCount++;
 s.q=sampleSeq++;s.t=now;s.l=lineL;s.c=lineM;s.r=lineR;s.pl=leftPWM;s.pr=rightPWM;
 s.g10=(int16_t)(pose.yawRate*10);s.yaw10=(int32_t)(pose.yawDegrees*10);s.distMm=(int32_t)(pose.dist*10);
 s.u=rangeCm;s.x=(int16_t)pose.x;s.y=(int16_t)pose.y;
 s.st=(uint8_t)((mode<<4)|(pose.gyro?8:0)|(mode==6?follower.state:0)); // bit3 = gyro working
 s.k=eventMask|follower.events;
 eventMask=0;follower.events=0;
}
bool startTx(int n){if(n>0&&n<(int)sizeof(tx)){txPos=0;txLen=n;return true;}return false;}
void scheduleTx(uint32_t now){
 static uint32_t lastStatus=0,lastSensors=0;
 if(txPos<txLen)return;
 if(configReply>=0){
  startTx(snprintf_P(tx,sizeof(tx),PSTR("{\"N\":222,\"ID\":%d,\"OK\":1,\"CS\":%d,\"RS\":%d,\"DS\":%d}\n"),configReply,cruise,echoRS,echoDS));configReply=-1;return;
 }
 if(pendingReply>=0){startTx(snprintf_P(tx,sizeof(tx),PSTR("{\"N\":211,\"OK\":%d}\n"),(int)pendingReply));pendingReply=-1;return;}
 bool statusDue=uint32_t(now-lastStatus)>=STATUS_MS;
 if(ringCount&&!(statusDue&&uint32_t(now-lastStatus)>=STATUS_MS+250)){
  Sample &s=ring[ringHead];ringHead=(ringHead+1)%RING;ringCount--;
  // d=[q,t,L,C,R,PL,PR,gyro_rate*10,yaw*10,dist_mm,x_cm,y_cm,ultrasonic,state(mode*16+8*gyroOK+follow),mask]
  startTx(snprintf_P(tx,sizeof(tx),PSTR("{\"N\":230,\"d\":[%u,%lu,%d,%d,%d,%d,%d,%d,%ld,%ld,%d,%d,%d,%u,%u]}\n"),
   s.q,(unsigned long)s.t,s.l,s.c,s.r,s.pl,s.pr,s.g10,(long)s.yaw10,(long)s.distMm,s.x,s.y,s.u,(unsigned)s.st,(unsigned)s.k));
  return;
 }
 if(statusDue){
  lastStatus=now;uint8_t ts=trafficState();
  bool searching=mode==6&&(follower.state==LineFollower::TURN||follower.state==LineFollower::SEARCH);
  startTx(snprintf_P(tx,sizeof(tx),PSTR("{\"N\":221,\"M\":%u,\"TS\":%u,\"ID\":0,\"X\":%d,\"Y\":%d,\"H\":%d,\"KX\":%d,\"KY\":%d,\"KH\":%d,\"L\":%d,\"C\":%d,\"R\":%d,\"GY\":%u,\"YA\":%d,\"UV\":%d,\"FR\":%d,\"UT\":%lu,\"FC\":%u,\"FS\":%u,\"SE\":%u,\"SP\":%u,\"GC\":0,\"PL\":%d,\"PR\":%d,\"ES\":%u,\"ER\":%u,\"PD\":0,\"RN\":%u,\"GN\":%u,\"ZR\":%u,\"MQ\":%u,\"MD\":%u,\"IM\":%u,\"IW\":%u,\"IQ\":%u,\"IB\":%u,\"CB\":%u,\"GL\":%d,\"GF\":%u}\n"),
   mode,ts,(int)pose.x,(int)pose.y,(int)(pose.heading*180/PI),(int)pose.kinX,(int)pose.kinY,(int)(pose.kinHeading*180/PI),lineL,lineM,lineR,(unsigned)pose.gyro,(int)pose.yawDegrees,FIRMWARE_UV,freeRam(),(unsigned long)now,
   (unsigned)(mode==6?follower.fault:0),(unsigned)(mode==6?follower.faultState:0),(unsigned)searching,(unsigned)(mode==6?follower.state:0),leftPWM,rightPWM,(unsigned)eStopLatched,(unsigned)eStopReason,runNumber,(unsigned)gyroNavigation,(unsigned)zeroRevision,sampleSeq,droppedSamples,(unsigned)pose.imuType,(unsigned)pose.whoMpu,(unsigned)pose.whoQmi,(unsigned)pose.busRecoveries,(unsigned)pose.calibrating,(int)gyroSignLearned,(unsigned)gyroSignFlips));
  return;
 }
 if(uint32_t(now-lastSensors)>=SENSOR_MS){
  lastSensors=now;
  startTx(snprintf_P(tx,sizeof(tx),PSTR("{\"N\":210,\"M\":%u,\"B\":0,\"G\":0,\"T\":0,\"U\":%d,\"L\":%d,\"C\":%d,\"R\":%d,\"V\":%d,\"S\":%d,\"Q\":0,\"A\":%d,\"D\":%d,\"AI\":0,\"VF\":0,\"P\":0,\"VD\":-1,\"LED\":0,\"UT\":%lu,\"GR10\":%d}\n"),
   mode,rangeCm,lineL,lineM,lineR,batteryMv,panAngle,leftPWM,rightPWM,(unsigned long)now,(int)(pose.yawRate*10)));
 }
}
void pumpTx(){int a=Serial.availableForWrite();while(txPos<txLen&&a-->0)Serial.write(tx[txPos++]);}

void setup(){
 Serial.begin(LINK_BAUD);
 pinMode(MOTOR_STBY,OUTPUT);pinMode(MOTOR_LEFT_PWM,OUTPUT);pinMode(MOTOR_RIGHT_PWM,OUTPUT);
 pinMode(MOTOR_LEFT_DIR,OUTPUT);pinMode(MOTOR_RIGHT_DIR,OUTPUT);motorOutput(0,0);
 pinMode(KEY_PIN,INPUT_PULLUP);pinMode(US_TRIG_PIN,OUTPUT);pinMode(US_ECHO_PIN,INPUT);
 pan.attach(SERVO_PAN_PIN);setPan(90);
 pose.begin(); // keep the car still for ~1 s after power-on (gyro bias)
 motorsOffSince=millis();
}
void loop(){
 uint32_t now=millis();
 serialInput();pumpTx();
 bool parked=mode==0&&!leftPWM&&!rightPWM&&uint32_t(now-motorsOffSince)>1000;
 pose.step(leftPWM,rightPWM,now,parked);
 sensors();physicalKey();serialInput();
 int left=0,right=0;now=millis();
 if(mode==5){
  if(uint32_t(now-lastRemote)>=REMOTE_TIMEOUT_MS)standby();else directionPWM(manualDir,speedSetting,left,right);
 }else if(mode==6){
  follower.step(blackL,blackM,blackR,now,pose.yawDegrees,gyroNavigation&&pose.gyro,rangeCm,left,right);
  if(follower.requestSignFlip){ // commanded a left pivot but the gyro said right: the gyro sign (GS) is reversed
   follower.requestSignFlip=false;pose.gyroSign=-pose.gyroSign;pose.yawDegrees=-pose.yawDegrees;pose.heading=-pose.heading;
   follower.negateAngles();gyroSignLearned=pose.gyroSign>0?1:-1;gyroSignFlips++;
  }
  // forward obstacle: three consecutive valid readings (~0.3 s) needed, a single noisy echo is ignored
  if(left>0&&right>0&&nearCount>=3){latchEStop(2);left=right=0;}
 }
 motorOutput(left,right);
 if(pose.calDone){pose.calDone=false;zeroRevision++;}
 uint32_t period=mode?SAMPLE_MS:IDLE_SAMPLE_MS;
 if(uint32_t(now-lastSample)>=period){lastSample=now;takeSample(now);}
 scheduleTx(now);pumpTx();
}
