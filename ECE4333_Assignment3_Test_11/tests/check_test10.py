from pathlib import Path
import subprocess,tempfile,re,gzip
from html.parser import HTMLParser
r=Path(__file__).resolve().parents[1]
p=(r/'ESP32_Assignment2/TrafficPage.h').read_text().split('R"HTML(',1)[1].split(')HTML";',1)[0]
js=p.split('<script>',1)[1].split('</script>',1)[0]
class IDs(HTMLParser):
 def __init__(self):super().__init__();self.ids=[]
 def handle_starttag(self,tag,attrs):
  for k,v in attrs:
   if k=='id':self.ids.append(v)
h=IDs();h.feed(p);assert len(h.ids)==len(set(h.ids))
for ref in re.findall(r"\$\('([^']+)'\)",js):assert ref in h.ids,ref
assert gzip.decompress(bytes(int(x,16) for x in re.findall(r'0x([0-9a-f]{2})',(r/'ESP32_Assignment2/TrafficPageGzip.h').read_text()))).decode()==p
uno=(r/'UNO_Assignment2/UNO_Assignment2.ino').read_text();assert 'gyroNavigation=true' in uno
assert '!pose.gyro||imuVerified!=3' in uno
handler=uno[uno.index(' if(n==215){'):uno.index(' if(n==213){')]
code='''#include <cassert>
bool answer=false;int mode=0;unsigned char imuVerified=0;float checkYaw=0;
struct P {bool gyro=true;float yawDegrees=0;} pose;
void reply(bool v){answer=v;}
void check(int d){int n=215;'''+handler+'''}
int main(){check(0);pose.yawDegrees=-30;check(1);assert(!answer&&imuVerified==0);
pose.yawDegrees=30;check(1);assert(answer&&imuVerified==1);
pose.yawDegrees=0;check(2);assert(answer&&imuVerified==3);
mode=6;check(0);assert(!answer&&imuVerified==3);
mode=0;pose.gyro=false;check(0);assert(!answer);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'page.js').write_text(js);subprocess.run(['node','--check',str(t/'page.js')],check=True)
 (t/'direction.cpp').write_text(code);subprocess.run(['g++',str(t/'direction.cpp'),'-o',str(t/'direction')],check=True);subprocess.run([str(t/'direction')],check=True)
 for name in ['test10_recovery','test1_behavior']:
  subprocess.run(['g++','-std=c++11',str(r/'tests'/f'{name}.cpp'),'-o',str(t/name)],check=True);subprocess.run([str(t/name)],check=True)
print('PASS unique UI IDs, DOM references, JavaScript syntax, gzip roundtrip, default gyro, autonomous admission gate, actual UNO direction checks')
