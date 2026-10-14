from pathlib import Path
import subprocess,tempfile
p=Path(__file__).resolve().parents[1]
s=(p/'UNO_Assignment2/UNO_Assignment2.ino').read_text()
line=next(l.strip() for l in s.splitlines() if 'if(remoteOwned&&mode!=6' in l)
code='#include <cassert>\n#include <cstdint>\nint mode=6,reason=0;bool remoteOwned=true,irOwned=false;uint32_t lastRemote=0,now=60000;const int REMOTE_TIMEOUT_MS=1500;\nuint32_t millis(){return now;}void standby(){mode=0;}\nvoid check(){'+line+'}\nint main(){check();assert(mode==6&&reason==0);mode=5;check();assert(mode==0&&reason==2);mode=5;irOwned=true;now=300;check();assert(mode==0);mode=6;standby();assert(mode==0);}\n'
with tempfile.TemporaryDirectory() as d:
 f=Path(d)/'test.cpp';f.write_text(code);exe=Path(d)/'test'
 subprocess.run(['g++','-std=c++11',str(f),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
print('PASS V19 exact watchdog: autonomous survives missing heartbeat; manual and IR still expire.')
