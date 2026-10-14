const fs=require('fs'),vm=require('vm'),assert=require('assert'),path=require('path');
const html=fs.readFileSync(path.join(__dirname,'../ESP32_Assignment2/TrafficPage.h'),'utf8');
const js=html.split('<script>')[1].split('</script>')[0],els={};
const element=()=>({value:'',checked:false,disabled:false,hidden:false,textContent:'',children:[],dataset:{},appendChild(x){this.children.push(x)},addEventListener(){},setAttribute(k,v){this[k]=v},getContext(){return new Proxy({},{get:()=>()=>{}})},getBoundingClientRect(){return {width:320,height:240}},style:{}});
for(const m of html.matchAll(/<[^>]+\bid="([^"]+)"[^>]*>/g)){const e=els[m[1]]=element();e.value=m[0].match(/\bvalue="([^"]*)"/)?.[1]||'';e.checked=/\bchecked\b/.test(m[0]);e.disabled=/\bdisabled\b/.test(m[0]);e.hidden=/\bhidden\b/.test(m[0]);}
els.routeMode.value="2";let routeRC=0,routeMode=1,rejectRoute=false;
const state={uno:{M:0,UV:42,GY:1,GN:1,IV:0,IC:0,ZR:0,UT:100,RN:0,YA:0,PL:0,PR:0,L:100,C:100,R:100,TS:0},settings:{SC:25,BL:600,BH:1023,RA:110,SA:110,CT:1500,GS:0,CS:60,DS:30,RP:170,RS:0,applied:1}};let checkYaw=0,commands=[];const timers=new Set();
const context={document:{getElementById(id){assert(els[id],`Missing DOM ${id}`);return els[id]},createElement:element,querySelectorAll(){return []}},window:{addEventListener(){}},location:{hostname:'robot.test'},console,AbortController,URLSearchParams,Blob,Image:function(){},URL:{createObjectURL(){return 'blob:test'},revokeObjectURL(){}},setTimeout(fn,ms){const t=setTimeout(()=>{timers.delete(t);fn()},Math.min(ms,4));timers.add(t);return t},clearTimeout(t){clearTimeout(t);timers.delete(t)},fetch:async(url,opts={})=>{
 const p=new URL(url,'http://robot.test').pathname;let out={},status=200;const args=Object.fromEntries(opts.body||[]);
 if(p==='/traffic-status'){state.uno.UT+=100;out={...state,age_ms:0,esp_ms:state.uno.UT,sensors:{UT:state.uno.UT,GR10:0,RC:routeRC,RM:routeMode,RI:0,RL:0,RT:-150},sensors_age_ms:0,objects:{},calibration:{},framesize:7};}
 else if(p==='/traffic-settings'){if(opts.method==='POST'){if(+args.GS!==state.settings.GS)state.uno.IV=0;state.settings={...Object.fromEntries(Object.entries(args).map(([k,v])=>[k,+v])),applied:1};}out=state.settings;}
 else if(p==='/cmd'){commands.push(args.action);if(args.action==='stop'){state.uno.M=0;state.uno.PL=state.uno.PR=0;}
 if(args.action==='route'&&!rejectRoute){assert.equal(state.uno.M,0);routeRC=(routeRC+1)%256;routeMode=+args.value;}
 if(args.action==='gyro_navigation'){if(+args.value===3){state.uno.IC++;state.uno.IV=0;}if(+args.value<2)state.uno.GN=+args.value;state.uno.ZR++;}
 if(args.action==='gyro_check'){const d=+args.value,delta=state.uno.YA-checkYaw;if(d===0){state.uno.IV=0;checkYaw=state.uno.YA;}if(d===1&&delta>10&&delta<100){state.uno.IV=1;checkYaw=state.uno.YA;}if(d===2&&state.uno.IV===1&&delta< -10&&delta> -100){state.uno.IV=3;checkYaw=state.uno.YA;}}
 out='OK';}
 else if(p==='/survey-start'){assert.equal(+args.CS,60);assert.equal(+args.turn4,120);commands.push('survey-start');state.uno.M=6;state.uno.RN++;out='sent';}
 else if(p==='/traffic-start'){assert.equal(state.uno.IV,3);assert.equal(+args.route_rc,routeRC);assert.equal(routeRC,5);state.uno.M=6;state.uno.RN++;state.uno.TS=2;out='Start requested';}
 else if(p==='/preview')status=204;
 return {ok:true,status,text:async()=>typeof out==='string'?out:JSON.stringify(out),headers:{get(){return null}}};}};
vm.createContext(context);vm.runInContext(js,context);const run=s=>vm.runInContext(s,context);const settle=()=>new Promise(r=>setTimeout(r,30));
(async()=>{await settle();assert(els.startBtn.disabled);assert(els.readyStatus.textContent.includes('direction'));
 for(const tab of ['calibration','camera','map','car']){run(`switchTab('${tab}')`);assert(!els[tab+'Panel'].hidden);}
 await run('calibrateGyro()');assert.equal(state.uno.IC,1);assert(els.gyroNavStatus.textContent.includes('Bias calibrated'));
 await run('checkGyro(0)');state.uno.YA=30;await run('checkGyro(1)');state.uno.YA=0;await run('checkGyro(2)');await settle();assert.equal(state.uno.IV,3);assert(!els.startBtn.disabled);
 await run('setGyroNavigation(2)');assert.equal(state.uno.IV,3);
 rejectRoute=true;await assert.rejects(run('applyRouteProfile()'),/did not confirm/);assert.equal(state.uno.M,0);rejectRoute=false;
 assert(run("scanCSV({rows:[flattenRecord({source:'uno',data:{RI:2,RT:-240}})]})").includes('data.RI'));
 els.scanEnabled.checked=true;await run('startCar()');assert.equal(state.uno.M,6);assert.equal(run('scanRuns.length'),1);
 const count=commands.length;await run('calibrateGyro()');assert.equal(commands.length,count); // running calibration refused
 await run('finishScan()');assert.equal(state.uno.M,0);assert(run('scanRuns[0].status').includes('finished'));
 els.GS.value='1';await run('saveSettings()');await settle();assert.equal(state.uno.IV,0);assert(els.startBtn.disabled);
 els.turn0.value='150';assert.throws(()=>run('routeForm()'));els.turn0.value='-150';
 assert.equal(run('wrapHeading(370)'),'10.0');assert.equal(run('wrapHeading(-190)'),'170.0');
 els.routeMode.value='1';state.uno.GY=0;state.uno.GN=0;state.uno.IV=0;state.settings.applied=0;els.scanEnabled.checked=false;
 run('updateReadiness({uno:'+JSON.stringify(state.uno)+',settings:{applied:0},age_ms:0})');assert(!els.startBtn.disabled);
 const prior=commands.length;await run('startCar()');assert.equal(commands.length,prior+1);assert.equal(commands.at(-1),'survey-start');assert.equal(state.uno.M,6);
 console.log('PASS Survey direct-start with IV=0, GY=0, GN=0 and unacknowledged settings; calibration controls retained');
 console.log('PASS full-page mocked UI flow: four tabs, startup gate, bias/check/zero, run, calibration denied while moving, per-run scan finish, sign invalidates verification, normalized heading');
})().catch(e=>{console.error(e);process.exitCode=1}).finally(()=>{for(const t of timers)clearTimeout(t)});
