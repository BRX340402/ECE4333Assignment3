from pathlib import Path
import re,subprocess,tempfile
r=Path(__file__).resolve().parents[1]
u=(r/'UNO_Assignment2/UNO_Assignment2.ino').read_text();e=(r/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text()
setup=e[e.index('void setup(){'):e.index(' frameMutex=xSemaphoreCreateMutex()',e.index('void setup(){'))]
assert 'sendJSON("{\\"N\\":100}")' not in setup
assert 'sendJSON("{\\"N\\":214}")' in setup
assert 'remoteOwned&&mode!=6&&' in u
assert 'char txBuffer[352]' in u and 'unoRx.length()<352' in e
assert 'lastTrafficTx>=600' in u
handler=re.search(r'if\(n==214\)\{(.*?)\}',u).group(1)
code='''#include <cassert>
struct Seen {bool seen=true;} vision,traffic;
bool aiResultSeen=true;int mode=6,leftPWM=60,rightPWM=60;
void boot(){'''+handler+'''}
int main(){boot();assert(!aiResultSeen&&!vision.seen&&!traffic.seen);assert(mode==6&&leftPWM==60&&rightPWM==60);}
'''
with tempfile.TemporaryDirectory() as t:
 p=Path(t);(p/'test.cpp').write_text(code);subprocess.run(['g++',str(p/'test.cpp'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
print('PASS ESP32 boot sends no STOP, frame-sequence reset preserves driving, autonomous remote-timeout exclusion, matching telemetry capacities')
