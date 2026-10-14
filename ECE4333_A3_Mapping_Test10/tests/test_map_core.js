// node tests/test_map_core.js <uno_lines.jsonl>  -> prints summary; compares with draw_map.py output
const fs=require('fs');eval(fs.readFileSync(__dirname+'/../tools/map_core.js','utf8')+';globalThis.M={mapDecode,mapRuns,mapBuildRun,mapSummary,mapSegRows,mapSVG,MAPREF0,mapCSV};');
const S=fs.readFileSync(process.argv[2],'utf8').split('\n').filter(Boolean).map(l=>{const o=JSON.parse(l);return(o.line||o)}).filter(o=>o.N===230).map(o=>M.mapDecode(o.d));
const runs=M.mapRuns(S);const run=runs.reduce((a,b)=>b.length>a.length?b:a);
const res=M.mapBuildRun(run,M.MAPREF0);
console.log(M.mapSummary(res,M.MAPREF0,runs.indexOf(run)+1,runs.length));
console.log(M.mapCSV(M.mapSegRows(res,M.MAPREF0)));
fs.writeFileSync(process.argv[3]||'/dev/null',M.mapSVG(res,M.MAPREF0,'JS port test'));
