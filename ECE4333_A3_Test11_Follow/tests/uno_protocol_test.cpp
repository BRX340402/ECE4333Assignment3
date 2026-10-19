// Runs the real UNO_Mapping.ino on the PC and checks the serial protocol.
//   g++ -std=c++14 -Iuno_host_stubs -I../UNO_Mapping uno_protocol_test.cpp -o t && ./t
#include "Arduino.h"
#include "Wire.h"
uint32_t g_ms=1000;int g_analog[20];unsigned long g_pulse=0;int g_pwm[20],g_dig[20];
bool g_imuPresent=true;int16_t g_gz=0;HostSerial Serial;TwoWire Wire;char __heap_start;char *__brkval=nullptr;
#include "../UNO_Mapping/UNO_Mapping.ino"
#include <vector>
#include <algorithm>
#include <cassert>
static int fails=0;
#define CHECK(c,msg) do{if(!(c)){printf("FAIL: %s\n",msg);fails++;}else printf("ok   %s\n",msg);}while(0)
void send(const char*s){for(const char*p=s;*p;p++)Serial.in.push_back(*p);Serial.in.push_back('\n');}
void runMs(uint32_t ms){uint32_t end=g_ms+ms;while(g_ms<end){loop();g_ms+=2;}}
std::vector<std::string> lines(const char*tag){std::vector<std::string> v;size_t p=0;std::string &o=Serial.out;
 while(true){size_t e=o.find('\n',p);if(e==std::string::npos)break;std::string l=o.substr(p,e-p);if(l.find(tag)!=std::string::npos)v.push_back(l);p=e+1;}return v;}
size_t maxLen(){size_t m=0,p=0;std::string &o=Serial.out;while(true){size_t e=o.find('\n',p);if(e==std::string::npos)break;m=std::max(m,e-p+1);p=e+1;}return m;}
int main(){
 g_analog[A2]=100;g_analog[A1]=900;g_analog[A0]=100;g_analog[A3]=200;g_pulse=0; // centre on black, no echo
 setup();runMs(3000);
 CHECK(lines("\"N\":221").size()>=2,"N221 status about every second while idle");
 CHECK(lines("\"UV\":41").size()>=2,"N221 reports UV=41");
 CHECK(lines("\"N\":230").size()>=5,"N230 samples at 2 Hz while idle");
 CHECK(lines("\"N\":210").size()==0,"N210 battery/sensor line is gone (flash saved)");
 send("{\"N\":101,\"D1\":6}");runMs(50);
 CHECK(lines("\"N\":211,\"OK\":0").size()==1,"Start refused before settings arrive");
 send("{\"N\":206,\"ID\":7,\"SC\":25,\"BL\":600,\"BH\":1023,\"RA\":180,\"SA\":360,\"CT\":1500,\"GS\":0,\"CS\":60,\"RS\":0,\"DS\":30}");runMs(100);
 CHECK(lines("{\"N\":222,\"ID\":7,\"OK\":1,\"CS\":60,\"RS\":0,\"DS\":30}").size()==1,"settings acknowledged with N222 echo (ESP32 format)");
 send("{\"N\":204,\"S\":1,\"V\":1,\"R\":1,\"G\":0,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");send("{\"N\":203,\"R\":1}");send("{\"N\":200,\"S\":1,\"A\":1,\"V\":1,\"P\":0,\"R\":-1,\"X\":-1,\"Y\":-1,\"W\":0,\"H\":0,\"O\":0}");
 size_t replies=lines("\"N\":211").size();runMs(100);
 CHECK(lines("\"N\":211").size()==replies,"camera/traffic messages ignored silently");
 Serial.out.clear();
 send("{\"N\":101,\"D1\":6}");runMs(2000);
 CHECK(lines("\"N\":211,\"OK\":1").size()==1,"Start accepted");
 CHECK(mode==6&&follower.state==LineFollower::FOLLOW,"mapping run active, FOLLOW");
 CHECK(g_pwm[MOTOR_LEFT_PWM]==60&&g_pwm[MOTOR_RIGHT_PWM]==60,"cruise PWM 60 on a centred line");
 auto s=lines("\"N\":230");
 CHECK(s.size()>=18&&s.size()<=21,"N230 at 10 Hz while driving (2 s)");
 CHECK(!s.empty()&&s.back().find(",105,")!=std::string::npos,"sample state field = mode 6 + gyro OK + FOLLOW (0x69=105)");
 CHECK(lines("\"IM\":1").size()>=1,"N221 reports the detected IMU type");
 CHECK(lines("\"MQ\":").size()>=1,"N221 carries sample counter");
 CHECK(!s.empty()&&s.back().find(",105,2,")!=std::string::npos,"sample event mask present (centre sensor saw black -> 2)");
 CHECK(!s.empty()&&std::count(s.back().begin(),s.back().end(),',')==18,"N230 carries 18 values: 15 old + predicted heading, x, y");
 printf("     longest line %zu bytes (tx buffer %zu, ESP32 accepts <352)\n",maxLen(),sizeof(tx));
 CHECK(maxLen()<sizeof(tx),"every line fits the UNO tx buffer");
 if(!s.empty())printf("     example: %s\n",s.back().c_str());
 g_pulse=58*10;runMs(150);
 CHECK(!eStopLatched,"one near ultrasonic reading does not latch E-stop");
 runMs(400);
 CHECK(eStopLatched&&eStopReason==2&&g_pwm[MOTOR_LEFT_PWM]==0,"3 consecutive near readings latch E-stop, motors off");
 send("{\"N\":207,\"D1\":0}");runMs(50);
 CHECK(eStopLatched,"E-stop cannot be cleared while obstacle is still near");
 g_pulse=0;runMs(300);send("{\"N\":207,\"D1\":0}");runMs(50);
 CHECK(!eStopLatched&&mode==0,"E-stop cleared when clear, car stays stopped");
 send("{\"N\":101,\"D1\":6}");runMs(300);send("{\"N\":100}");runMs(50);
 CHECK(mode==0&&g_pwm[MOTOR_LEFT_PWM]==0,"STOP (N100) stops the run");
 send("{\"N\":102,\"D1\":1,\"D2\":80}");runMs(100);
 CHECK(mode==5&&g_pwm[MOTOR_LEFT_PWM]==80,"manual drive (web arrows) works");
 runMs(2000);
 CHECK(mode==0,"manual drive times out without heartbeat");
 // no IMU on the bus: gyro must be reported missing and a mapping Start refused
 g_imuPresent=false;runMs(600);
 CHECK(!pose.gyro,"failed gyro reads are reported (GY=0)");
 Serial.out.clear();send("{\"N\":101,\"D1\":6}");runMs(100);
 CHECK(mode==0&&lines("\"N\":211,\"OK\":0").size()==1,"Start refused while the gyro is not working");
 g_imuPresent=true;runMs(3500);
 CHECK(pose.gyro,"gyro re-detected automatically while parked");
 Serial.out.clear();send("{\"N\":101,\"D1\":6}");runMs(100);
 CHECK(mode==6,"Start accepted again once the gyro works");
 // web-page "Zero + recalibrate": N213 D1=2 while stopped
 send("{\"N\":100}");runMs(1200);pose.yawDegrees=37.5f;uint8_t zr0=zeroRevision;Serial.out.clear();
 send("{\"N\":213,\"D1\":2}");runMs(300);
 CHECK(pose.calibrating&&lines("\"N\":211,\"OK\":1").size()==1,"zero accepted, 1 s bias re-measurement running");
 send("{\"N\":101,\"D1\":6}");runMs(100);
 CHECK(mode==0,"Start refused while the gyro is calibrating");
 runMs(1500);runMs(1200);
 CHECK(!pose.calibrating&&zeroRevision==(uint8_t)(zr0+1)&&fabs(pose.yawDegrees)<0.01f,"after calibration heading is 0 and ZR increments");
 CHECK(lines("\"CB\":1").size()>=1&&lines("\"CB\":0").size()>=1,"N221 reports CB (calibrating) 1 then 0");
 CHECK(lines("\"N\":230").size()>=10,"idle N230 at 5 Hz for the live gyro readout");
 // reversed gyro sign: at the first corner pivot (commanded LEFT) the gyro reports a RIGHT turn
 send("{\"N\":100}");runMs(1500);Serial.out.clear();
 send("{\"N\":101,\"D1\":6}");runMs(1200);                 // following, centre black
 g_analog[A2]=100;g_analog[A1]=100;g_analog[A0]=100;            // line ends
 g_analog[A2]=900;runMs(60);g_analog[A2]=100;                    // left sensor saw tape last -> pivot left
 g_gz=(int16_t)(65.5f*120);                                      // with GS=0 (sign -1) this reads as -120 deg/s = right turn
 runMs(400);
 CHECK(mode==6&&follower.state!=LineFollower::FAIL,"reversed gyro sign does not stop the run");
 CHECK(gyroSignLearned!=0&&gyroSignFlips==1,"gyro sign auto-corrected at the first pivot");
 CHECK(pose.yawRate>0,"after correction a left pivot reads as a LEFT (positive) turn");
 g_gz=0;runMs(1100);
 CHECK(lines("\"GF\":1").size()>=1,"N221 reports the correction (GF/GL) for the web page");
 send("{\"N\":100}");runMs(1500);
 send("{\"N\":206,\"ID\":8,\"SC\":25,\"BL\":600,\"BH\":1023,\"RA\":180,\"SA\":360,\"CT\":1500,\"GS\":0,\"CS\":60,\"RS\":0,\"DS\":30}");runMs(100);
 CHECK(pose.gyroSign>0,"saved GS from the ESP32 does not undo the learned sign until power-off");
 // hysteresis: a reading wobbling around the threshold does not flicker
 g_analog[A1]=900;runMs(20);bool wasBlack=blackM;g_analog[A1]=590;runMs(20);bool s1=blackM;g_analog[A1]=620;runMs(20);bool s2=blackM;
 g_analog[A1]=540;runMs(20);bool s3=blackM;g_analog[A1]=620;runMs(20);bool s4=blackM;
 CHECK(wasBlack&&s1&&s2&&!s3&&!s4,"line sensor hysteresis (black>=640 to enter, <560 to leave at BL=600)");
 // ---- Stop on red (RS=1) ----
 send("{\"N\":100}");runMs(1500);g_analog[A2]=100;g_analog[A1]=900;g_analog[A0]=100;
 send("{\"N\":206,\"ID\":9,\"SC\":25,\"BL\":600,\"BH\":1023,\"RA\":180,\"SA\":360,\"CT\":1500,\"GS\":1,\"CS\":60,\"RS\":1,\"DS\":30}");runMs(100);
 send("{\"N\":101,\"D1\":6}");runMs(800);
 CHECK(mode==6&&traffic.enabled&&g_pwm[MOTOR_LEFT_PWM]>0,"run with Stop on red enabled, driving");
 Serial.out.clear();
 send("{\"N\":204,\"S\":11,\"V\":1,\"R\":1,\"G\":0,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");runMs(100);
 CHECK(traffic.st==TrafficLite::VERIFY&&g_pwm[MOTOR_LEFT_PWM]==0&&g_pwm[MOTOR_RIGHT_PWM]==0,"red >= RP: car stops (VERIFY)");
 CHECK(lines("\"TS\":4,\"ID\":1").size()>=1,"TS=4 + request ID sent at once so the ESP32 starts the STOP AI");
 send("{\"N\":205,\"S\":1,\"ID\":1,\"V\":1,\"P\":900}");runMs(50);send("{\"N\":205,\"S\":2,\"ID\":1,\"V\":1,\"P\":850}");runMs(50);
 CHECK(traffic.st==TrafficLite::HOLD&&g_pwm[MOTOR_LEFT_PWM]==0,"2 STOP-sign AI hits: 3 s hold");
 runMs(3200);
 CHECK(traffic.st==TrafficLite::CLEARING&&g_pwm[MOTOR_LEFT_PWM]>0,"after 3 s the car drives on");
 send("{\"N\":204,\"S\":12,\"V\":1,\"R\":1,\"G\":0,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");runMs(50);
 CHECK(traffic.st==TrafficLite::CLEARING&&g_pwm[MOTOR_LEFT_PWM]>0,"the same red sign does not stop it again");
 for(int i=13;i<16;i++){char b[120];snprintf(b,sizeof b,"{\"N\":204,\"S\":%d,\"V\":1,\"R\":0,\"G\":0,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}",i);send(b);runMs(50);}
 CHECK(traffic.st==TrafficLite::GO,"re-armed after 3 frames without red");
 send("{\"N\":204,\"S\":20,\"V\":1,\"R\":1,\"G\":0,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");runMs(50);
 send("{\"N\":205,\"S\":3,\"ID\":2,\"V\":1,\"P\":0}");runMs(50);send("{\"N\":205,\"S\":4,\"ID\":2,\"V\":1,\"P\":0}");runMs(50);
 CHECK(traffic.st==TrafficLite::WAIT_GREEN&&g_pwm[MOTOR_LEFT_PWM]==0,"red light (AI says not STOP): wait for green");
 send("{\"N\":204,\"S\":21,\"V\":1,\"R\":0,\"G\":1,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");runMs(50);
 CHECK(traffic.st==TrafficLite::WAIT_GREEN,"one green frame is not enough");
 send("{\"N\":204,\"S\":22,\"V\":1,\"R\":0,\"G\":1,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");runMs(100);
 CHECK(traffic.st==TrafficLite::CLEARING&&g_pwm[MOTOR_LEFT_PWM]>0,"two green frames: drive on");
 send("{\"N\":100}");runMs(100);
 send("{\"N\":206,\"ID\":10,\"SC\":25,\"BL\":600,\"BH\":1023,\"RA\":180,\"SA\":360,\"CT\":1500,\"GS\":1,\"CS\":60,\"RS\":0,\"DS\":30}");runMs(100);
 send("{\"N\":101,\"D1\":6}");runMs(300);
 send("{\"N\":204,\"S\":30,\"V\":1,\"R\":1,\"G\":0,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");runMs(100);
 CHECK(traffic.st==TrafficLite::GO&&g_pwm[MOTOR_LEFT_PWM]>0,"Stop on red OFF: red is ignored (mapping only)");

 // ---- Test11 additions ----
 // linkFmt (replaces snprintf_P / vfprintf) matches snprintf
 { bool same=true;srand(5);for(int i=0;i<2000;i++){int a=(rand()%65536)-32768;unsigned b=rand()%65536;long c=(long)(int32_t)(((uint32_t)rand()<<9)^(uint32_t)rand());unsigned long d=(unsigned long)(uint32_t)(((uint32_t)rand()<<16)^(uint32_t)rand());
    char x[80],y[80];int n1=linkFmt(x,sizeof x,"v=%d,%u,%ld,%lu,%d%%",a,b,c,d,0),n2=snprintf(y,sizeof y,"v=%d,%u,%ld,%lu,%d%%",a,b,c,d,0);if(n1!=n2||strcmp(x,y))same=false;}
   char s5[6];int n=linkFmt(s5,sizeof s5,"%ld",-1234567L);
   CHECK(same&&n==8&&strcmp(s5,"-1234")==0,"linkFmt matches snprintf on 2000 random values and reports truncation like snprintf"); }
 // longest N221 / N230 with every field at its widest
 { mode=7;follow.state=TargetFollow::SWEEP;traffic.req=65535;lineL=lineM=lineR=1023;leftPWM=rightPWM=-180;runNumber=65535;zeroRevision=255;sampleSeq=65535;
   pose.imuType=2;pose.whoMpu=255;pose.whoQmi=255;pose.busRecoveries=255;pose.calibrating=true;gyroSignLearned=-1;gyroSignFlips=255;follower.seg=4;follower.lapsDone=9;eStopLatched=true;
   txPos=txLen=0;configReply=-1;pendingReply=-1;ringCount=0;g_ms=4294967000u;scheduleTx(g_ms+500);
   std::string l(tx,txLen);size_t fr=l.find("\"FR\":")+5,fe=l.find(',',fr);size_t n221=txLen-(fe-fr)+6; // FR is an AVR int: at most 6 characters
   printf("     widest N221 %zu bytes, tx buffer %zu\n",n221,sizeof(tx));
   CHECK(n221<sizeof(tx),"widest possible N221 status line fits the UNO tx buffer (256)");
   Sample w;w.q=65535;w.t=4294967295u;w.yaw10=-2147483647-1;w.distMm=-2147483647-1;w.l=w.c=w.r=w.pl=w.pr=w.g10=w.u=w.x=w.y=w.kh=w.kx=w.ky=-32768;w.st=255;w.k=255;
   ring[0]=w;ringHead=0;ringCount=1;txPos=txLen=0;traffic.changed=false;scheduleTx(g_ms+500);
   printf("     widest N230 %d bytes\n",(int)txLen);CHECK(txLen>0&&txLen<sizeof(tx),"widest possible N230 sample fits the UNO tx buffer");
   mode=0;eStopLatched=false;pose.calibrating=false;txPos=txLen=0;ringCount=0;g_ms=1000000;standby(); }
 // ---- vision target following (mode 7) ----
 send("{\"N\":100}");runMs(300);
 send("{\"N\":206,\"ID\":11,\"SC\":25,\"BL\":600,\"BH\":1023,\"RA\":180,\"SA\":360,\"CT\":1500,\"GS\":1,\"CS\":70,\"RS\":0,\"DS\":30}");runMs(100);
 g_analog[A2]=100;g_analog[A1]=100;g_analog[A0]=100;g_gz=0;Serial.out.clear();
 send("{\"N\":101,\"D1\":7,\"D2\":5,\"D3\":62,\"D4\":15,\"D5\":10}");runMs(100);
 CHECK(mode==0&&lines("\"N\":211,\"OK\":0").size()==1,"follow start with an out-of-range stop width is refused");
 Serial.out.clear();send("{\"N\":101,\"D1\":7,\"D2\":250,\"D3\":62,\"D4\":15,\"D5\":10}");runMs(400);
 CHECK(mode==7&&lines("\"N\":211,\"OK\":1").size()==1,"follow start accepted (mode 7)");
 CHECK(g_pwm[MOTOR_LEFT_PWM]==0&&g_pwm[MOTOR_RIGHT_PWM]==0&&follow.state==TargetFollow::HOLD,"no target yet: car stands still");
 CHECK(lines("\"TS\":11").size()>=1,"N221 TS = 11 while no target (stopped)");
 Serial.out.clear();
 // target 12 deg to the left, small in the image (far): drive on, turning left (right wheel faster)
 for(int i=0;i<8;i++){char b[100];snprintf(b,sizeof b,"{\"N\":215,\"S\":%d,\"V\":1,\"T\":1,\"X\":300,\"W\":100,\"A\":100}",i+1);send(b);runMs(100);}
 CHECK(follow.state==TargetFollow::TRACK&&g_pwm[MOTOR_RIGHT_PWM]>g_pwm[MOTOR_LEFT_PWM],"target left of centre: TRACK, right wheel faster (turning left)");
 CHECK(lines("\"TS\":9").size()>=1,"N221 TS = 9 while tracking");
 auto f7=lines("\"N\":230");
 CHECK(!f7.empty()&&f7.back().find(",121,")!=std::string::npos,"mode 7 sample state byte = 0x79 (121): mode 7, gyro OK, TRACK");
 // target wide enough: arrived, motors stop driving forward
 for(int i=0;i<8;i++){char b[100];snprintf(b,sizeof b,"{\"N\":215,\"S\":%d,\"V\":1,\"T\":1,\"X\":500,\"W\":300,\"A\":100}",i+20);send(b);runMs(100);}
 CHECK(follow.state==TargetFollow::ARRIVED&&g_pwm[MOTOR_LEFT_PWM]==0&&g_pwm[MOTOR_RIGHT_PWM]==0,"target at the stop width: ARRIVED, car stops");
 runMs(1700);
 CHECK(follow.state==TargetFollow::HOLD&&g_pwm[MOTOR_LEFT_PWM]==0,"no more camera frames: car stops after 1.5 s");
 send("{\"N\":100}");runMs(100);CHECK(mode==0,"STOP ends target following");
 // obstacle straight ahead while following: ultrasonic E-stop still works
 send("{\"N\":101,\"D1\":7,\"D2\":250,\"D3\":62,\"D4\":15,\"D5\":10}");runMs(200);
 for(int i=0;i<6;i++){char b[100];snprintf(b,sizeof b,"{\"N\":215,\"S\":%d,\"V\":1,\"T\":1,\"X\":500,\"W\":50,\"A\":100}",i+40);send(b);runMs(100);}
 CHECK(mode==7&&g_pwm[MOTOR_LEFT_PWM]>0,"following, driving forward at a far target");
 g_pulse=58*10;runMs(700);
 CHECK(eStopLatched&&g_pwm[MOTOR_LEFT_PWM]==0,"obstacle < limit while following: E-stop latches");
 g_pulse=0;runMs(300);send("{\"N\":207,\"D1\":0}");runMs(100);CHECK(!eStopLatched&&mode==0,"E-stop cleared");
 // ---- wait for green before driving off, lap limit ----
 g_analog[A2]=100;g_analog[A1]=900;g_analog[A0]=100;
 send("{\"N\":101,\"D1\":6,\"D2\":1,\"D3\":0}");runMs(600);
 CHECK(mode==6&&traffic.st==TrafficLite::WAIT_GREEN&&g_pwm[MOTOR_LEFT_PWM]==0,"Start with 'wait for green': car stands still (WAIT FOR GREEN)");
 send("{\"N\":204,\"S\":61,\"V\":1,\"R\":0,\"G\":1,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");send("{\"N\":204,\"S\":62,\"V\":1,\"R\":0,\"G\":1,\"D\":-1,\"SD\":-1,\"B\":0,\"LC\":0}");runMs(200);
 CHECK(g_pwm[MOTOR_LEFT_PWM]>0,"two green frames: the car drives off");
 send("{\"N\":100}");runMs(100);
 send("{\"N\":101,\"D1\":6,\"D2\":0,\"D3\":2}");runMs(500);
 CHECK(mode==6&&lapsLimit==2&&g_pwm[MOTOR_LEFT_PWM]>0,"Start with a 2-lap limit: driving");
 follower.lapsDone=1;runMs(50);CHECK(mode==6,"one lap done: keeps going");
 follower.lapsDone=2;runMs(50);CHECK(mode==0&&g_pwm[MOTOR_LEFT_PWM]==0,"two laps done: the car stops");
 printf(fails?"\n%d FAILED\n":"\nALL UNO PROTOCOL CHECKS PASSED\n",fails);return fails?1:0;
}
