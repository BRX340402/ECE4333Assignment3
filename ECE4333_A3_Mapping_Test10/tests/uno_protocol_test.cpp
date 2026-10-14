// Runs the real UNO_Mapping.ino on the PC and checks the serial protocol.
//   g++ -std=c++14 -Iuno_host_stubs -I../UNO_Mapping uno_protocol_test.cpp -o t && ./t
#include "Arduino.h"
#include "Wire.h"
uint32_t g_ms=1000;int g_analog[20];unsigned long g_pulse=0;int g_pwm[20],g_dig[20];
bool g_imuPresent=true;int16_t g_gz=0;HostSerial Serial;TwoWire Wire;char __heap_start;char *__brkval=nullptr;
#include "../UNO_Mapping/UNO_Mapping.ino"
#include <vector>
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
 CHECK(lines("\"UV\":40").size()>=2,"N221 reports UV=40");
 CHECK(lines("\"N\":230").size()>=5,"N230 samples at 2 Hz while idle");
 CHECK(lines("\"N\":210").size()>=1,"N210 sensors line");
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
 CHECK(!s.empty()&&s.back().find(",105,2]")!=std::string::npos,"sample event mask present (centre sensor saw black -> 2)");
 printf("     longest line %zu bytes (ESP32 accepts <352)\n",maxLen());
 CHECK(maxLen()<352,"every line fits the ESP32 receive buffer");
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
 printf(fails?"\n%d FAILED\n":"\nALL UNO PROTOCOL CHECKS PASSED\n",fails);return fails?1:0;
}
