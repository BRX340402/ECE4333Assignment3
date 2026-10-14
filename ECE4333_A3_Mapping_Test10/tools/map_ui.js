/*MAPUI - Test10 tablet recording / live map / downloads (no PC needed)*/
let mapLines=[],mapSamples=[],mapNext=null,mapEspTotal=0,mapLost=0,mapBusyPoll=false,mapLastOk=0,mapDirty=false,mapSavedAt=0,
 mapLastBuild=0,mapBuiltCount=-1,mapRes=null,mapRunsCache=[],mapRunPick='live',mapStoreWarn='',mapDownloadedCount=0;
let mapStatus221=null;
let mapRef=MAPREF0.map(s=>Object.assign({},s));
try{const r=JSON.parse(localStorage.getItem('a3map10ref')||'null');if(Array.isArray(r)&&r.length===5)r.forEach((v,i)=>{if(v>0)mapRef[i].len=+v;});}catch(e){}
function mapAddLine(idx,esp,text,pc){
 mapLines.push(idx+'\t'+esp+'\t'+pc+'\t'+text);
 if(text.startsWith('{"N":221,')){try{mapStatus221=JSON.parse(text);}catch(e){}}
 if(text.startsWith('{"N":230,')){try{const o=JSON.parse(text);if(Array.isArray(o.d)&&o.d.length>=15){const s=mapDecode(o.d);s.idx=idx;s.esp=esp;s.pc=pc;mapSamples.push(s);}}catch(e){}}
}
function mapSave(force){
 if(!mapDirty&&!force)return;if(!force&&Date.now()-mapSavedAt<4000)return;
 try{localStorage.setItem('a3map10',JSON.stringify({v:1,next:mapNext,lost:mapLost,lines:mapLines}));mapStoreWarn='';mapDirty=false;mapSavedAt=Date.now();}
 catch(e){mapStoreWarn='Tablet storage full: download the data now (it is still kept in this page until you close it).';}
}
function mapRestore(){
 try{const o=JSON.parse(localStorage.getItem('a3map10')||'null');if(!o||!Array.isArray(o.lines))return;
  mapNext=o.next;mapLost=o.lost||0;
  for(const l of o.lines){const p=l.split('\t');if(p.length>=4)mapAddLine(+p[0],+p[1],p.slice(3).join('\t'),+p[2]);}
 }catch(e){}
}
async function mapPoll(){
 if(mapBusyPoll)return;mapBusyPoll=true;
 try{
  for(let round=0;round<8;round++){
   const txt=await request('/map-log?from='+(mapNext===null?0:mapNext)+'&max=200');
   const lines=txt.split('\n'),h={};(lines[0]||'').replace(/^#/,'').split(' ').forEach(p=>{const i=p.indexOf('=');if(i>0)h[p.slice(0,i)]=p.slice(i+1);});
   const total=+h.lines||0,oldest=+h.oldest||0;mapEspTotal=total;mapLastOk=Date.now();
   if(h.error){$('mapStatus').textContent='ESP32: '+h.error;break;}
   if(mapNext===null)mapNext=oldest;              // first visit: take everything the ESP32 still holds
   if(total<mapNext){mapNext=0;continue;}          // ESP32 restarted: its counter began again at 0
   const pc=Date.now();let got=0;
   for(const l of lines.slice(1)){if(!l)continue;const a=l.indexOf('\t'),b=l.indexOf('\t',a+1);if(a<0||b<0)continue;
    const idx=+l.slice(0,a);if(idx<mapNext)continue;if(idx>mapNext)mapLost+=idx-mapNext;
    mapAddLine(idx,+l.slice(a+1,b),l.slice(b+1),pc);mapNext=idx+1;got++;}
   if(got)mapDirty=true;
   if(mapNext>=total||!got)break;
  }
 }catch(e){}
 finally{mapBusyPoll=false;}
 mapSave(false);mapRefresh(false);
}
function mapGyroReadout(){
 const s=mapSamples[mapSamples.length-1],u=mapStatus221;if(!s)return;
 const y=s.yaw,h=((y%360)+360)%360;
 $('gyroYaw').textContent=y.toFixed(1)+' °';
 $('gyroDetail').textContent='heading '+h.toFixed(0)+'° (0-360) · rate '+s.g.toFixed(1)+' °/s'+(u&&u.CB?' · CALIBRATING, keep still':'')+' · updated '+Math.max(0,(Date.now()-s.pc)/1000).toFixed(1)+' s ago';
 $('gyroNeedle').setAttribute('transform','rotate('+(-y).toFixed(1)+')');
}
async function mapZeroGyro(){
 const b=$('gyroZeroBtn'),zr0=mapStatus221?mapStatus221.ZR:null;b.disabled=true;
 $('gyroMsg').textContent='Keep the car completely still for ~2 s… 请保持小车静止';
 try{await post('/cmd',{action:'gyro_navigation',value:2});}
 catch(e){$('gyroMsg').textContent='Not accepted: '+(e.message||e)+' — press STOP CAR first, then zero. 先停车再归零';b.disabled=false;return;}
 const t0=Date.now();
 while(Date.now()-t0<6000){await delay(250);const u=mapStatus221;
  if(u&&zr0!==null&&u.ZR!==zr0&&!u.CB){$('gyroMsg').textContent='Zeroed ✓ (bias re-measured). Now turn the car LEFT by hand: yaw must increase. 归零完成';b.disabled=false;mapGyroReadout();return;}}
 $('gyroMsg').textContent='No confirmation from the car within 6 s — stop the car and try again. 未收到确认，请停车后重试';b.disabled=false;
}
function mapRunLabel(r,i){const d=((r[r.length-1].t-r[0].t)/1000).toFixed(0);return 'Run '+(i+1)+' - '+r.length+' samples, '+d+' s';}
function mapRefresh(force){
 mapGyroReadout();
 const age=mapLastOk?((Date.now()-mapLastOk)/1000).toFixed(0):'-',live=mapSamples[mapSamples.length-1];
 const st=['IDLE','FOLLOW','CROSS','TURN','SEARCH','FAIL','RED STOP'];
 $('mapStatus').textContent='Recorded '+mapLines.length+' lines / '+mapSamples.length+' samples on this tablet'+
  (mapLost?' | LOST '+mapLost+' (offline > ~3 min)':' | nothing lost')+' | last contact '+age+' s ago'+
  (live?' | car: '+(live.mode===6?'AUTO '+(st[live.fs]||live.fs):live.mode===5?'MANUAL':'stopped')+', yaw '+live.yaw.toFixed(0)+' deg, L/C/R '+live.L+'/'+live.C+'/'+live.R:'')+
  (mapLines.length>mapDownloadedCount?' | NOT downloaded yet':' | downloaded');
 if(mapStoreWarn)$('mapStatus').textContent+=' | '+mapStoreWarn;
 const u=mapStatus221;let g='Gyro: waiting for UNO status';
 if(u){const chip=u.IM===1?'MPU6050 / GY-521 (WHO_AM_I '+u.IW+')':u.IM===2?'QMI8658':'NO CHIP FOUND';
  g=u.GY?'Gyro OK ('+chip+')'+(u.CB?' — calibrating, keep still':'')+(u.GF?' — GYRO DIRECTION WAS REVERSED and was auto-corrected (until power-off). In settings set Gyro Z sign GS = '+(u.GL>0?1:0)+' and Save. 陀螺仪方向已自动纠正，请在设置中把 GS 改为 '+(u.GL>0?1:0):''):
   'GYRO NOT WORKING ('+chip+', IW='+u.IW+', IQ='+u.IQ+', bus resets '+(u.IB??0)+') — Start is blocked. '+
   (u.IW===255?'Nothing answers on I2C: check the GY-521 board is pushed fully into its header, then switch power OFF and ON with the car still.':'Switch power OFF and ON with the car still; see README.');}
 $('mapGyro').textContent=g;$('mapGyro').style.color=u&&(!u.GY||u.GF)?'#ff6b6b':'';
 if(!force&&(mapSamples.length===mapBuiltCount||Date.now()-mapLastBuild<1500))return;
 mapLastBuild=Date.now();mapBuiltCount=mapSamples.length;
 mapRunsCache=mapRuns(mapSamples);
 const sel=$('mapRun'),keep=mapRunPick;sel.innerHTML='';
 const o0=document.createElement('option');o0.value='live';o0.textContent='Latest run (live)';sel.appendChild(o0);
 mapRunsCache.forEach((r,i)=>{const o=document.createElement('option');o.value=String(i);o.textContent=mapRunLabel(r,i);sel.appendChild(o);});
 sel.value=keep==='live'||+keep<mapRunsCache.length?keep:'live';
 if(!mapRunsCache.length){$('mapView').innerHTML='<p class="hint">No driving data yet. Press Start continuous run; the map appears here while the car drives.</p>';$('mapSummary').textContent='';$('mapSegTable').innerHTML='';mapRes=null;return;}
 const idx=sel.value==='live'?mapRunsCache.length-1:+sel.value,run=mapRunsCache[idx];
 mapRes=mapBuildRun(run,mapRef);mapRes.no=idx+1;
 $('mapView').innerHTML=mapSVG(mapRes,mapRef,'Run '+(idx+1)).replace('<svg ','<svg style="width:100%;height:auto;border-radius:8px" ');
 $('mapSummary').textContent=mapSummary(mapRes,mapRef,idx+1,mapRunsCache.length);
 const rows=mapSegRows(mapRes,mapRef);
 $('mapSegTable').innerHTML=rows.length?'<tr><th>#</th><th>Segment</th><th>Complete</th><th>Heading ref/meas</th><th>Length ref/meas (cm)</th></tr>'+
  rows.map(r=>'<tr><td>'+r.order+'</td><td>'+r.segment+'</td><td>'+(r.complete?'yes':'—')+'</td><td>'+r.ref_heading_deg+' / '+r.meas_heading_deg+'</td><td>'+r.ref_length_cm+' / '+r.meas_length_cm+'</td></tr>').join(''):'';
}
function mapPickRun(v){mapRunPick=v;mapRefresh(true);}
function mapStamp(){const d=new Date(),p=n=>String(n).padStart(2,'0');return d.getFullYear()+p(d.getMonth()+1)+p(d.getDate())+'_'+p(d.getHours())+p(d.getMinutes())+p(d.getSeconds());}
function mapSaveFile(name,text,type){
 const blob=new Blob([text],{type:type||'text/plain'}),url=URL.createObjectURL(blob),a=document.createElement('a');
 a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),60000);
 $('message').textContent='Saved '+name+' ('+Math.round(text.length/1024)+' kB). If nothing appeared, check the browser Downloads list.';
}
function mapDownload(kind){
 const st=mapStamp();
 if(kind==='csv'||kind==='raw'){
  if(!mapLines.length){$('message').textContent='Nothing recorded yet.';return;}
  if(kind==='csv'){
   const cols=['tablet_ms','esp_idx','esp_ms','q','t_ms','L','C','R','PL','PR','gyro_rate_dps','yaw_deg','dist_cm','x_cm','y_cm','ultrasonic_cm','mode','follow_state','gyro_ok','L_seen','C_seen','R_seen','all_black','ev_cross','ev_turn','ev_search','ev_estop'];
   const csv=cols.join(',')+'\n'+mapSamples.map(s=>[s.pc,s.idx,s.esp,s.q,s.t,s.L,s.C,s.R,s.PL,s.PR,s.g,s.yaw,s.dist,s.x,s.y,s.u,s.mode,s.fs,s.gy].concat([0,1,2,3,4,5,6,7].map(b=>(s.k>>b)&1)).join(',')).join('\n')+'\n';
   mapSaveFile('map_samples_'+st+'.csv',csv,'text/csv');
  }else mapSaveFile('map_all_lines_'+st+'.txt','esp_idx\tesp_ms\ttablet_ms\tuno_line\n'+mapLines.join('\n')+'\n');
  mapDownloadedCount=mapLines.length;mapRefresh(false);return;
 }
 if(!mapRes){$('message').textContent='No driving run to export yet.';return;}
 const tag='run'+mapRes.no+'_'+st;
 if(kind==='svg')mapSaveFile('map_'+tag+'.svg',mapSVG(mapRes,mapRef,'Run '+mapRes.no),'image/svg+xml');
 if(kind==='seg')mapSaveFile('map_segments_'+tag+'.csv',mapCSV(mapSegRows(mapRes,mapRef)),'text/csv');
 if(kind==='points')mapSaveFile('map_points_'+tag+'.csv',mapCSV(mapPointRows(mapRes,mapRef)),'text/csv');
 if(kind==='summary')mapSaveFile('map_summary_'+tag+'.txt',$('mapSummary').textContent+'\n');
 if(kind==='png'){
  const svg=mapSVG(mapRes,mapRef,'Run '+mapRes.no),img=new Image(),url=URL.createObjectURL(new Blob([svg],{type:'image/svg+xml'}));
  img.onload=()=>{try{const c=document.createElement('canvas');c.width=img.width*2;c.height=img.height*2;const g=c.getContext('2d');g.scale(2,2);g.drawImage(img,0,0);URL.revokeObjectURL(url);
   c.toBlob(b=>{if(!b){$('message').textContent='PNG export not supported here: use Map SVG.';return;}const u=URL.createObjectURL(b),a=document.createElement('a');a.href=u;a.download='map_'+tag+'.png';document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(u),60000);$('message').textContent='Saved map_'+tag+'.png';},'image/png');}
   catch(e){$('message').textContent='PNG export failed on this browser: use Map SVG.';}};
  img.onerror=()=>{$('message').textContent='PNG export failed on this browser: use Map SVG.';};img.src=url;
 }
}
function mapClear(){
 if(mapLines.length>mapDownloadedCount&&!confirm('Recording has NOT been downloaded. Delete it from this tablet anyway?'))return;
 if(!confirm('Delete all recorded data from this tablet? (The car itself is not affected.)'))return;
 mapLines=[];mapSamples=[];mapLost=0;mapNext=mapEspTotal;mapDownloadedCount=0;mapRes=null;
 try{localStorage.removeItem('a3map10');}catch(e){}
 mapBuiltCount=-1;mapRefresh(true);
}
function mapLoadRefInputs(){mapRef.forEach((s,i)=>{$('mapLen'+i).value=s.len;});}
function mapSaveRef(){
 const v=mapRef.map((s,i)=>+$('mapLen'+i).value);
 if(v.some(x=>!(x>0&&x<2000))){$('message').textContent='Enter lengths in cm (1-2000).';return;}
 v.forEach((x,i)=>mapRef[i].len=x);try{localStorage.setItem('a3map10ref',JSON.stringify(v));}catch(e){}
 $('message').textContent='Track lengths saved on this tablet.';mapRefresh(true);
}
function mapResetRef(){mapRef=MAPREF0.map(s=>Object.assign({},s));try{localStorage.removeItem('a3map10ref');}catch(e){}mapLoadRefInputs();mapRefresh(true);}
window.addEventListener('beforeunload',e=>{mapSave(true);if(mapLines.length>mapDownloadedCount){e.preventDefault();e.returnValue='';}});
document.addEventListener('visibilitychange',()=>{if(document.hidden)mapSave(true);else mapPoll();});
mapRestore();mapLoadRefInputs();mapRefresh(true);setInterval(mapPoll,500);
/*END MAPUI*/
