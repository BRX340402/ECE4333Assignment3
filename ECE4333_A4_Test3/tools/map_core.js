/*MAPCORE - mapping algorithm of PC_Mapping/draw_map.py (tested by tests/test_map_core.js) + Test11 predicted/fused tracks, heading chart, target-following runs*/
const MAPREF0=[{name:'C-O-D',h:0,len:320},{name:'D-E',h:-136.6,len:151.3},{name:'E-O-A',h:-240,len:220},{name:'A-B',h:-210,len:43.3},{name:'B-C',h:-120,len:125}];
const MAPCOL={1:'#2a7de1',2:'#f08c00',3:'#d6336c',4:'#ae3ec9',5:'#000000',6:'#ff8fab'};
function mapDecode(d){const st=d[13],k=d[14],m=st>>4,o={q:d[0],t:d[1],L:d[2],C:d[3],R:d[4],PL:d[5],PR:d[6],g:d[7]/10,yaw:d[8]/10,dist:d[9]/10,x:d[10],y:d[11],u:d[12],mode:m,fs:st&7,gy:(st>>3)&1,k:k};
 if(d.length>=18){o.kh=d[15]/10;o.kx=d[16];o.ky=d[17];}                     // predicted pose (wheel model only), UV>=41
 if(m===7){o.tgtErr=d[2]/10;o.tgtW=d[3];o.tgtBear=d[4]/10;o.L=o.C=o.R=null;} // mode 7 reuses the L,C,R slots
 return o;}
function mapRuns(S){const runs=[];let cur=[],p=null;
 for(const r of S){if(r.mode!==6&&r.mode!==7){if(cur.length)runs.push(cur);cur=[];p=null;continue;}
  if(p&&(r.mode!==p.mode||r.q<p.q||r.t<p.t||r.t-p.t>3000)){runs.push(cur);cur=[];}cur.push(r);p=r;}
 if(cur.length)runs.push(cur);const out=runs.filter(r=>r.length>=5);out.forEach(r=>{r.kind=r[0].mode;});return out;}
function mapStraight(run,yaw,dist){
 const fwd=run.map(r=>r.PL>0&&r.PR>0&&(r.fs===1||r.fs===2)),out=[],N=run.length;let i=0;
 while(i<N){if(!fwd[i]){i++;continue;}let j=i,sum=yaw[i],c=1;
  while(j+1<N&&fwd[j+1]&&Math.abs(yaw[j+1]-sum/c)<=8){j++;sum+=yaw[j];c++;}
  if(j-i+1>=6)out.push({i0:i,i1:j,head:sum/c,len:dist[j]-dist[i]});i=j+1;}
 // drop short transitional runs (heading still settling at the end of a pivot)
 const maxLen=out.reduce((a,r)=>Math.max(a,r.len),0),minLen=Math.max(12,0.06*maxLen);
 for(let i=out.length-1;i>=0;i--)if(out[i].len<minLen)out.splice(i,1);
 const m=[];for(const r of out){const L=m[m.length-1];
  if(L&&Math.abs(L.head-r.head)<12){const w0=L.i1-L.i0+1,w1=r.i1-r.i0+1;L.head=(L.head*w0+r.head*w1)/(w0+w1);L.i1=r.i1;L.len=dist[L.i1]-dist[L.i0];}
  else m.push(Object.assign({},r));}
 return m;}
function mapLabel(runs,ref){const H=ref.map(s=>s.h),n=H.length,turn=H.map((h,l)=>l+1<n?H[l+1]-h:H[0]-h);
 runs.forEach((r,k)=>{let lab=null;
  if(k===0)lab=Math.abs(r.head-H[0])<30?0:null;
  else{const p=runs[k-1],d=r.head-p.head;
   if(p.label!==null&&Math.abs(d-turn[p.label])<=25)lab=(p.label+1)%n;
   else{let off=0;for(let q=k-1;q>=0;q--)if(runs[q].label!==null){off=runs[q].head-H[runs[q].label];break;}
    let best=1e9,bi=null;H.forEach((h,i)=>{const e=Math.abs(r.head-off-h);if(e<best){best=e;bi=i;}});lab=best<12?bi:null;r.resync=true;}}
  r.label=lab;});
 return runs;}
function mapSolve3(A,b){const M=A.map((r,i)=>r.concat([b[i]]));
 for(let c=0;c<3;c++){let p=c;for(let r=c+1;r<3;r++)if(Math.abs(M[r][c])>Math.abs(M[p][c]))p=r;
  if(Math.abs(M[p][c])<1e-12)return null;[M[c],M[p]]=[M[p],M[c]];
  for(let r=0;r<3;r++)if(r!==c){const f=M[r][c]/M[c][c];for(let j=0;j<4;j++)M[r][j]-=f*M[c][j];}}
 return[0,1,2].map(i=>M[i][3]/M[i][i]);}
function mapBuildRun(run,ref,fixedScale){
 const n=ref.length,N=run.length,t=run.map(r=>(r.t-run[0].t)/1000),yaw=run.map(r=>r.yaw-run[0].yaw),dist=run.map(r=>r.dist-run[0].dist);
 let gaps=0;for(let i=1;i<N;i++)gaps+=Math.max(0,(run[i].q-run[i-1].q-1)&0xFFFF);
 const notes=[];
 const segs=mapLabel(mapStraight(run,yaw,dist),ref).filter(g=>g.label!==null);
 const labels=new Array(N).fill(null);segs.forEach(g=>{for(let i=g.i0;i<=g.i1;i++)labels[i]=g.label;});
 const nx=l=>(l+1)%n;
 segs.forEach((g,k)=>{const prevOk=k>0&&nx(segs[k-1].label)===g.label&&!g.resync,nextOk=k+1<segs.length&&nx(g.label)===segs[k+1].label&&!segs[k+1].resync;
  g.complete=(prevOk||(k===0&&g.label===0))&&nextOk;g.inOrder=prevOk||nextOk||k===0;});
 const comp=segs.filter(g=>g.complete&&g.len>0);let scale,how;
 const lapEnds=[];for(let k=1;k<segs.length;k++)if(segs[k].label===0&&segs[k-1].label===n-1)lapEnds.push(segs[k].i0);
 const lapRef=ref.reduce((a,s)=>a+s.len,0),lapMeas=lapEnds.length?dist[lapEnds[lapEnds.length-1]]-dist[0]:0;
 if(fixedScale){scale=fixedScale;how='fixed';}
 else if(lapEnds.length&&lapMeas>0){scale=lapEnds.length*lapRef/lapMeas;how='auto from '+lapEnds.length+' complete lap(s) ('+(lapEnds.length*lapRef).toFixed(0)+' cm)';}
 else if(comp.length){scale=comp.reduce((a,g)=>a+ref[g.label].len,0)/comp.reduce((a,g)=>a+g.len,0);how='auto from '+comp.length+' complete segment(s)';}
 else{scale=1;how='NOT calibrated yet (needs a straight bounded by two corners) - UNO model units';notes.push('No complete segment yet: drive at least from C past corner E.');}
 const A=[[0,0,0],[0,0,0],[0,0,0]],bb=[0,0,0];let used=0;
 segs.forEach(g=>{if(!g.inOrder)return;const h=ref[g.label].h;
  for(let i=g.i0;i<=g.i1;i++){if(run[i].fs!==1)continue;const v=[1,t[i],yaw[i]],e=yaw[i]-h;
   for(let p=0;p<3;p++){bb[p]+=v[p]*e;for(let q=0;q<3;q++)A[p][q]+=v[p]*v[q];}used++;}});
 const fit=used>30&&new Set(segs.map(g=>g.label)).size>=3?mapSolve3(A,bb):null;let yawc,gyroTxt;
 if(fit){yawc=yaw.map((v,i)=>(v-fit[0]-fit[1]*t[i])/(1+fit[2]));gyroTxt='gyro correction: offset '+fit[0].toFixed(2)+' deg, drift '+fit[1].toFixed(3)+' deg/s, scale error '+(fit[2]*100).toFixed(2)+' %';}
 else{yawc=yaw.slice();gyroTxt='gyro correction: not fitted yet (needs 3 different straight segments)';}
 const xs=[0],ys=[0];
 for(let i=1;i<N;i++){const ds=(dist[i]-dist[i-1])*scale,h=(yawc[i]+yawc[i-1])/2*Math.PI/180;xs.push(xs[i-1]+ds*Math.cos(h));ys.push(ys[i-1]+ds*Math.sin(h));}
 const closure=[];let s0=0;
 for(const e of lapEnds){const ex=xs[e],ey=ys[e];closure.push(Math.hypot(ex,ey));const d0=dist[s0],d1=dist[e],dd=d1>d0?d1-d0:1;
  for(let i=s0;i<N;i++){const f=i>=e?1:(dist[i]-d0)/dd;xs[i]-=ex*f;ys[i]-=ey*f;}s0=e;}
 const cnt=b=>run.reduce((a,r)=>a+((r.k>>b)&1),0);
 const yawMoved=run.some(r=>r.yaw!==0),gyOk=run.filter(r=>r.gy).length;
 if(!yawMoved)notes.push('GYRO NOT WORKING: heading stayed exactly 0 for the whole run, so no map can be built and the car cannot recognise corners or the old track. Check the IMU line on this page (IM=0 means no chip found).');
 else if(gyOk===0&&run.some(r=>r.k!==undefined))notes.push('Samples do not carry the gyro flag (older UNO firmware).');
 if(segs.some(g=>!g.inOrder))notes.push('Segments marked (?) are out of the expected C>D>E>A>B order: the car took a wrong branch or reversed there.');
 return mapAddTracks({run,t,yaw,yawc,dist,xs,ys,segs,labels,scale,how,gyroTxt,laps:lapEnds.length,closure,lapEnds,notes,gaps,
  counts:{cross:cnt(4),turn:cnt(5),search:cnt(6),allBlack:cnt(3)}},ref);}

/* ---- Test11: predicted vs fused tracks ----
   fused     = gyro heading + wheel-speed model, exactly as the UNO logged it (x,y in cm, unscaled)
   predicted = wheel-speed model only (kinematics: v=(vl+vr)/2, w=(vr-vl)/b, Euler integration on the UNO), no gyro
   mapped    = fused + this page's post-processing (gyro offset/drift fit, distance scale, lap closure) */
function mapDistSeg(px,py,a,b){const dx=b[0]-a[0],dy=b[1]-a[1],l2=dx*dx+dy*dy;let f=l2?((px-a[0])*dx+(py-a[1])*dy)/l2:0;f=Math.max(0,Math.min(1,f));return Math.hypot(px-a[0]-f*dx,py-a[1]-f*dy);}
function mapDistPoly(px,py,poly){let m=1e9;for(let i=1;i<poly.length;i++){const d=mapDistSeg(px,py,poly[i-1],poly[i]);if(d<m)m=d;}return m;}
function mapXtrack(xs,ys,poly){let s=0,q=0,mx=0;const n=xs.length;for(let i=0;i<n;i++){const d=mapDistPoly(xs[i],ys[i],poly);s+=d;q+=d*d;if(d>mx)mx=d;}return{mean:s/n,rms:Math.sqrt(q/n),max:mx};}
function mapUnwrap(deg){const o=[0];for(let i=1;i<deg.length;i++){let d=deg[i]-deg[i-1];d=((d+540)%360+360)%360-180;o.push(o[i-1]+d);}return o;}
function mapAddTracks(res,ref){
 const run=res.run,N=run.length,poly=mapRefPoly(ref);
 res.fx=run.map(r=>r.x-run[0].x);res.fy=run.map(r=>r.y-run[0].y);
 res.hasPred=run.every(r=>r.kh!==undefined);
 if(res.hasPred){res.kyaw=mapUnwrap(run.map(r=>r.kh));res.px=run.map(r=>r.kx-run[0].kx);res.py=run.map(r=>r.ky-run[0].ky);}
 res.err={fused:mapXtrack(res.fx,res.fy,poly),mapped:mapXtrack(res.xs,res.ys,poly)};
 if(res.hasPred)res.err.pred=mapXtrack(res.px,res.py,poly);
 const hrms=a=>{let q=0,n=0;res.labels.forEach((l,i)=>{if(l===null)return;const e=a[i]-ref[l].h;q+=e*e;n++;});return n?Math.sqrt(q/n):null;};
 res.hdg={fused:hrms(res.yaw),pred:res.hasPred?hrms(res.kyaw):null};
 res.hdgEnd=res.hasPred?res.kyaw[N-1]-res.yaw[N-1]:null;
 res.hdgRms=res.hasPred?Math.sqrt(res.yaw.reduce((a,v,i)=>a+(res.kyaw[i]-v)*(res.kyaw[i]-v),0)/N):null;
 res.closureRaw=res.lapEnds.map(e=>({fused:Math.hypot(res.fx[e],res.fy[e]),pred:res.hasPred?Math.hypot(res.px[e],res.py[e]):null}));
 return res;}
function mapNum(v,d){return v===null||v===undefined||!isFinite(v)?'n/a':v.toFixed(d===undefined?1:d);}
function mapTrackLines(res){const L=[],e=res.err,f=k=>e[k]?'mean '+mapNum(e[k].mean)+', rms '+mapNum(e[k].rms)+', max '+mapNum(e[k].max)+' cm':'n/a (UNO older than Test11: no predicted pose logged)';
 L.push('track error = distance to the drawn centre line (grey):');
 L.push('  predicted (wheel model only): '+f('pred'));L.push('  fused (gyro + wheel model)  : '+f('fused'));L.push('  mapped (corrected + scaled) : '+f('mapped'));
 L.push('heading error on the straights vs the drawing (rms): predicted '+mapNum(res.hdg.pred)+' deg, fused/gyro '+mapNum(res.hdg.fused)+' deg');
 if(res.hasPred)L.push('predicted heading minus gyro heading: at the end '+mapNum(res.hdgEnd)+' deg, rms over the run '+mapNum(res.hdgRms)+' deg');
 if(res.closureRaw.length)L.push('lap closure (end of lap vs start, raw): '+res.closureRaw.map((c,i)=>'lap '+(i+1)+' fused '+mapNum(c.fused)+' cm'+(c.pred===null?'':', predicted '+mapNum(c.pred)+' cm')).join('; '));
 return L;}
/* generic line chart (SVG string). o={title,xl,yl,series:[{name,color,x,y,dash,w}],bars:[{x0,x1,y,color}],zero:true} */
function mapChartSVG(o){
 const W=900,H=300,ml=58,mr=14,mt=34,mb=44,xs=[],ys=[];
 o.series.forEach(s=>{s.x.forEach(v=>xs.push(v));s.y.forEach(v=>{if(isFinite(v))ys.push(v);});});(o.bars||[]).forEach(b=>{xs.push(b.x0,b.x1);ys.push(b.y);});
 if(!xs.length)return '';
 let x0=Math.min(...xs),x1=Math.max(...xs),y0=Math.min(...ys),y1=Math.max(...ys);if(o.zero){y0=Math.min(y0,0);y1=Math.max(y1,0);}
 if(x1-x0<1e-9)x1=x0+1;if(y1-y0<1e-9){y1=y0+1;}const pad=(y1-y0)*0.06;y0-=pad;y1+=pad;
 const step=v=>{const r=v/6,p=Math.pow(10,Math.floor(Math.log10(r))),f=r/p;return(f<1.5?1:f<3?2:f<7?5:10)*p;};
 const sx=step(x1-x0),sy=step(y1-y0),X=v=>(ml+(v-x0)/(x1-x0)*(W-ml-mr)).toFixed(1),Y=v=>(H-mb-(v-y0)/(y1-y0)*(H-mt-mb)).toFixed(1),out=[];
 out.push('<svg xmlns="http://www.w3.org/2000/svg" width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" font-family="sans-serif"><rect width="100%" height="100%" fill="#ffffff"/>');
 out.push('<text x="'+ml+'" y="18" font-size="15" fill="#222">'+o.title+'</text>');
 for(let g=Math.ceil(x0/sx)*sx;g<=x1+1e-9;g+=sx)out.push('<line x1="'+X(g)+'" y1="'+mt+'" x2="'+X(g)+'" y2="'+(H-mb)+'" stroke="#eee"/><text x="'+X(g)+'" y="'+(H-mb+16)+'" font-size="11" text-anchor="middle" fill="#555">'+(+g.toFixed(6))+'</text>');
 for(let g=Math.ceil(y0/sy)*sy;g<=y1+1e-9;g+=sy)out.push('<line x1="'+ml+'" y1="'+Y(g)+'" x2="'+(W-mr)+'" y2="'+Y(g)+'" stroke="'+(Math.abs(g)<1e-9?'#bbb':'#eee')+'"/><text x="'+(ml-6)+'" y="'+(+Y(g)+4)+'" font-size="11" text-anchor="end" fill="#555">'+(+g.toFixed(6))+'</text>');
 out.push('<text x="'+((ml+W-mr)/2)+'" y="'+(H-8)+'" font-size="12" text-anchor="middle" fill="#333">'+o.xl+'</text><text transform="translate(14 '+((mt+H-mb)/2)+') rotate(-90)" font-size="12" text-anchor="middle" fill="#333">'+o.yl+'</text>');
 (o.bars||[]).forEach(b=>out.push('<line x1="'+X(b.x0)+'" y1="'+Y(b.y)+'" x2="'+X(b.x1)+'" y2="'+Y(b.y)+'" stroke="'+b.color+'" stroke-width="7" stroke-opacity="0.35"/>'));
 o.series.forEach(s=>{const pts=[];s.x.forEach((v,i)=>{if(isFinite(s.y[i]))pts.push(X(v)+','+Y(s.y[i]));});
  out.push('<polyline fill="none" stroke="'+s.color+'" stroke-width="'+(s.w||2)+'"'+(s.dash?' stroke-dasharray="'+s.dash+'"':'')+' stroke-linejoin="round" points="'+pts.join(' ')+'"/>');});
 let lx=ml+4;const ly=H-mb-6;
 o.series.concat((o.bars||[]).length?[{name:o.barName||'drawing',color:o.bars[0].color,w:7}]:[]).forEach(s=>{out.push('<rect x="'+lx+'" y="'+(ly-9)+'" width="16" height="4" fill="'+s.color+'"/><text x="'+(lx+21)+'" y="'+(ly-4)+'" font-size="12" fill="#222">'+s.name+'</text>');lx+=24+s.name.length*7;});
 out.push('</svg>');return out.join('\n');}
function mapHeadingSVG(res,ref){
 const bars=res.segs.map(g=>({x0:res.t[g.i0],x1:res.t[g.i1],y:ref[g.label].h,color:'#868e96'})),S=[];
 if(res.hasPred)S.push({name:'predicted (wheel model)',color:'#8d6e63',x:res.t,y:res.kyaw,dash:'6 4',w:2});
 S.push({name:'gyro / fused',color:'#2f9e44',x:res.t,y:res.yaw,w:2});S.push({name:'mapped (corrected)',color:'#2a7de1',x:res.t,y:res.yawc,w:1.2});
 return mapChartSVG({title:'Heading vs time (deg, + = left, 0 = along C-O-D)',xl:'time (s)',yl:'heading (deg)',series:S,bars:bars,barName:'drawing heading',zero:true});}
function mapRefPoly(ref){const p=[[0,0]];let x=0,y=0;for(const s of ref){const h=s.h*Math.PI/180;x+=s.len*Math.cos(h);y+=s.len*Math.sin(h);p.push([x,y]);}return p;}
function mapSummary(res,ref,runNo,runCount){const L=[];
 L.push('Test11 map summary (run '+runNo+' of '+runCount+', '+res.run.length+' samples, '+res.t[res.t.length-1].toFixed(1)+' s, UNO sample gaps '+res.gaps+')');
 L.push('distance scale: x'+res.scale.toFixed(3)+' ('+res.how+')');
 L.push('  -> measured speed factor ~ '+(0.33*res.scale).toFixed(3)+' cm/s per PWM (UNO uses 0.33; MapConfig.h LEFT/RIGHT_CM_S_PER_PWM)');
 L.push(res.gyroTxt);
 L.push('laps completed: '+res.laps+(res.closure.length?'; closure error before correction: '+res.closure.map(e=>e.toFixed(1)+' cm').join(', '):''));
 L.push('segment order seen: '+(res.segs.map(g=>ref[g.label].name+(g.inOrder?'':'(?)')).join(' > ')||'(none yet)'));
 L.push('crossing holds: '+res.counts.cross+', corner turns: '+res.counts.turn+', searches: '+res.counts.search+', all-black hits: '+res.counts.allBlack);
 mapTrackLines(res).forEach(x=>L.push(x));res.notes.forEach(s=>L.push('NOTE: '+s));return L.join('\n');}
function mapSegRows(res,ref){return res.segs.map((g,k)=>{const s=ref[g.label];let hs=0;for(let i=g.i0;i<=g.i1;i++)hs+=res.yawc[i];const len=g.len*res.scale;
 return{order:k+1,segment:s.name,complete:g.complete?1:0,ref_heading_deg:s.h,meas_heading_deg:+(hs/(g.i1-g.i0+1)).toFixed(1),ref_length_cm:s.len,meas_length_cm:+len.toFixed(1),
  length_error_cm:g.complete?+(len-s.len).toFixed(1):'',t_start_s:+res.t[g.i0].toFixed(1),t_end_s:+res.t[g.i1].toFixed(1)};});}
function mapCSV(rows){if(!rows.length)return'';const k=Object.keys(rows[0]);return k.join(',')+'\n'+rows.map(r=>k.map(c=>r[c]===undefined||r[c]===null?'':r[c]).join(',')).join('\n')+'\n';}
function mapPointRows(res,ref){return res.run.map((r,i)=>({t_s:+res.t[i].toFixed(2),s_cm:+(res.dist[i]*res.scale).toFixed(1),x_cm:+res.xs[i].toFixed(1),y_cm:+res.ys[i].toFixed(1),
 heading_deg:+res.yawc[i].toFixed(1),fused_x_cm:+res.fx[i].toFixed(1),fused_y_cm:+res.fy[i].toFixed(1),fused_heading_deg:+res.yaw[i].toFixed(1),pred_x_cm:res.hasPred?+res.px[i].toFixed(1):'',pred_y_cm:res.hasPred?+res.py[i].toFixed(1):'',pred_heading_deg:res.hasPred?+res.kyaw[i].toFixed(1):'',segment:res.labels[i]===null?'corner':ref[res.labels[i]].name,follow_state:r.fs,L:r.L,C:r.C,R:r.R,all_black:(r.k>>3)&1,ev_cross:(r.k>>4)&1}));}
function mapSVG(res,ref,title,opt){
 opt=Object.assign({drawing:true,pred:true,fused:true,mapped:true},opt||{});
 const rp=mapRefPoly(ref),ax=[].concat(rp.map(p=>p[0])),ay=[].concat(rp.map(p=>p[1]));
 if(opt.mapped){ax.push(...res.xs);ay.push(...res.ys);}if(opt.fused){ax.push(...res.fx);ay.push(...res.fy);}if(opt.pred&&res.hasPred){ax.push(...res.px);ay.push(...res.py);}
 const x0=Math.min(...ax)-30,x1=Math.max(...ax)+30,y0=Math.min(...ay)-30,y1=Math.max(...ay)+30,W=900,k=W/(x1-x0),H=Math.round((y1-y0)*k)+70;
 const X=x=>((x-x0)*k).toFixed(1),Y=y=>((y1-y)*k+40).toFixed(1),o=[];
 const line=(xs,ys,c,w,dash,op)=>'<polyline fill="none" stroke="'+c+'" stroke-width="'+w+'"'+(dash?' stroke-dasharray="'+dash+'"':'')+(op?' stroke-opacity="'+op+'"':'')+' stroke-linecap="round" stroke-linejoin="round" points="'+xs.map((x,i)=>X(x)+','+Y(ys[i])).join(' ')+'"/>';
 o.push('<svg xmlns="http://www.w3.org/2000/svg" width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" font-family="sans-serif">');
 o.push('<rect width="100%" height="100%" fill="#ffffff"/>');
 o.push('<text x="10" y="22" font-size="16" fill="#222">'+title+' - grid 50 cm'+(opt.mapped?' - mapped scale x'+res.scale.toFixed(2):'')+'</text>');
 for(let g=Math.floor(x0/50)*50;g<=x1;g+=50)o.push('<line x1="'+X(g)+'" y1="40" x2="'+X(g)+'" y2="'+H+'" stroke="#eee"/>');
 for(let g=Math.floor(y0/50)*50;g<=y1;g+=50)o.push('<line x1="0" y1="'+Y(g)+'" x2="'+W+'" y2="'+Y(g)+'" stroke="#eee"/>');
 if(opt.drawing)o.push('<polyline fill="none" stroke="#999" stroke-width="10" stroke-opacity="0.35" stroke-linejoin="round" points="'+rp.map(p=>X(p[0])+','+Y(p[1])).join(' ')+'"/>');
 if(opt.mapped){let i=1;while(i<res.xs.length){const c=MAPCOL[res.run[i].fs]||MAPCOL[1],pts=[X(res.xs[i-1])+','+Y(res.ys[i-1])];
  while(i<res.xs.length&&(MAPCOL[res.run[i].fs]||MAPCOL[1])===c){pts.push(X(res.xs[i])+','+Y(res.ys[i]));i++;}
  o.push('<polyline fill="none" stroke="'+c+'" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" points="'+pts.join(' ')+'"/>');}
  res.run.forEach((r,j)=>{if((r.k>>3)&1)o.push('<circle cx="'+X(res.xs[j])+'" cy="'+Y(res.ys[j])+'" r="2.5" fill="#212529"/>');});
  res.segs.slice(1).forEach(g=>o.push('<rect x="'+(X(res.xs[g.i0])-3.5)+'" y="'+(Y(res.ys[g.i0])-3.5)+'" width="7" height="7" fill="#1098ad"/>'));}
 if(opt.fused)o.push(line(res.fx,res.fy,'#2f9e44',2.2,'',0.95));
 if(opt.pred&&res.hasPred)o.push(line(res.px,res.py,'#8d6e63',2.4,'7 4',1));
 const ex=opt.fused?res.fx:(opt.pred&&res.hasPred?res.px:res.xs),ey=opt.fused?res.fy:(opt.pred&&res.hasPred?res.py:res.ys),e=ex.length-1;
 o.push('<circle cx="'+X(ex[e])+'" cy="'+Y(ey[e])+'" r="5" fill="none" stroke="#e03131" stroke-width="2"/>');
 o.push('<circle cx="'+X(0)+'" cy="'+Y(0)+'" r="6" fill="#2b8a3e"/><text x="'+(+X(0)+8)+'" y="'+(+Y(0)+16)+'" font-size="13" fill="#2b8a3e">start C</text>');
 let lx=10;const ly=H-12,ent=[];
 if(opt.drawing)ent.push(['b','#999','drawing',85]);
 if(opt.pred&&res.hasPred)ent.push(['d','#8d6e63','predicted (wheel model)',170]);
 if(opt.fused)ent.push(['l','#2f9e44','fused (gyro + wheel)',150]);
 if(opt.mapped)[[1,'FOLLOW'],[2,'CROSS hold'],[3,'corner TURN'],[4,'SEARCH']].forEach(([st,nm])=>ent.push(['l',MAPCOL[st],'mapped '+nm,128]));
 ent.push(['r','#e03131','car now',70]);
 ent.forEach(([t,c,nm,w])=>{if(lx+w>W){lx=10;}
  if(t==='r')o.push('<circle cx="'+(lx+5)+'" cy="'+(ly-3)+'" r="4" fill="none" stroke="'+c+'" stroke-width="2"/>');
  else if(t==='b')o.push('<rect x="'+lx+'" y="'+(ly-7)+'" width="14" height="8" fill="'+c+'" fill-opacity="0.35"/>');
  else o.push('<rect x="'+lx+'" y="'+(ly-4)+'" width="14" height="4" fill="'+c+'"/>');
  o.push('<text x="'+(lx+18)+'" y="'+ly+'" font-size="12">'+nm+'</text>');lx+=w;});
 o.push('</svg>');return o.join('\n');}
/* ---- target-following runs (mode 7): the L,C,R slots carry heading error, target width, bearing ---- */
function mapBuildFollow(run){
 const N=run.length,t=run.map(r=>(r.t-run[0].t)/1000),yaw=run.map(r=>r.yaw-run[0].yaw),set=run.map((r,i)=>yaw[i]+r.tgtErr),
  fx=run.map(r=>r.x-run[0].x),fy=run.map(r=>r.y-run[0].y),hasPred=run.every(r=>r.kh!==undefined),res={run,t,yaw,set,fx,fy,hasPred,width:run.map(r=>r.tgtW),bear:run.map(r=>r.tgtBear),err:run.map(r=>r.tgtErr)};
 if(hasPred){res.kyaw=mapUnwrap(run.map(r=>r.kh));res.px=run.map(r=>r.kx-run[0].kx);res.py=run.map(r=>r.ky-run[0].ky);}
 const trk=run.map((r,i)=>r.fs===1||r.fs===2?i:-1).filter(i=>i>=0),arr=run.findIndex(r=>r.fs===2),first=run.findIndex(r=>r.fs===1);
 res.firstTrack=first>=0?t[first]:null;res.arrived=arr>=0?t[arr]:null;
 res.errRms=trk.length?Math.sqrt(trk.reduce((a,i)=>a+res.err[i]*res.err[i],0)/trk.length):null;
 res.finalWidth=res.width[N-1];res.pathCm=(run[N-1].dist-run[0].dist);
 res.lostCount=run.reduce((a,r,i)=>a+(i>0&&(r.fs===3||r.fs===4)&&run[i-1].fs!==3&&run[i-1].fs!==4?1:0),0);
 res.hdgRms=hasPred?Math.sqrt(yaw.reduce((a,v,i)=>a+(res.kyaw[i]-v)*(res.kyaw[i]-v),0)/N):null;
 res.endGap=hasPred?Math.hypot(res.px[N-1]-fx[N-1],res.py[N-1]-fy[N-1]):null;
 return res;}
function mapFollowRows(res){return res.run.map((r,i)=>({t_s:+res.t[i].toFixed(2),state:['','TRACK','ARRIVED','LOST','SEARCH'][r.fs]||r.fs,fused_x_cm:res.fx[i],fused_y_cm:res.fy[i],fused_heading_deg:+res.yaw[i].toFixed(1),
 pred_x_cm:res.hasPred?res.px[i]:'',pred_y_cm:res.hasPred?res.py[i]:'',pred_heading_deg:res.hasPred?+res.kyaw[i].toFixed(1):'',target_heading_deg:+res.set[i].toFixed(1),heading_error_deg:res.err[i],target_width_permille:res.width[i],target_bearing_deg:res.bear[i],PL:r.PL,PR:r.PR}));}
function mapFollowSummary(res,runNo,runCount){const L=[];
 L.push('Target-following summary (run '+runNo+' of '+runCount+', '+res.run.length+' samples, '+res.t[res.t.length-1].toFixed(1)+' s)');
 L.push('first target lock: '+(res.firstTrack===null?'never':res.firstTrack.toFixed(1)+' s')+'; arrived at stop width: '+(res.arrived===null?'no':res.arrived.toFixed(1)+' s')+'; target lost '+res.lostCount+' time(s)');
 L.push('heading error of the inner loop while tracking (rms): '+mapNum(res.errRms)+' deg; final target width '+res.finalWidth+' per mille of the image');
 L.push('distance driven (odometer): '+mapNum(res.pathCm)+' cm; end point fused ('+res.fx[res.fx.length-1]+', '+res.fy[res.fy.length-1]+') cm'+(res.hasPred?', predicted ('+res.px[res.px.length-1]+', '+res.py[res.py.length-1]+') cm, gap '+mapNum(res.endGap)+' cm':''));
 if(res.hasPred)L.push('predicted heading minus gyro heading: at the end '+mapNum(res.kyaw[res.kyaw.length-1]-res.yaw[res.yaw.length-1])+' deg, rms '+mapNum(res.hdgRms)+' deg');
 return L.join('\n');}
function mapFollowCharts(res){
 const H=[{name:'target heading (set point)',color:'#e8590c',x:res.t,y:res.set,dash:'6 4',w:2},{name:'gyro / fused heading',color:'#2f9e44',x:res.t,y:res.yaw,w:2}];
 if(res.hasPred)H.push({name:'predicted (wheel model)',color:'#8d6e63',x:res.t,y:res.kyaw,dash:'2 3',w:2});
 return [mapChartSVG({title:'Target following: heading vs time (deg, + = left)',xl:'time (s)',yl:'heading (deg)',series:H,zero:true}),
  mapChartSVG({title:'Target following: target width in the image (per mille) and camera bearing (deg)',xl:'time (s)',yl:'width / bearing',series:[{name:'width (per mille)',color:'#1c7ed6',x:res.t,y:res.width,w:2},{name:'bearing (deg, + = left)',color:'#ae3ec9',x:res.t,y:res.bear,w:1.5}],zero:true})];}
function mapFollowSVG(res,title,opt){
 opt=Object.assign({pred:true,fused:true},opt||{});
 const ax=[0],ay=[0];if(opt.fused){ax.push(...res.fx);ay.push(...res.fy);}if(opt.pred&&res.hasPred){ax.push(...res.px);ay.push(...res.py);}
 const x0=Math.min(...ax)-30,x1=Math.max(...ax)+30,y0=Math.min(...ay)-30,y1=Math.max(...ay)+30,W=900,k=W/(x1-x0),H=Math.round((y1-y0)*k)+70,X=x=>((x-x0)*k).toFixed(1),Y=y=>((y1-y)*k+40).toFixed(1),o=[];
 const line=(xs,ys,c,w,dash)=>'<polyline fill="none" stroke="'+c+'" stroke-width="'+w+'"'+(dash?' stroke-dasharray="'+dash+'"':'')+' stroke-linecap="round" stroke-linejoin="round" points="'+xs.map((x,i)=>X(x)+','+Y(ys[i])).join(' ')+'"/>';
 o.push('<svg xmlns="http://www.w3.org/2000/svg" width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" font-family="sans-serif"><rect width="100%" height="100%" fill="#ffffff"/>');
 o.push('<text x="10" y="22" font-size="16" fill="#222">'+title+' - grid 50 cm</text>');
 for(let g=Math.floor(x0/50)*50;g<=x1;g+=50)o.push('<line x1="'+X(g)+'" y1="40" x2="'+X(g)+'" y2="'+H+'" stroke="#eee"/>');
 for(let g=Math.floor(y0/50)*50;g<=y1;g+=50)o.push('<line x1="0" y1="'+Y(g)+'" x2="'+W+'" y2="'+Y(g)+'" stroke="#eee"/>');
 if(opt.fused)o.push(line(res.fx,res.fy,'#2f9e44',2.4,''));if(opt.pred&&res.hasPred)o.push(line(res.px,res.py,'#8d6e63',2.4,'7 4'));
 res.run.forEach((r,i)=>{if(r.fs===2&&(i===0||res.run[i-1].fs!==2))o.push('<rect x="'+(X(res.fx[i])-4)+'" y="'+(Y(res.fy[i])-4)+'" width="8" height="8" fill="#e8590c"/>');});
 const e=res.fx.length-1;o.push('<circle cx="'+X(res.fx[e])+'" cy="'+Y(res.fy[e])+'" r="5" fill="none" stroke="#e03131" stroke-width="2"/>');
 o.push('<circle cx="'+X(0)+'" cy="'+Y(0)+'" r="6" fill="#2b8a3e"/><text x="'+(+X(0)+8)+'" y="'+(+Y(0)+16)+'" font-size="13" fill="#2b8a3e">start</text>');
 o.push('<text x="10" y="'+(H-12)+'" font-size="12">green = fused (gyro + wheel model) · brown dashed = predicted (wheel model only) · orange square = reached stop width · red ring = car now</text></svg>');return o.join('\n');}
/*END MAPCORE*/
