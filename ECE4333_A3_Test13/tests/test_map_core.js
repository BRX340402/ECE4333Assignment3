// node tests/test_map_core.js <uno_lines.jsonl> [out.svg]  -> prints summary; checks the Test11 predicted/fused tracks
const fs=require('fs');eval(fs.readFileSync(__dirname+'/../tools/map_core.js','utf8')+';globalThis.M={mapStateSVG,mapTransRows,SMNAME,mapBuildFollow,mapDecode,mapRuns,mapBuildRun,mapSummary,mapSegRows,mapSVG,MAPREF0,mapCSV,mapHeadingSVG,mapPointRows};');
const S=fs.readFileSync(process.argv[2],'utf8').split('\n').filter(Boolean).map(l=>{const o=JSON.parse(l);return(o.line||o)}).filter(o=>o.N===230).map(o=>M.mapDecode(o.d));
S.forEach((s,i)=>{s.n=i;s.sm=[1,1,2,3,4,1,5,1,1][s.fs]||1;});
// Assignment 4: synthetic overtaking episode (OUT, PASS, RETURN) and a STOP-sign stop inside the run
{const a=300,b=520;for(let i=a;i<=b&&i<S.length;i++)S[i].sm=i<a+30?9:(i<a+120?10:11);S[300].u=30;for(let i=600;i<640&&i<S.length;i++)S[i].sm=6;}
const runs=M.mapRuns(S);const run=runs.reduce((a,b)=>b.length>a.length?b:a);
const res=M.mapBuildRun(run,M.MAPREF0);
console.log(M.mapSummary(res,M.MAPREF0,runs.indexOf(run)+1,runs.length));
console.log(M.mapCSV(M.mapSegRows(res,M.MAPREF0)));
fs.writeFileSync(process.argv[3]||'/dev/null',M.mapSVG(res,M.MAPREF0,'JS port test'));
// Test11 checks
let bad=0;const chk=(c,m)=>{console.log((c?'PASS ':'FAIL ')+m);if(!c)bad++;};
chk(res.hasPred,'run carries the predicted pose (kh,kx,ky)');
chk(res.err.mapped.mean<25&&res.err.fused.mean<res.err.pred.mean*0.5,'mapped track stays near the drawn centre line; fused is much closer than predicted');
chk(res.err.pred.mean>res.err.fused.mean,'predicted track (wheel model only) is worse than the gyro-fused track');
const svg=M.mapSVG(res,M.MAPREF0,'t');chk(svg.includes('predicted (wheel model)')&&svg.includes('fused (gyro + wheel)'),'map shows predicted and fused layers');
const only=M.mapSVG(res,M.MAPREF0,'t',{pred:false,fused:false,mapped:false});chk(!only.includes('#8d6e63')&&!only.includes('#2f9e44" stroke-width="2.2"'),'layers can be switched off');
const hs=M.mapHeadingSVG(res,M.MAPREF0);chk(hs.includes('<polyline')&&hs.includes('gyro / fused')&&hs.includes('predicted'),'heading chart drawn');
const rows=M.mapPointRows(res,M.MAPREF0);chk(rows.length===res.run.length&&'pred_x_cm' in rows[0]&&'fused_x_cm' in rows[0],'points CSV has predicted and fused columns');
fs.writeFileSync('/tmp/heading.svg',hs);
{const ys=[...svg.matchAll(/<text x="(\d+)" y="([\d.]+)" font-size="12">(drawing|predicted \(wheel model\)|fused \(gyro \+ wheel\)|mapped [A-Za-z ]+|car now)<\/text>/g)].map(m=>[+m[1],+m[2]]);
 const seen=new Set();let overlap=false;ys.forEach(([x,y])=>{const k=y;ys.forEach(([x2,y2])=>{if(y2===y&&x2!==x&&Math.abs(x2-x)<60)overlap=true;});});
 chk(ys.length>=7&&new Set(ys.map(a=>a[1])).size>=2&&!overlap,'map legend wraps onto a second row instead of printing over the first (screenshot bug)');}

const sum=M.mapSummary(res,M.MAPREF0,1,1),sv=M.mapSVG(res,M.MAPREF0,'t');
chk(res.hasSm&&res.trans.length>5,'state machine transitions are derived from the logged state codes');
chk(res.episodes.length===1&&res.episodes[0].ok===true&&res.episodes[0].obstacle&&res.episodes[0].obstacle.u===30,'overtaking episode found: duration, success, obstacle position from the ultrasonic range');
chk(res.objects.some(o=>o.kind==='obstacle')&&res.objects.some(o=>o.kind==='stop'),'obstacle and STOP-sign positions estimated in the inertial frame');
chk(/overtaking: 1 manoeuvre/.test(sum)&&/estimated STOP sign position/.test(sum)&&/off-road episodes/.test(sum),'summary lists overtaking, object positions and the traffic-rule self-check');
chk(M.mapStateSVG(res).includes('OVERTAKE: PASS')&&M.mapTransRows(res)[0].to!==undefined,'state timeline chart and transitions CSV rows');
chk(sv.includes('#7048e8')&&sv.includes('obstacle'),'map highlights the overtaking path and draws the obstacle');
{const rows=M.mapSegRows(res,M.MAPREF0);chk(rows.length>3&&rows.every(r=>r.turn_check!=='WRONG WAY')&&rows.some(r=>r.expected_turn_deg!==''),'segment table lists expected vs measured turn at every corner (no wrong way in the simulation)');}
fs.writeFileSync('/tmp/map13.svg',sv);fs.writeFileSync('/tmp/state13.svg',M.mapStateSVG(res));
process.exit(bad?1:0);
