#include <Arduino.h>
// ECE 4333 Assignment 4 - MERGED RACE BUILD (UV=44): Test14 base; E-stop, distance calibration, no-gyro mode and laps limit removed;
// odometer scale learning, map heading steer and B-C-only overtaking restored.
// Figure-8 race: start in front of the traffic light (middle of O-D) on green, line following + mapping,
// two STOP signs (red blob -> slow, big red blob / stop line -> stop, 3 s timer from the AI request,
// servo scans the intersection left/right), Bug-algorithm obstacle avoidance at race speed (45 deg out,
// side scan with the panned ultrasonic, 45 deg back) and dynamic overtaking on the outside only
// (tail the competitor on the STOP-sign segments, pass elsewhere). Race speed can be changed live (N217).
// RaceRules.h = traffic light + STOP signs, Avoid.h = avoidance / overtaking, LineFollower.h = map.
// Test11: mode 7 = vision target following (TargetFollow.h: camera outer loop + gyro PD inner
// loop; compiled out in Test13 unless FOLLOW_ENABLED), predicted (wheel-model only) pose in every sample, optional wait-for-green start and
// lap limit. Serial text is built by LinkTx.h (no vfprintf in flash).
#include <avr/pgmspace.h>
#include "MapConfig.h"
#if PAN_SERVO
#include <Servo.h>
#endif
#include "Hardware.h"
#include "IntegerCommand.h"
#include "PoseEstimate.h"
#include "LineFollower.h"
#include "RaceRules.h"
#include "LinkTx.h"
#if FOLLOW_ENABLED
#include "TargetFollow.h"
#endif
#include "Avoid.h"

PoseEstimate pose;LineFollower follower;RaceRules traffic;Avoid ovt;
#if PAN_SERVO
Servo pan;
#endif
#if FOLLOW_ENABLED
TargetFollow follow;
#endif
// mode: 0 standby, 5 manual (web arrows), 6 autonomous mapping run, 7 vision target following
uint8_t mode=0;
bool settingsReady=false;int configReply=-1;int8_t pendingReply=-1;
int blackLow=600,blackHigh=1023,cruise=60,echoRS=0,echoDS=40;
uint8_t zeroRevision=0;uint16_t runNumber=0;
// Test14 race parameters (N101 D4..D15): slowPct = speed while a STOP sign is ahead, stopLine = map fallback stop
// before O (0 off), scanAmp = intersection scan angle; state-machine code in every sample
uint8_t slowPct=50,stopLine=20,scanAmp=60;uint32_t panAt=0;
uint8_t pingSeq=0,pingSeen=0; // a new ultrasonic reading for the state machines
int lineL=0,lineM=0,lineR=0,rangeCm=-1,leftPWM=0,rightPWM=0;
int manualDir=9,speedSetting=90,panAngle=90;uint32_t lastRemote=0,motorsOffSince=0;

// ---------- N230 sample ring (sampled on time, sent when the UART is free) ----------
struct Sample{uint32_t t;int32_t yaw10,distMm;uint16_t q;int16_t l,c,r,pl,pr,g10,u,x,y,kh,kx,ky;uint8_t st,k,sm;};
static const uint8_t RING=5;Sample ring[RING];uint8_t ringHead=0,ringCount=0;
uint16_t sampleSeq=0,droppedSamples=0;uint8_t eventMask=0;uint32_t lastSample=0;

bool black(int v){return v>=blackLow&&v<=blackHigh;}
// Hysteresis: become black only clearly inside the black range, become white only clearly outside.
bool blackL=false,blackM=false,blackR=false;
bool hyst(bool was,int v){
 if(blackLow>0){int t=blackLow+(was?-LINE_HYST:LINE_HYST);return v>=t&&v<=blackHigh;} // black = high readings
 int t=blackHigh+(was?LINE_HYST:-LINE_HYST);return v<=t;                              // black = low readings
}
int8_t gyroSignLearned=0;uint8_t gyroSignFlips=0; // auto-corrected gyro direction (kept until power-off)
#if PAN_SERVO
void setPan(int a){a=constrain(a,0,180);if(a!=panAngle)panAt=millis();panAngle=a;pan.write(a);}
#else
#define setPan(a) ((void)0)
#endif
int usRange(){return(panAngle==90&&uint32_t(millis()-panAt)>PAN_SETTLE_MS)?rangeCm:-1;} // straight ahead only
int usAny(){return uint32_t(millis()-panAt)>PAN_SETTLE_MS?rangeCm:-2;}                            // current servo direction, -2 = moving
uint8_t lookAngle(int8_t dir,uint8_t amp){return PAN_LEFT_LOW?90-dir*amp:90+dir*amp;}             // dir +1 left, -1 right
void motorOutput(int left,int right){
 left=constrain(left,-DRIVE_LIMIT,DRIVE_LIMIT);right=constrain(right,-DRIVE_LIMIT,DRIVE_LIMIT);
 digitalWrite(MOTOR_STBY,(left||right)?HIGH:LOW);
 digitalWrite(MOTOR_LEFT_DIR,left>=0?HIGH:LOW);digitalWrite(MOTOR_RIGHT_DIR,right>=0?HIGH:LOW);
 analogWrite(MOTOR_LEFT_PWM,abs(left));analogWrite(MOTOR_RIGHT_PWM,abs(right));
 if(!left&&!right&&(leftPWM||rightPWM))motorsOffSince=millis();
 leftPWM=left;rightPWM=right;
}
void standby(){follower.stop();
#if FOLLOW_ENABLED
 follow.stop();
#endif
 traffic.reset();mode=0;manualDir=9;motorOutput(0,0);}
void startRun(bool waitGreen){
 runNumber++;mode=6;setPan(90);pose.zero(millis());
 ovt.reset();ovt.ok=ovt.fails=0;ovt.speed=cruise;traffic.stops=traffic.stopsAI=0;
 follower.cruise=cruise; // pivot, crossHold, maxTurn, wideSweep, blockCm, route: LineFollower defaults = MapConfig.h values
 follower.start(pose.yawDegrees,millis());
 traffic.reset();traffic.enabled=echoRS==1;
 if(waitGreen)traffic.enter(RaceRules::WAIT_GREEN,millis()); // the robot starts when the light turns green
 sampleSeq=0;droppedSamples=0;ringCount=0;lastSample=0;
}
#if FOLLOW_ENABLED
void startFollow(int w,int fov,int kp,int kd){
 runNumber++;mode=7;setPan(90);pose.zero(millis());
 follow.stopW=w;follow.fov=fov;follow.speed=cruise;follow.pd.kp10=kp;follow.pd.kd100=kd;
 follow.start(pose.yawDegrees,millis());traffic.reset();
 sampleSeq=0;droppedSamples=0;ringCount=0;lastSample=0;
}
#endif
#if MANUAL_DRIVE
void directionPWM(int dir,int speed,int &left,int &right){
 left=right=0;
 switch(dir){case 1:left=right=speed;break;case 2:left=right=-speed;break;
 case 3:left=-speed;right=speed;break;case 4:left=speed;right=-speed;break;
 case 5:left=speed/2;right=speed;break;case 6:left=-speed/2;right=-speed;break;
 case 7:left=speed;right=speed/2;break;case 8:left=-speed;right=-speed/2;break;}
}
#endif

// ---------- serial protocol (same N numbers as Test9 so the web page keeps working) ----------
void reply(bool ok){pendingReply=ok?1:0;}
void command(char *text){
 IntegerCommand doc;
 if(!doc.begin(text)||!doc[PSTR("N")].isInt16())return;
 int n=doc[PSTR("N")],d=doc[PSTR("D1")].orDefault(0);
 if(n==204){ // colour frame from the ESP32: R = red region >= RP pixels, G = green seen
  // R = level for the current zone (1 light red / STOP blob >= RPS, 2 STOP blob >= RP); frames from another zone are ignored
  if(mode==6&&doc[PSTR("S")].isUInt16()&&(long)doc[PSTR("Z")]==traffic.zone)traffic.colour(doc[PSTR("S")],(long)doc[PSTR("V")]==1,(uint8_t)constrain((long)doc[PSTR("R")],0,2),(long)doc[PSTR("G")]==1,
   follower.state==LineFollower::FOLLOW||follower.state==LineFollower::CROSS||ovt.state==Avoid::TAIL,millis());
  return;
 }
 if(n==205){ // STOP-sign AI result for our request ID
  if(mode==6&&doc[PSTR("ID")].isUInt16())traffic.ai(doc[PSTR("ID")],(long)doc[PSTR("V")]==1,(long)doc[PSTR("P")]>0,millis());
  return;
 }
 if(n==219){if(mode==6&&doc[PSTR("ID")].isUInt16())traffic.captured(doc[PSTR("ID")],millis());return;} // the ESP32 took the AI frame: the servo may move
 if(n==217){ // live race speed from the web page (allowed while driving)
  if(d<40||d>DRIVE_LIMIT){reply(false);return;}
  cruise=d;follower.cruise=d;ovt.speed=d;reply(true);return;
 }
 if(n==215){ // colour target frame: S seq, V valid, T seen, X centre x and W width (per mille of the image), A frame age ms
#if FOLLOW_ENABLED
  static const char K215[] PROGMEM="S\0V\0T\0X\0W\0A";
  if(mode==7&&doc[K215].isUInt16()){long a[6];for(uint8_t i=0;i<6;i++)a[i]=doc[K215+2*i];follow.measure((uint16_t)a[0],a[1]==1,a[2]==1,(int)a[3],(int)a[4],(int)a[5]);}
#endif
  return;
 }
 if((n>=200&&n<=203)||n==214)return; // other camera messages are not needed
 if(n==213){ // gyro navigation 0/1, 1 also zeroes; 2 = zero only
  if(mode!=0||d<1||d>2){reply(false);return;}
  if(d){if(d==2)pose.startRecal(millis());else{pose.zero(millis());zeroRevision++;}} // 2: re-measure bias (1 s) then zero; ZR increments when done
  reply(true);return;
 }
 if(n==208){reply(false);return;} // teammate PD not in the mapping build
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
  // D2 = 1: wait for a green light before driving off. The run ends at STOP / fault / ordered stop; laps are counted, not limited
  bool can=mode==0&&settingsReady&&!pose.calibrating;
  if(d==6&&can&&pose.gyro){
   // D4 start offset after O (cm), D5 speed % while a STOP sign is ahead, D6 map stop line before O (cm, 0 off),
   // D7 avoidance/overtaking 0/1, D8 detect cm, D9 lateral offset cm, D10 side-echo margin cm, D11 car length (drive on after the echo is gone) cm,
   // D12 intersection scan angle deg, D13 intersection clear cm, D14 tail gap cm, D15 wheel axle to line sensors cm
   static const char K101[] PROGMEM="D4\0D5\0D6\0D7\0D8\0D9\0D10\0D11\0D12\0D13\0D14\0D15";
   static const uint8_t DEF[12] PROGMEM={83,50,20,1,35,20,6,15,60,45,18,8},LO[12] PROGMEM={20,20,0,0,15,10,2,0,30,15,8,0},HI[12] PROGMEM={150,100,60,1,80,40,30,40,85,100,40,25};
   uint8_t v[12];const char *k=K101;
   for(uint8_t i=0;i<12;i++){long x=doc[k].orDefault(pgm_read_byte(&DEF[i]));if(x<pgm_read_byte(&LO[i])||x>pgm_read_byte(&HI[i])){reply(false);return;}v[i]=x;k+=strlen_P(k)+1;}
   follower.startOff=v[0];slowPct=v[1];stopLine=v[2];scanAmp=v[8];traffic.clearCm=v[9];
   ovt.enabled=v[3];ovt.trigCm=v[4];ovt.latCm=v[5];ovt.margin=v[6];ovt.tailCm=v[7];ovt.gapCm=v[10];ovt.joinAdv=v[11];
   startRun(doc[PSTR("D2")].orDefault(0)==1);reply(true);return;
  }
#if FOLLOW_ENABLED
  // target following: D2 stop width (per mille), D3 camera FOV deg (negative = mirrored), D4 Kp x10, D5 Kd x100. Needs the gyro.
  if(d==7&&can&&pose.gyro){
   int w=doc[PSTR("D2")].orDefault(FOLLOW_STOP_W_PM),f=doc[PSTR("D3")].orDefault(FOLLOW_FOV_DEG),kp=doc[PSTR("D4")].orDefault(FOLLOW_KP10),kd=doc[PSTR("D5")].orDefault(FOLLOW_KD100);
   int af=f<0?-f:f;
   if(w>=40&&w<=900&&af>=20&&af<=140&&kp>=1&&kp<=80&&kd>=0&&kd<=50){startFollow(w,f,kp,kd);reply(true);return;}
  }
#endif
  reply(false);return;
 }
 #if MANUAL_DRIVE
 if(n==102){
  if(d<1||d>9||mode>=6){reply(false);return;}
  if(d==9){standby();return;}
  if(doc.containsKey(PSTR("D2"))){int sp=doc[PSTR("D2")];if(sp<0||sp>DRIVE_LIMIT)return;speedSetting=sp;}
  mode=5;manualDir=d;lastRemote=millis();return;
 }
 if(n==103){lastRemote=millis();return;}
 if(n==104){if(d<0||d>DRIVE_LIMIT)return;speedSetting=d;reply(true);return;}
 #endif
 if(n==5){int a=doc[PSTR("D2")].orDefault(90);if(d!=1||a<0||a>180){reply(false);return;}standby();setPan(a);reply(true);return;}
 if(n==21||n==22||n==210)return; // status is sent periodically anyway
 reply(false);
}
char input[176];uint8_t inputPos=0;bool receiving=false;
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
 static uint32_t lastPing=0;
 if(millis()-lastPing>=100){
  lastPing=millis();digitalWrite(US_TRIG_PIN,LOW);delayMicroseconds(2);
  digitalWrite(US_TRIG_PIN,HIGH);delayMicroseconds(10);digitalWrite(US_TRIG_PIN,LOW);
  unsigned long duration=pulseIn(US_ECHO_PIN,HIGH,12000);rangeCm=duration?(int)(duration/58):-1;pingSeq++;
 }
 blackL=hyst(blackL,lineL);blackM=hyst(blackM,lineM);blackR=hyst(blackR,lineR);
 bool l=blackL,m=blackM,r=blackR;
 eventMask|=(l?1:0)|(m?2:0)|(r?4:0)|((l&&m&&r)?8:0);
}

// ---------- telemetry ----------
char tx[300];uint16_t txPos=0,txLen=0; // longest message (N221 with every field at its widest) is checked by tests/uno_protocol_test.cpp
uint8_t trafficState(){ // TS shown by the web page: 0 OFF, 2 TRACK, 3 STOP ahead (slow), 4 STOP + AI, 5 wait green, 6 STOP hold/scan, 7 CROSS, 8 FAULT, 9-12 target following
#if FOLLOW_ENABLED
 if(mode==7)return 8+follow.state; // 9 TRACK, 10 ARRIVED, 11 target lost (stopped), 12 searching
#endif
 if(mode!=6)return 0;
 if(traffic.st!=RaceRules::GO&&traffic.st!=RaceRules::DONE)return traffic.ts();
 switch(follower.state){case LineFollower::CROSS:return 7;case LineFollower::FAIL:return 8;default:return 2;}
}
uint8_t ovtScore(){return (ovt.ok>9?9:ovt.ok)*10+(ovt.fails>9?9:ovt.fails);} // N221 OV: successful detours x10 + failed
// state-machine code logged with every sample (page: names and state-transition log)
// 0 standby, 1 track, 2 crossing, 3 corner turn, 4 line search, 5 STOP ahead: slow, 6 STOP sign: stop + AI (3 s timer starts),
// 7 wait for green, 8 follow competitor (no passing here), 9 avoid: out 45, 10 avoid: pass (side scan), 11 avoid: back 45 + join,
// 12 fault, 13 laps finished, 14 STOP: scan intersection, 15 manual / follow
uint8_t smState(){
 if(mode==0)return 0;
 if(mode!=6)return 15;
 if(traffic.st==RaceRules::HOLD)return traffic.ph?14:6;
 if(traffic.st==RaceRules::WAIT_GREEN)return 7;
 switch(ovt.state){case Avoid::TAIL:return 8;case Avoid::OUT:return 9;case Avoid::PASS:return 10;case Avoid::BACK:case Avoid::JOIN:return 11;default:break;}
 if(follower.state==LineFollower::FAIL)return 12;
 if(traffic.slow())return 5;
 switch(follower.state){case LineFollower::CROSS:return 2;case LineFollower::TURN:return 3;case LineFollower::SEARCH:return 4;default:return 1;}
}
void takeSample(uint32_t now){
 if(ringCount==RING){droppedSamples++;ringHead=(ringHead+1)%RING;ringCount--;} // overwrite oldest
 Sample &s=ring[(ringHead+ringCount)%RING];ringCount++;
 s.q=sampleSeq++;s.t=now;s.l=lineL;s.c=lineM;s.r=lineR;s.pl=leftPWM;s.pr=rightPWM;
 s.g10=(int16_t)(pose.yawRate*10);s.yaw10=(int32_t)(pose.yawDegrees*10);s.distMm=(int32_t)(pose.dist*10);
 s.u=rangeCm;s.x=(int16_t)pose.x;s.y=(int16_t)pose.y;
 s.kh=(int16_t)(pose.kinHeading*(1800/PI));s.kx=(int16_t)pose.kinX;s.ky=(int16_t)pose.kinY; // predicted pose: wheel model only, no gyro
 uint8_t fs=mode==6?(traffic.stopped()?6:(ovt.active()?7:follower.state)):0; // 7 = avoidance drives
#if FOLLOW_ENABLED
 if(mode==7){fs=follow.state;s.l=follow.err10;s.c=follow.widthPm;s.r=follow.bear10;} // slots L,C,R carry heading error x10, target width (per mille), bearing x10
#endif
 s.sm=smState()|(mode==6?(follower.seg<<4):0); // low nibble: state-machine code, high nibble: map segment (lap = 7 -> 0 at S)
 s.st=(uint8_t)((mode<<4)|(pose.gyro?8:0)|fs); // bit3 = gyro working, state 6 = red stop
 s.k=eventMask|follower.events;
 eventMask=0;follower.events=0;
}
bool startTx(int n){if(n>0&&n<(int)sizeof(tx)){txPos=0;txLen=n;return true;}return false;}
void scheduleTx(uint32_t now){
 static uint32_t lastStatus=0;
 if(txPos<txLen)return;
 if(configReply>=0){
  startTx(linkFmt(tx,sizeof(tx),PSTR("{\"N\":222,\"ID\":%d,\"OK\":1,\"CS\":%d,\"RS\":%d,\"DS\":%d}\n"),configReply,cruise,echoRS,echoDS));configReply=-1;return;
 }
 if(pendingReply>=0){startTx(linkFmt(tx,sizeof(tx),PSTR("{\"N\":211,\"OK\":%d}\n"),(int)pendingReply));pendingReply=-1;return;}
 bool statusDue=uint32_t(now-lastStatus)>=STATUS_MS||traffic.changed; // a traffic state change is reported at once (ESP32 starts the STOP AI)
 if(ringCount&&!traffic.changed&&!(statusDue&&uint32_t(now-lastStatus)>=STATUS_MS+250)){
  Sample &s=ring[ringHead];ringHead=(ringHead+1)%RING;ringCount--;
  // d=[q,t,L,C,R,PL,PR,gyro_rate*10,yaw*10,dist_mm,x_cm,y_cm,ultrasonic,state(mode*16+8*gyroOK+follow),mask, predicted: heading*10,x_cm,y_cm]
  // mode 7: L,C,R = heading error*10, target width (per mille), target bearing*10
  startTx(linkFmt(tx,sizeof(tx),PSTR("{\"N\":230,\"d\":[%u,%lu,%d,%d,%d,%d,%d,%d,%ld,%ld,%d,%d,%d,%u,%u,%d,%d,%d,%u]}\n"),
   s.q,(unsigned long)s.t,s.l,s.c,s.r,s.pl,s.pr,s.g10,(long)s.yaw10,(long)s.distMm,s.x,s.y,s.u,(unsigned)s.st,(unsigned)s.k,s.kh,s.kx,s.ky,(unsigned)s.sm));
  return;
 }
 if(statusDue){
  lastStatus=now;traffic.changed=false;
  startTx(linkFmt(tx,sizeof(tx),PSTR("{\"N\":221,\"M\":%u,\"TS\":%u,\"ID\":%u,\"L\":%d,\"C\":%d,\"R\":%d,\"GY\":%u,\"UV\":%d,\"UT\":%lu,\"FC\":%u,\"PL\":%d,\"PR\":%d,\"ES\":%u,\"RN\":%u,\"GN\":%u,\"ZR\":%u,\"IM\":%u,\"CB\":%u,\"GL\":%d,\"GF\":%u,\"SG\":%u,\"LP\":%u,\"OV\":%u,\"Z\":%u,\"CV\":%d,\"SN\":%u,\"SA\":%u}\n"),
   mode,trafficState(),traffic.req,lineL,lineM,lineR,(unsigned)pose.gyro,FIRMWARE_UV,(unsigned long)now,
   (unsigned)(mode==6?follower.fault:0),leftPWM,rightPWM,0u,runNumber,1u,(unsigned)zeroRevision,(unsigned)pose.imuType,(unsigned)pose.calibrating,(int)gyroSignLearned,(unsigned)gyroSignFlips,(unsigned)follower.seg,(unsigned)follower.lapsDone,(unsigned)ovtScore(),(unsigned)traffic.zone,cruise,(unsigned)traffic.stops,(unsigned)traffic.stopsAI));
 }
}
void pumpTx(){int a=Serial.availableForWrite();while(txPos<txLen&&a-->0)Serial.write(tx[txPos++]);}

void setup(){
 Serial.begin(LINK_BAUD);
 pinMode(MOTOR_STBY,OUTPUT);pinMode(MOTOR_LEFT_PWM,OUTPUT);pinMode(MOTOR_RIGHT_PWM,OUTPUT);
 pinMode(MOTOR_LEFT_DIR,OUTPUT);pinMode(MOTOR_RIGHT_DIR,OUTPUT);motorOutput(0,0);
 pinMode(KEY_PIN,INPUT_PULLUP);pinMode(US_TRIG_PIN,OUTPUT);pinMode(US_ECHO_PIN,INPUT);
 #if PAN_SERVO
 pan.attach(SERVO_PAN_PIN);
 #endif
 setPan(90);
 pose.begin(); // keep the car still for ~1 s after power-on (gyro bias)
 motorsOffSince=millis();
}
void loop(){
 uint32_t now=millis();
 serialInput();pumpTx();
 bool parked=mode==0&&!leftPWM&&!rightPWM&&uint32_t(now-motorsOffSince)>1000;
 pose.step(leftPWM,rightPWM,now,parked);
 sensors(); // Test14: physical KEY start/stop removed to save flash (use the web page / power switch)serialInput();
 int left=0,right=0;now=millis();
 #if MANUAL_DRIVE
 if(mode==5){
  if(uint32_t(now-lastRemote)>=REMOTE_TIMEOUT_MS)standby();else directionPWM(manualDir,speedSetting,left,right);
 }else
 #endif
 if(mode==6){
  // ---- vision zone from the map (RaceRules.h): 1 traffic light (O-S, or waiting for green), 2 STOP-sign approach, 0 none ----
  uint8_t sg=follower.seg;traffic.segment(sg);
  traffic.setZone(traffic.st==RaceRules::WAIT_GREEN||sg==7?1:((sg==2||sg==6)&&traffic.st!=RaceRules::DONE?2:0));
  bool fresh=pingSeq!=pingSeen;pingSeen=pingSeq;int ra=usAny();
  traffic.tick(now,ra,fresh);
  if(traffic.zone==2)traffic.stopLine(follower.toO(),stopLine,now);
  // ---- pan servo: intersection scan while holding at a STOP sign, side scan while passing, otherwise straight ----
  int8_t lk=traffic.look();uint8_t want=lk?lookAngle(lk,scanAmp):(ovt.look?lookAngle(ovt.look,SIDE_LOOK_DEG):90);
  if(want!=panAngle)setPan(want);
  if(traffic.stopped()){follower.lastMs=now;ovt.lastDist=pose.dist;} // stand still for red / STOP sign; everything resumes where it was
  else{
   follower.odo(pose.dist);            // odometer (true cm) once per step, also while the avoidance drives
   ovt.armed=follower.seg==5;          // overtaking only on B-C (the obstacle segment)
   bool owns=false;
   if(ovt.enabled||ovt.state!=Avoid::IDLE){
    owns=ovt.step(pose.yawDegrees,pose.yawRate,pose.dist,panAngle==90||ovt.look?ra:-2,fresh,
     follower.settled(now)||(ovt.state==Avoid::TAIL&&follower.state==LineFollower::FOLLOW),follower.refYaw,follower.outside(),follower.toCorner(),sg==2||sg==6,blackL,blackM,blackR,now,left,right);
    if(owns)follower.lastMs=now;
   }
   if(!owns){
    follower.step(blackL,blackM,blackR,now,pose.yawDegrees,pose.gyro,usRange(),left,right);
    uint8_t pct=traffic.slow()?slowPct:100;if(ovt.scale<pct)pct=ovt.scale; // slower towards a STOP sign / behind a competitor
    {int u=usRange();if(u>=0&&u<=(ovt.enabled?Avoid::STOP_CM:FORWARD_GUARD_CM))pct=0;} // last-resort guard: wait behind it (no E-stop latch)
    if(pct<100&&left>0&&right>0){left=left*pct/100;right=right*pct/100;}
   }
   if(ovt.handover){ // back on the line: the follower continues, map progress corrected
    follower.resume(pose.yawDegrees,ovt.H,now);follower.segStart+=ovt.lostCm;
    if(ovt.lost)follower.enterSearch(now);ovt.handover=false;
   }
  }
  if(follower.requestSignFlip){ // commanded a left pivot but the gyro said right: the gyro sign (GS) is reversed
   follower.requestSignFlip=false;pose.gyroSign=-pose.gyroSign;pose.yawDegrees=-pose.yawDegrees;pose.heading=-pose.heading;
   follower.negateAngles();gyroSignLearned=pose.gyroSign>0?1:-1;gyroSignFlips++;
  }
 }
#if FOLLOW_ENABLED
 else if(mode==7){
  follow.step(pose.yawDegrees,pose.yawRate,now,left,right);
 }
#endif
 motorOutput(left,right);
 if(pose.calDone){pose.calDone=false;zeroRevision++;}
 uint32_t period=mode?SAMPLE_MS:IDLE_SAMPLE_MS;
 if(uint32_t(now-lastSample)>=period){lastSample=now;takeSample(now);}
 scheduleTx(now);pumpTx();
}
