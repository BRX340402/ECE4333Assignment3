from pathlib import Path
import tempfile,subprocess,re,json
r=Path(__file__).resolve().parents[1]
u=(r/'UNO_Assignment2/UNO_Assignment2.ino').read_text()
prefix=u[u.index(' IntegerCommand doc;'):u.index(' if(n==214)')]
handler=u[u.index(' if(n==216){'):u.index(' if(n==215){')]
code='''#include <cassert>
#include <cstdlib>
#include <string>
#include "IntegerCommand.h"
#include "Ex3Tracking.h"
Ex3Tracking ex3;int mode=0;bool answer=false;void reply(bool b){answer=b;}
void command(char *text){'''+prefix+handler+'''}
void send(std::string s){answer=false;command(&s[0]);}
int main(){send("{\\"N\\":216,\\"D1\\":2,\\"D2\\":0,\\"D3\\":-145}");assert(answer&&ex3.routeRevision==1&&ex3.turns[0]==-145&&ex3.routeMode==2);
mode=6;send("{\\"N\\":216,\\"D1\\":1,\\"D2\\":0,\\"D3\\":-150}");assert(!answer&&ex3.routeRevision==1);
mode=0;for(const char *v:{"151","-151","0","19","32768","-32769","1.5","null"}){send(std::string("{\\"N\\":216,\\"D1\\":1,\\"D2\\":4,\\"D3\\":")+v+"}");assert(!answer&&ex3.routeRevision==1);}
send("{\\"N\\":216,\\"D1\\":1,\\"D2\\":0}");assert(!answer);
send("{\\"N\\":216,\\"D1\\":1,\\"D2\\":5,\\"D3\\":120}");assert(!answer);
send("{\\"N\\":216,\\"D1\\":1,\\"D2\\":4,\\"D3\\":120}");assert(answer&&ex3.routeRevision==2);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 t=Path(tmp);(t/'avr').mkdir();(t/'avr/pgmspace.h').write_text('#pragma once\n#define PSTR(x) (x)\n#define pgm_read_byte(x) (*(const unsigned char*)(x))\n')
 (t/'test.cpp').write_text(code);subprocess.run(['g++','-std=c++11','-I'+str(t),'-I'+str(r/'UNO_Assignment2'),str(t/'test.cpp'),'-o',str(t/'test')],check=True);subprocess.run([str(t/'test')],check=True)
assert 'if(ex3.routeMode==1)return;' in u[u.index(' if(n==204){'):u.index(' if(n==200){')]
assert 'if(ex3.routeMode==1){traffic.seen=false;traffic.red=false;}traffic.start' in u
# Worst expected telemetry values, not only zeros, must fit the fixed serial buffer.
values=dict(M=6,TS=8,ID=65535,X=-32768,Y=-32768,H=-180,KX=-32768,KY=-32768,KH=-180,L=1023,C=1023,R=1023,GY=1,YA=-32768,FR=-32768,UT=4294967295,FC=16,FS=8,SE=1,SP=6,GC=2,PL=-180,PR=-180,ES=1,ER=2,RN=65535,GN=1,ZR=255,IC=255,IV=3,B=255,G=1,T=1,U=-1,V=9999,S=180,Q=65535,A=-180,D=-180,AI=1,VF=1,P=1000,VD=10000,LED=2,GR10=-32768,RM=2,RI=4,RL=255,RT=-32768,RC=255)
for line in u.splitlines():
 if 'snprintf_P(txBuffer' not in line:continue
 fmt=re.search(r'PSTR\(("(?:\\.|[^"\\])*")\)',line)
 if not fmt:continue
 text=json.loads(fmt[1]);text=re.sub(r'"([A-Z0-9]+)":%[a-z]+',lambda m:'"'+m[1]+'":'+str(values.get(m[1],65535)),text)
 assert len(text)<352,(len(text),text)
 print('Telemetry maximum sample length:',len(text))
print('PASS actual UNO route handler: stopped-only, ranges, missing/malformed fields; survey traffic bypass; serial capacity')
