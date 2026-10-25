const fs=require('fs');eval(fs.readFileSync(__dirname+'/../tools/map_core.js','utf8')+';globalThis.analyze=mapStateAnalysis;');
const assert=require('assert');
function check(states){let n=states.length,r={run:states.map(sm=>({sm,u:30})),t:states.map((_,i)=>i),yaw:Array(n).fill(0),fx:Array(n).fill(0),fy:Array(n).fill(0)};analyze(r);return r.episodes;}
assert.equal(check([1,8,8,1])[0].outcome,'wait cleared');
assert.equal(check([1,8,8,1])[0].ok,null);
let e=check([1,9,5,6,9,10,7,11,1]);assert.equal(e.length,1);assert.equal(e[0].ok,true);
assert.equal(check([9,10,11,0])[0].ok,null);
assert.equal(check([9,10,12])[0].ok,false);
console.log('PASS A4 wait-only, traffic pauses, abort and failure statistics');
