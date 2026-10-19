// node tests/test_map_core.js <uno_lines.jsonl> [out.svg]  -> prints summary; checks the Test11 predicted/fused tracks
const fs=require('fs');eval(fs.readFileSync(__dirname+'/../tools/map_core.js','utf8')+';globalThis.M={mapDecode,mapRuns,mapBuildRun,mapSummary,mapSegRows,mapSVG,MAPREF0,mapCSV,mapHeadingSVG,mapPointRows};');
const S=fs.readFileSync(process.argv[2],'utf8').split('\n').filter(Boolean).map(l=>{const o=JSON.parse(l);return(o.line||o)}).filter(o=>o.N===230).map(o=>M.mapDecode(o.d));
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
process.exit(bad?1:0);
