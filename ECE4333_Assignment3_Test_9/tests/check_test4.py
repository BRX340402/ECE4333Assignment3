from pathlib import Path
import subprocess,tempfile,gzip,re
root=Path(__file__).resolve().parents[1]
page=(root/'ESP32_Assignment2/TrafficPage.h').read_text().split('R"HTML(',1)[1].split(')HTML";',1)[0]
packed=bytes(int(v,16) for v in re.findall(r'0x([0-9a-f]{2})',(root/'ESP32_Assignment2/TrafficPageGzip.h').read_text()))
assert gzip.decompress(packed).decode()==page
uno=(root/'UNO_Assignment2/UNO_Assignment2.ino').read_text()
motor=uno[uno.index('void motorOutput('):uno.index('void standby(')]
code='''#include <cassert>
#include <cstdlib>
#include "TeamControl.h"
bool eStopLatched=false;int leftPWM=0,rightPWM=0;
const int DRIVE_LIMIT=180,MOTOR_STBY=0,MOTOR_LEFT_DIR=1,MOTOR_RIGHT_DIR=2,MOTOR_LEFT_PWM=3,MOTOR_RIGHT_PWM=4,HIGH=1,LOW=0;
int pin[5]={};void digitalWrite(int p,int v){pin[p]=v;}void analogWrite(int p,int v){pin[p]=v;}
int constrain(int x,int a,int b){return x<a?a:(x>b?b:x);}
'''+motor+'''
int main(){
 motorOutput(50,50);assert(leftPWM==50&&rightPWM==50);
 motorOutput(-30,-30);assert(leftPWM==-30&&rightPWM==-30);
 eStopLatched=true;motorOutput(100,100);assert(!leftPWM&&!rightPWM&&!pin[0]);
 TeamControl c;int l=0,r=0;
 assert(c.line(100,700,100,600,850,50,100,0,l,r)&&l==50&&r==50);
 assert(c.line(700,700,100,600,850,50,120,0,l,r)&&l<r);
 assert(c.line(100,700,700,600,850,50,140,0,l,r)&&l>r);
 assert(!c.line(100,100,100,600,850,50,160,0,l,r));
 assert(c.line(700,100,100,600,850,0,180,0,l,r)&&!l&&!r);
 TeamControl::heading(10,0,0,50,l,r);assert(l<r&&l>=0&&r<=50);
}
'''
js=page.split('<script>',1)[1].split('</script>',1)[0]
log=js.split('// SCAN_LOG_BEGIN',1)[1].split('// SCAN_LOG_END',1)[0]
checks=r'''const assert=require('assert');
const els={};function element(){return {value:'0',textContent:'',children:[],appendChild(v){this.children.push(v);}};}
const $=id=>els[id]||(els[id]=element());const document={createElement:element};const window={addEventListener(){}};
let driving=true;const delay=async()=>{};
let failStop=false;async function post(){if(failStop)throw Error('offline');}
async function request(){return JSON.stringify(sample(5000,1,0));}
'''+log+r'''
function sample(ut,rn=1,m=6,age=0){return {uno:{UT:ut,RN:rn,M:m,TS:2,PL:0,PR:0},age_ms:age,sensors:{UT:ut,U:42,V:7400,GR10:35},sensors_age_ms:0,objects:{red:{pixels:170}}};}
(async()=>{
 const first=newScan();recordSample(sample(1000));recordSample(sample(1000));
 assert.equal(first.rows[0].uno_new,1);assert.equal(first.rows[1].uno_new,0);
 assert.equal(first.rows[0]['sensors.U'],42);assert.equal(first.rows[0]['objects.red.pixels'],170);
 recordSample(sample(1500,1,6,2000));assert.equal(first.rows[2].uno_fresh,0);
 scanGap('lost,"frame"');assert.equal(first.rows[3].event,'connection_gap');assert(!('uno.UT' in first.rows[3]));
 recordSample(sample(100,0));assert.equal(first.segment,2);
 assert(scanCSV(first).includes('"lost,""frame"""'));
 failStop=true;await finishScan();assert.equal(activeScan,first);assert(!first.status.includes('finished'));
 failStop=false;await finishScan();assert.equal(activeScan,null);assert(first.status.includes('finished'));
 const second=newScan();recordSample(sample(6000,2));assert.notEqual(first.id,second.id);assert.equal(second.rows.length,1);assert(first.rows.length>1);
 second.rows=Array(40000).fill({});recordSample(sample(6500,2));assert.equal(activeScan,null);assert(second.status.includes('incomplete'));
 console.log('PASS scan tables: separate runs, all-field flattening, duplicate/stale flags, gaps, CSV escaping, stop confirmation, capacity.');
})().catch(e=>{console.error(e);process.exit(1);});
'''
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'test.cpp').write_text(code);subprocess.run(['g++','-std=c++11','-I'+str(root/'UNO_Assignment2'),str(p/'test.cpp'),'-o',str(p/'test')],check=True);subprocess.run([str(p/'test')],check=True)
 (p/'page.js').write_text(js);subprocess.run(['node','--check',str(p/'page.js')],check=True)
 (p/'log.cjs').write_text(checks);subprocess.run(['node',str(p/'log.cjs')],check=True)
print('PASS gzip roundtrip, page syntax, actual motor-output no trim/E-stop gate, PD direction/zero/lost-line/heading.')
