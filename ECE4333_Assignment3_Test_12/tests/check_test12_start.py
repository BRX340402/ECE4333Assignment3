from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1];s=(r/'UNO_Assignment2/UNO_Assignment2.ino').read_text()
gate=s[s.index(' if(eStopLatched||(m==6'):s.index(' motorOutput(0,0);traffic.stop',s.index('void chooseMode'))]
handler=s[s.index(' if(n==101){'):s.index(' if(n==102){')]
code='''#include <cassert>
int mode=0;bool eStopLatched=false,gyroNavigation=false,settingsReady=false,answer=false,admitted=false;
int imuVerified=0;struct {int routeMode=1;}ex3;struct {bool gyro=false;}pose;
struct V{bool fresh(int){return true;}}vision;
int millis(){return 0;}void reply(bool b){answer=b;}void standby(){mode=0;}
void chooseMode(int m,bool){'''+gate+'''admitted=true;mode=m;}
void start(int d){int n=101;'''+handler+'''}
int main(){start(6);assert(admitted&&mode==6);mode=0;admitted=false;ex3.routeMode=2;start(6);assert(!admitted&&!answer);
ex3.routeMode=1;eStopLatched=true;start(6);assert(!admitted&&mode==0);
eStopLatched=false;settingsReady=true;gyroNavigation=true;pose.gyro=true;imuVerified=3;ex3.routeMode=2;start(6);assert(admitted);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code);subprocess.run(['g++',str(p/'test.cpp'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('PASS actual UNO startup guards: survey bypass, traffic-mode checks retained, E-stop still prevents motor mode admission')
