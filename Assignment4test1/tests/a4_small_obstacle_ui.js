const fs=require('fs'), vm=require('vm'), assert=require('assert');
const html=fs.readFileSync(__dirname+'/../ESP32_Assignment2/TrafficPage.h','utf8');
const script=html.match(/<script>([\s\S]*?)<\/script>/)[1];new vm.Script(script);
const fields={};for(const id of ['OT','OL','OC','OJ']){const tag=html.match(new RegExp('<input id="'+id+'"[^>]+>'))[0];fields[id]={};for(const key of ['value','min','max'])fields[id][key]=tag.match(new RegExp(key+'="([^"]+)"'))[1];}
const fn=script.slice(script.indexOf('function readOvertakeForm()'),script.indexOf('const A4IDS='));
const ctx={$:id=>fields[id]};vm.createContext(ctx);vm.runInContext(fn,ctx);
const read=()=>JSON.parse(JSON.stringify(ctx.readOvertakeForm()));
assert.deepEqual(read(),{trig:10,par:26,join:15});
for(const [id,bad] of [['OT','9'],['OT','51'],['OT','25.5'],['OT',''],['OL','2'],['OL','21'],['OC','19'],['OC','41'],['OJ','26']]){let old=fields[id].value;fields[id].value=bad;assert.throws(read);fields[id].value=old;}
fields.OT.value='10';fields.OL.value='3';fields.OC.value='20';fields.OJ.value='0';assert.deepEqual(read(),{trig:10,par:23,join:0});
fields.OT.value='50';fields.OL.value='20';fields.OC.value='40';assert.equal(read().par,60);
assert(html.includes('<option value="2" selected>Always left'));
const mem={'a4-small-obstacle-v1':JSON.stringify([0,60,1,100,1,35,45,8,1,35])};
for(const id of ['PA','PL','PM','PS','OS'])fields[id]={value:'0'};fields.OV={checked:false};fields.SR={value:'35'};
ctx.localStorage={getItem:k=>mem[k]||null,setItem:(k,v)=>mem[k]=v};
vm.runInContext(script.slice(script.indexOf('const A4IDS='),script.indexOf('async function panTest')),ctx);
ctx.loadA4();assert.equal(fields.OT.value,'50');assert.equal(fields.OV.checked,false);
ctx.saveA4();fields.OT.value='30';ctx.loadA4();assert.equal(fields.OT.value,'50');
console.log('PASS small-obstacle defaults, invalid inputs, boundary values, old-setting isolation and settings round-trip');
