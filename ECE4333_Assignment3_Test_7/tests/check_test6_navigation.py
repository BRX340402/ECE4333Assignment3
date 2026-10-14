from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'UNO_Assignment2/UNO_Assignment2.ino').read_text()
block=s[s.index(' if(n==208||n==213)'):s.index(' if(n==207)')]
assert s.count('pose.yawDegrees,gyroNavigation&&pose.gyro,')==3
assert 'gyroNavigation&&pose.gyro?pose.yawRate:0' in s
code='''#include <cassert>
#include <cstdint>
#define PSTR(x) x
int mode=0;bool teamPD=false,gyroNavigation=false;uint8_t zeroRevision=0;bool acknowledged=false;
unsigned millis(){return 100;}
void reply(bool v){acknowledged=v;}
struct FakeDoc {struct Field{bool isInt16(){return true;}};Field operator[](const char*){return {};}}doc;
struct Reset{void reset(){}}teamControl,ex3;
struct Pose{int resets=0;void zero(unsigned){resets++;}}pose;
void command(int n,int d){
'''+block+'''}
int main(){
 assert(!gyroNavigation);
 command(213,1);assert(gyroNavigation&&zeroRevision==1&&pose.resets==1&&acknowledged);
 command(213,0);assert(!gyroNavigation&&zeroRevision==1&&pose.resets==1);
 command(213,2);assert(!gyroNavigation&&zeroRevision==2&&pose.resets==2);
 mode=6;command(213,1);assert(!gyroNavigation&&!acknowledged&&zeroRevision==2);
 command(213,2);assert(pose.resets==2&&!acknowledged);
 mode=0;command(213,3);assert(!gyroNavigation&&!acknowledged);
 command(208,1);assert(teamPD&&!gyroNavigation);
}
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['g++','-std=c++11',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS actual N213 handler: default OFF, enable + zero, disable, zero while OFF, moving-state rejection, invalid value, independent PD; gyro motor gates.')
