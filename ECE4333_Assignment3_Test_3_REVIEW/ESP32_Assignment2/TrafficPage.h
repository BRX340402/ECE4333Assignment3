#pragma once
static const char TRAFFIC_PAGE[] PROGMEM=R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ECE 4333 Robot Car Test 3</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#101722;color:#edf3fa;font:16px system-ui,sans-serif}main{max-width:960px;margin:auto;padding:18px}header,.actions,nav{display:flex;align-items:center;gap:10px;flex-wrap:wrap}header{justify-content:space-between}h1{font-size:24px;margin:8px 0}h2{font-size:19px;margin:0 0 12px}section,details{background:#1c2939;border:1px solid #304156;border-radius:12px;padding:16px;margin:14px 0}button,input,select{font:inherit;border:1px solid #52677f;border-radius:7px;padding:9px;background:#26394e;color:#fff}button{cursor:pointer}button:disabled{opacity:.5;cursor:default}button.primary{background:#287b67}.danger{background:#bd3b3b}nav{margin:14px 0}nav button[aria-selected=true]{background:#287b67;border-color:#55baa2}label{display:block;margin:12px 0}input[type=number]{width:100px}input[type=checkbox]{width:auto}p{line-height:1.5;margin:10px 0}.hint,small{color:#b5c5d8}#message,#cal{color:#ffcf91;min-height:24px;overflow-wrap:anywhere}#live{display:block;width:100%;height:auto;min-height:180px;background:#080d15;border-radius:8px}table{width:100%;border-collapse:collapse;font-size:14px}td,th{padding:10px 5px;text-align:left;border-bottom:1px solid #354459}th{color:#b5c5d8;font-weight:500}summary{cursor:pointer}a{color:#8cd7ff}.tablewrap{overflow:auto}#savedCal{white-space:pre-wrap;font-size:14px;line-height:1.7}.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}.drivepad{display:grid;grid-template-columns:repeat(3,65px);gap:7px;margin:10px 0}.drivepad button{touch-action:none;user-select:none}[hidden]{display:none!important}@media(max-width:620px){.grid{grid-template-columns:1fr}main{padding:12px}td,th{padding:9px 3px;font-size:12px}}
</style></head><body><main>
<header><h1>Robot Car <small>Test 1</small></h1><span id="runState">Connecting…</span></header>
<div class="actions"><button class="danger" onclick="stopCar()">STOP CAR</button><button class="danger" onclick="emergencyStop()">EMERGENCY STOP (latched)</button><button onclick="clearEStop()">Reset E-stop</button><button class="primary" onclick="startCar()">Start continuous run</button></div>
<p class="hint">Browser stop needs Wi-Fi. Reset only clears latch; it does NOT restart the car. Keep the physical power switch accessible.</p>
<p id="poseInfo" class="hint">Awaiting position telemetry…</p>
<p id="message" role="status"></p><p id="visionHealth" class="hint"></p>
<section><div style="position:relative"><img id="live" width="320" height="240" alt="Live camera — connecting"><div id="boxes" style="position:absolute;inset:0;pointer-events:none"></div></div><small id="previewState">Connecting camera…</small> <button id="togglePreview" onclick="togglePreview()">Pause preview</button>
<div class="tablewrap"><table><thead><tr><th>Target</th><th>Detected</th><th>Pixels</th><th>Width (px)</th><th>Distance (cm)</th></tr></thead><tbody>
<tr><td>Red</td><td id="redSeen">—</td><td id="redPixels">—</td><td id="redWidth">—</td><td id="redDistance">—</td></tr>
<tr><td>Green</td><td id="greenSeen">—</td><td id="greenPixels">—</td><td id="greenWidth">—</td><td id="greenDistance">—</td></tr>
<tr><td>STOP sign</td><td id="stopSeen">—</td><td id="stopPixels">—</td><td id="stopWidth">—</td><td id="stopDistance">—</td></tr>
</tbody></table></div><small>Pixels = coloured area; width is used for distance. STOP uses its matched red region after AI confirmation. “—” means unavailable or not calibrated.</small></section>
<nav role="tablist"><button id="carTab" role="tab" aria-selected="true" aria-controls="carPanel" onclick="switchTab('car')">Car control</button><button id="cameraTab" role="tab" aria-selected="false" aria-controls="cameraPanel" onclick="switchTab('camera')">Camera & calibration</button></nav>
<div id="carPanel" role="tabpanel">
<section><h2>Continuous figure-eight</h2><p>Start follows the line without colour calibration. Uncalibrated red slows the car but keeps it moving. Calibrated red stops within the chosen distance and waits for green; green needs no distance calibration. Confirmed STOP signs pause for 3 seconds, then continue. No lap limit.</p><p class="hint">Autonomous running continues after Wi-Fi disconnects or this page closes. STOP requires a working connection; local faults still stop the car. Manual driving retains its connection timeout.</p>
<label>Autonomous cruise speed (PWM 50–150) <input id="CS" onchange="$('DS').value=Math.min(Number($('DS').value),Number(this.value));saveSettings()" type="number" min="50" max="150" value="60"></label>
<label>Reduced speed (PWM 0 to cruise speed) <input id="DS" onchange="saveSettings()" type="number" min="0" max="150" value="40"></label>
<label>Red action threshold (pixels) <input id="RP" onchange="saveSettings()" type="number" min="0" max="160000" value="100"></label>
<p class="hint">Stop / slow only when the detected red area reaches this value. Use the Red Pixels reading below the preview, not width. Zero adds no extra area limit; the detector minimum still applies. Reduced speed 0 stops on red in the original mode too. After STOP is confirmed, crossing uses cruise speed if reduced speed is 0.</p>
<label><input id="RS" type="checkbox" onchange="saveSettings()">Stop on red, then scan for STOP</label>
<p class="hint">Enabled: stop as soon as red is detected, regardless of distance calibration. Two accumulated STOP frames release the car to cross; missed frames do not reset the count. Scanning continues while stopped. Plain red or an uncertain scan keeps it stopped; three consecutive valid green-only frames release either the red verification hold or the red-light wait. Disabled: use the original distance-based behaviour above. Changing this option or cruise speed stops the car and saves automatically. Wait for UNO confirmed, then press Start again. Try PWM 60 for a slower cruise.</p>
<label>Stopping distance (cm) <input id="SC" type="number" min="5" max="150" value="25"></label>
<details><summary>Line and search settings</summary><p id="floor">Waiting for floor sensors…</p>
<label>Black range <input id="BL" type="number" min="0" max="949" value="600"> to <input id="BH" type="number" min="1" max="950" value="850"></label>
<label>First right search (degrees) <input id="RA" type="number" min="30" max="120" value="120"></label>
<label>Reverse-left sweep (degrees) <input id="SA" type="number" min="60" max="240" value="240"></label>
<label>Minimum straight crossing (ms) <input id="CT" type="number" min="300" max="5000" value="1500"></label>
<label>Gyro Z sign <select id="GS"><option value="0">Negative raw Z = left</option><option value="1">Positive raw Z = left</option></select></label>
<small>Choose a black range that excludes the grey floor. Search: right up to 120°, then reverse left through the original heading to 120° left (240° total left sweep). No third right sweep. A detected side sensor resumes steering toward the line. With a working gyro, left turns must increase yaw. Without a gyro, scans use approximate motor-on times, not measured angles.</small></details>
<button onclick="saveSettings()">Save car settings</button><p id="applied" class="hint"></p></section>
<details><summary>Manual movement</summary><label>Speed <input id="speed" type="number" value="60" min="0" max="180"></label><p class="hint">Hold an arrow to move; release to stop.</p><div class="drivepad"><span></span><button data-dir="1">↑</button><span></span><button data-dir="3">←</button><button class="danger" onclick="stopCar()">■</button><button data-dir="4">→</button><span></span><button data-dir="2">↓</button></div></details>
</div>
<div id="cameraPanel" role="tabpanel" hidden>
<section><h2>Distance calibration</h2><p>Stop the car. Select a target, scan its width, enter the measured camera-to-target distance, then save. Keep the target still. STOP requires AI confirmation.</p>
<label>Target <select id="profile" onchange="resetSample()"><option value="light">Red</option><option value="green">Green</option><option value="stop">STOP sign</option></select></label>
<label>Scanned width (px) <input id="width" readonly> / image <input id="image_w" readonly></label>
<label>Measured distance (cm) <input id="cm" type="number" min="5" max="300" value="30"></label>
<button onclick="sampleWidth()">Scan target width</button><button id="saveCal" onclick="calibrate()" disabled>Save measured distance</button><button onclick="clearCalibration()">Clear selected target</button><p id="cal" role="status"></p><p id="sampleInfo" class="hint"></p><div id="savedCal">Loading calibration…</div></section>
<section><h2>Camera settings · SVGA startup</h2><p class="hint">Starts at SVGA 800×600. After checking the picture, manually select CIF 400×296 to activate the supplied STOP calibration (96 px / 400 px at 10 cm). JPEG quality 10, two frame buffers and the uploaded camera orientation are retained. Camera white balance uses driver defaults. Changing settings stops the car.</p>
<label><input id="detect" type="checkbox" checked onchange="setDetection('detect',this.checked)">STOP AI (96×96)</label>
<label><input id="color" type="checkbox" checked onchange="setDetection('color',this.checked)">Red / green detection</label>
<label>Resolution <select id="framesize" onchange="setCamera('framesize',this.value)"><option value="0">QQVGA 160×120</option><option value="3">HQVGA 240×176</option><option value="4">QVGA 320×240</option><option value="5">CIF 400×296</option><option value="6">VGA 640×480</option><option value="7" selected>SVGA 800×600</option><option value="8">XGA 1024×768</option><option value="9">SXGA 1280×1024</option><option value="10">UXGA 1600×1200</option></select></label>
<label>White balance <select id="wb_mode" onchange="setCamera('wb_mode',this.value)"><option value="0">Auto (0)</option><option value="1">Sunny (1)</option><option value="2">Cloudy (2)</option><option value="3">Office (3)</option><option value="4">Home (4)</option></select></label>
<label><input id="awb" type="checkbox" checked onchange="setCamera('awb',+this.checked)">Automatic white balance</label>
<label><input id="awb_gain" type="checkbox" checked onchange="setCamera('awb_gain',+this.checked)">White balance gain</label>
<button id="restoreDefaults" onclick="restoreCameraDefaults()">Restore SVGA camera defaults</button><p class="hint">Stops the car and restarts ESP32. Reconnect Wi-Fi and refresh afterward. Distance calibration remains saved. The sensor driver still supplies its normal automatic image controls.</p>
<label>Brightness <select id="brightness" onchange="setCamera('brightness',this.value)"><option>-2</option><option>-1</option><option selected>0</option><option>1</option><option>2</option></select></label>
<label>Contrast <select id="contrast" onchange="setCamera('contrast',this.value)"><option>-2</option><option>-1</option><option selected>0</option><option>1</option><option>2</option></select></label>
<label>Saturation <select id="saturation" onchange="setCamera('saturation',this.value)"><option>-2</option><option>-1</option><option selected>0</option><option>1</option><option>2</option></select></label>
<label><input id="vflip" type="checkbox" onchange="setCamera('vflip',+this.checked)">Flip vertically</label><label><input id="hmirror" type="checkbox" onchange="setCamera('hmirror',+this.checked)">Mirror horizontally</label>
<details><summary>Camera diagnosis</summary><p id="cameraInfo" class="hint">Read actual camera settings to see uptime and frame age.</p><p class="hint">Turn off STOP AI and Red / green detection to test camera-only output. Test bars come from the sensor; turn them off before driving.</p>
<label><input id="colorbar" type="checkbox" onchange="setCamera('colorbar',+this.checked)">Sensor test colour bars</label>
</details><details><summary>Exposure, gain and advanced settings</summary>
<label>JPEG quality <input id="quality" type="number" min="10" max="63" value="10"><button onclick="setCamera('quality',$('quality').value)">Apply</button></label>
<label><input id="aec" type="checkbox" checked onchange="setCamera('aec',+this.checked)">Auto exposure</label>
<label><input id="aec2" type="checkbox" onchange="setCamera('aec2',+this.checked)">DSP exposure</label>
<label>Exposure level <select id="ae_level" onchange="setCamera('ae_level',this.value)"><option>-2</option><option>-1</option><option selected>0</option><option>1</option><option>2</option></select></label>
<label>Manual exposure <input id="aec_value" type="number" min="0" max="1200" value="300"><button onclick="setCamera('aec_value',$('aec_value').value)">Apply</button></label>
<label><input id="agc" type="checkbox" checked onchange="setCamera('agc',+this.checked)">Auto gain</label>
<label>Manual gain <input id="agc_gain" type="number" min="0" max="30" value="0"><button onclick="setCamera('agc_gain',$('agc_gain').value)">Apply</button></label>
<label>Gain ceiling <select id="gainceiling" onchange="setCamera('gainceiling',this.value)"><option value="0">2×</option><option value="1">4×</option><option value="2">8×</option><option value="3">16×</option><option value="4">32×</option><option value="5">64×</option><option value="6">128×</option></select></label>
<label>Effect <select id="special_effect" onchange="setCamera('special_effect',this.value)"><option value="0">None</option><option value="1">Negative</option><option value="2">Grayscale</option><option value="3">Red tint</option><option value="4">Green tint</option><option value="5">Blue tint</option><option value="6">Sepia</option></select></label>
<label><input id="bpc" type="checkbox" onchange="setCamera('bpc',+this.checked)">Black pixel correction</label>
<label><input id="wpc" type="checkbox" onchange="setCamera('wpc',+this.checked)">White pixel correction</label>
<label><input id="raw_gma" type="checkbox" onchange="setCamera('raw_gma',+this.checked)">Gamma</label>
<label><input id="lenc" type="checkbox" onchange="setCamera('lenc',+this.checked)">Lens correction</label>
<label><input id="dcw" type="checkbox" onchange="setCamera('dcw',+this.checked)">Downsize</label>
<label>Model view <select id="view_mode" onchange="setCamera('view_mode',this.value)"><option value="1">Centre crop (trained)</option><option value="0">Full image letterbox</option><option value="2">2× crop</option><option value="3">4× crop</option></select></label>
<label>AI threshold (%) <input id="threshold" type="number" min="50" max="99" value="70"><button onclick="setCamera('threshold',$('threshold').value)">Apply</button></label>
</details><button onclick="loadCamera()">Read actual camera settings</button><p class="hint">Reboot restores SVGA and the camera startup profile. Distance calibration is saved separately. Changing resolution makes a calibration inactive until its resolution matches again.</p>
<button id="capture" onclick="captureNew()">Capture training photo</button><span id="captureState"></span><p id="downloads" hidden><a id="jpgLink">Open original JPEG</a> · <a id="bmpLink">Model input BMP</a></p>
</section></div></main>
<script>
const $=id=>document.getElementById(id),names=['OFF','RESERVED','TRACK','APPROACH','VERIFY STOP (slow)','WAIT FOR GREEN','STOP HOLD','CROSS STRAIGHT','FAULT'];
const keys=['SC','BL','BH','RA','SA','CT','GS','CS','DS','RP'];let driving=false,latest=null,scannedToken=null,sampleResolution=null,currentTab='car',previewTimer=null,manualHeld=false,previewBusy=false,previewPaused=false,previewURL=null,previewStamp='',previewLastShown=0,cameraBusy=false;
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function request(url,data){const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),4000);try{const r=await fetch(url,{cache:'no-store',signal:controller.signal,...(data===undefined?{}:{method:'POST',body:new URLSearchParams(data)})});const t=await r.text();if(!r.ok){const e=Error(t);e.status=r.status;throw e;}return t;}finally{clearTimeout(timer);}}
async function post(url,data={}){return request(url,data);}
function switchTab(name){currentTab=name;for(const t of ['car','camera']){$(t+'Panel').hidden=t!==name;$(t+'Tab').setAttribute('aria-selected',String(t===name));}}
async function stopCar(){driving=false;manualHeld=false;try{$('message').textContent=await post('/cmd',{action:'stop'});}catch(e){$('message').textContent='STOP was not delivered. Autonomous running may continue; use the physical power switch if needed.';}}
async function emergencyStop(){driving=false;manualHeld=false;try{$('message').textContent='Emergency stop command sent. Confirm ES=1 and PWM 0 on this page.';await post('/cmd',{action:'estop'});}catch(e){$('message').textContent='E-stop communication failed! Use physical power switch.';}}
async function clearEStop(){driving=false;try{await post('/cmd',{action:'clear_estop'});$('message').textContent='Reset requested. Confirm ES=0 and OFF before pressing Start.';}catch(e){$('message').textContent=e.message;}}
async function startCar(){if(settingsBusy){$('message').textContent='Wait for UNO settings confirmation.';return;}
 try{const wanted=readSettingsForm(),current=JSON.parse(await request('/traffic-settings'));
 if(!settingsMatch(current,wanted)&&!await saveSettings())return;
 $('message').textContent=await post('/traffic-start');driving=true;}catch(e){$('message').textContent=e.message;}}
async function loadSettings(){try{const d=JSON.parse(await request('/traffic-settings'));keys.forEach(k=>$(k).value=d[k]);$('RS').checked=!!d.RS;}catch(e){$('message').textContent=e.message;}}
let settingsBusy=false;
function readSettingsForm(){const d={};keys.forEach(k=>{const n=Number($(k).value);if(!Number.isInteger(n))throw Error('Use whole numbers');d[k]=n;});d.RS=+$('RS').checked;return d;}
function settingsMatch(current,wanted){return !!current.applied&&[...keys,'RS'].every(k=>Number(current[k])===wanted[k]);}
async function saveSettings(){if(settingsBusy)return false;settingsBusy=true;driving=false;manualHeld=false;$('RS').disabled=$('CS').disabled=$('DS').disabled=$('RP').disabled=true;
 try{const wanted=readSettingsForm();$('applied').textContent='Stopping car and applying settings…';
 await post('/cmd',{action:'stop'});
 let saved=false;for(let n=0;n<16;n++){try{await post('/traffic-settings',wanted);saved=true;break;}catch(e){if(e.status!==409)throw e;await delay(400);}}
 if(!saved)throw Error('UNO did not acknowledge OFF. Settings not applied.');
 for(let n=0;n<20;n++){const actual=JSON.parse(await request('/traffic-settings'));if(settingsMatch(actual,wanted)){$('message').textContent=`UNO confirmed: stop on red ${wanted.RS?'ON':'OFF'}, cruise PWM ${wanted.CS}, reduced PWM ${wanted.DS}, red threshold ${wanted.RP} px. Press Start to run.`;return true;}await delay(400);}
 throw Error('Settings saved but UNO confirmation missing. Do not start; check matching UNO Test 3.');
 }catch(e){$('message').textContent=e.message;return false;}finally{settingsBusy=false;$('RS').disabled=$('CS').disabled=$('DS').disabled=$('RP').disabled=false;}}
function resetSample(){scannedToken=null;sampleResolution=null;$('width').value='';$('image_w').value='';$('saveCal').disabled=true;$('sampleInfo').textContent='Scan the selected target first.';}
async function sampleWidth(){resetSample();try{const d=JSON.parse(await post('/traffic-sample',{profile:$('profile').value}));$('width').value=d.width;$('image_w').value=d.image_w;sampleResolution=d.framesize;scannedToken=d.token;$('saveCal').disabled=false;$('sampleInfo').textContent=`Scanned ${d.width} px. Enter the distance and save.`;}catch(e){$('cal').textContent=e.message;}}
async function calibrate(){try{if(scannedToken===null)throw Error('Scan the target first');$('cal').textContent=await post('/traffic-calibrate',{profile:$('profile').value,cm:$('cm').value,token:scannedToken});scannedToken=null;$('saveCal').disabled=true;}catch(e){$('cal').textContent=e.message;}}
async function clearCalibration(){try{$('cal').textContent=await post('/traffic-clear',{profile:$('profile').value});resetSample();}catch(e){$('cal').textContent=e.message;}}
function showSavedCalibration(d){const c=d.calibration||{},labels={light:'Red',green:'Green',stop:'STOP'};$('savedCal').textContent=['light','green','stop'].map(k=>{const v=c[k];return !v||!v.valid?`${labels[k]}: not calibrated`:`${labels[k]}: saved width ${v.width} px / image ${v.image_w} px, measured distance ${v.distance_cm} cm${v.active?'':' (resolution mismatch — rescan)'}`;}).join('\n');}
function showObjects(objects){for(const k of ['red','green','stop']){const v=objects[k]||{};$(k+'Seen').textContent=!v.valid?'—':v.seen?'Yes':'No';$(k+'Pixels').textContent=v.valid&&v.seen?v.pixels:'—';$(k+'Width').textContent=v.valid&&v.seen&&v.width>0?v.width:'—';$(k+'Distance').textContent=v.valid&&v.seen&&v.distance_cm>=0?v.distance_cm:'—';}}
const faultReasons={1:'Colour data older than 1.5 s',2:'Invalid or ambiguous colour frame',3:'Red target lost during approach (over 0.7 s)',4:'STOP inference reported invalid data',5:'STOP verification timed out (20 s)',6:'Junction crossing did not finish',7:'Legacy lift fault (not used by Test 1 search)',8:'Gyroscope unavailable during search or crossing',9:'Line recovery exceeded its time limit (55 s)',10:'Yaw changed opposite to the search direction',11:'Line not found after up to five recovery attempts',12:'Crossing heading deviated by more than 40 degrees',13:'Unclassified control fault',14:'Obstacle too close for forward reposition'};
function faultMessage(u){const code=Number(u.FC),stage=names[u.FS]||'unknown stage';return Number.isInteger(code)&&code>0?`FAULT ${code} — ${faultReasons[code]||'Unknown fault'} | From: ${stage}. Stop, correct the cause, then restart.`:'FAULT — detailed reason unavailable; upload matching UNO Test 1. Stop before restarting.';}
async function poll(){try{const d=JSON.parse(await request('/traffic-status'));latest=d;showObjects(d.objects||{});boxObjects=d.objects||{};boxesAt=Date.now();drawBoxes();showSavedCalibration(d);const vh=d.vision_health||{};$('visionHealth').textContent=`Colour frame age: ${vh.age_ms??'—'} ms | Red regions: ${vh.red_regions??'—'} | Green regions: ${vh.green_regions??'—'} | Colour processing: ${vh.processing_ms??'—'} ms | STOP inference: ${d.objects?.stop?.inference_ms??'—'} ms`;const u=d.uno||{},c=d.settings||{};$('runState').textContent=d.age_ms<1500?(Number(u.UV)===32?(names[u.TS]||'Waiting for UNO'):'UNO VERSION MISMATCH — upload Test 3 UNO'):'UNO disconnected';$('poseInfo').textContent=`Predicted X=${u.KX??'—'} cm, Y=${u.KY??'—'} cm, heading=${u.KH??'—'}° | Gyro-aided X=${u.X??'—'} cm Y=${u.Y??'—'} cm heading=${u.H??'—'}° | PWM L=${u.PL??'—'}, R=${u.PR??'—'} | E-stop=${u.ES?'LATCHED':'clear'} (${u.ER??0}) | Gyro=${u.GY?'OK':'unavailable'} | FR=${u.FR??'—'} B`;$('floor').textContent=`Floor L ${u.L} · M ${u.C} · R ${u.R} | Gyro ${u.GY?'OK':'unavailable'} | Yaw ${u.YA}° | Search: ${u.SE?['right (up to 120°)','left (240° total)','return right toward start','tiny forward nudge (if clear)'][u.SP]||'unknown':'idle'} (${u.SM?'timed fallback':'gyro when available'})`;$('applied').textContent=settingsBusy?'Applying settings…':c.applied?`UNO confirmed: stop on red ${c.RS?'ON':'OFF'} | Cruise PWM ${c.CS} | Reduced PWM ${c.DS} | Red threshold ${c.RP} px`:'Waiting for UNO mode / speed confirmation';$('detect').checked=!!d.detect_enabled;$('color').checked=!!d.color_enabled;if(u.TS===8){driving=false;$('message').textContent=faultMessage(u);}if(scannedToken!==null&&sampleResolution!==d.framesize)resetSample();}catch(e){showObjects({});boxObjects={};drawBoxes();$('runState').textContent='Disconnected — autonomous run may continue';}finally{setTimeout(poll,500);}}
async function heartbeat(){try{if(manualHeld)await post('/cmd',{action:'ping'});}catch(e){driving=false;manualHeld=false;$('message').textContent='Manual control connection lost; manual driving will time out.';}finally{setTimeout(heartbeat,300);}}
let boxObjects={},boxesAt=0;
function drawBoxes(){
 const layer=$('boxes');layer.innerHTML='';
 if(previewPaused||!previewStamp||Date.now()-previewLastShown>1500||Date.now()-boxesAt>1500)return;
 for(const k of ['red','green','stop']){const v=boxObjects[k];
  if(!v?.box_valid||!(v.image_w>0&&v.image_h>0&&v.w>0&&v.h>0))continue;
  const age=Math.abs((Number(previewStamp)-Number(v.captured))|0);if(age>(k==='stop'?5000:1200))continue;
  const x=Math.max(0,v.x),y=Math.max(0,v.y),w=Math.min(v.w,v.image_w-x),h=Math.min(v.h,v.image_h-y);if(w<=0||h<=0)continue;
  const el=document.createElement('div');el.style.cssText=`position:absolute;left:${100*x/v.image_w}%;top:${100*y/v.image_h}%;width:${100*w/v.image_w}%;height:${100*h/v.image_h}%;border:2px solid ${k==='red'?'#ff4545':k==='green'?'#39ff88':'#ffcf40'};color:white;font:bold 12px sans-serif;box-sizing:border-box`;
  const label=document.createElement('span');label.textContent=k==='stop'?(v.fomo_cell?'STOP (FOMO cell)':'STOP'):k.toUpperCase();label.style.cssText=`position:absolute;${k==='stop'?'bottom:0':'top:0'};left:0;background:#101722cc;white-space:nowrap`;el.appendChild(label);layer.appendChild(el);
 }
}
async function refreshPreview(){
 if(previewBusy||previewPaused)return;
 previewBusy=true;clearTimeout(previewTimer);
 const started=Date.now(),controller=new AbortController();
 const timeout=setTimeout(()=>controller.abort(),8000);let nextURL=null;
 try{
  const r=await fetch(`http://${location.hostname}:81/preview?after=${encodeURIComponent(previewStamp)}&t=${started}`,{cache:'no-store',signal:controller.signal});
  if(r.status===204){if(previewLastShown&&Date.now()-previewLastShown>2000)$('previewState').textContent='Waiting for a new frame — last image retained';return;}
  if(!r.ok)throw Error(`Camera temporarily unavailable (${r.status})`);
  const blob=await r.blob();if(!blob.size)throw Error('Empty camera frame');
  nextURL=URL.createObjectURL(blob);
  // Decode off-screen. Failed/incomplete JPEGs never replace the visible frame.
  const probe=new Image();
  await new Promise((resolve,reject)=>{probe.onload=resolve;probe.onerror=()=>reject(Error('Incomplete camera frame'));probe.src=nextURL;});
  if(previewPaused)return;
  const old=previewURL;$('live').src=nextURL;previewURL=nextURL;nextURL=null;
  if(old)URL.revokeObjectURL(old);
  previewStamp=r.headers.get('X-Frame-Stamp')||'';previewLastShown=Date.now();drawBoxes();
  const age=Number(r.headers.get('X-Frame-Age')||0)+Date.now()-started;
  $('previewState').textContent=age>2000?'Delayed camera frame — updating…':'Live camera';
 }catch(e){if(!previewPaused)$('previewState').textContent=previewURL?'Preview paused — last image retained; retrying…':'Waiting for first camera frame…';}
 finally{if(nextURL)URL.revokeObjectURL(nextURL);clearTimeout(timeout);previewBusy=false;if(!previewPaused)previewTimer=setTimeout(refreshPreview,Math.max(250,Math.min(1500,Date.now()-started)));}
}
function previewAge(){drawBoxes();if(!previewPaused&&previewLastShown&&Date.now()-previewLastShown>3000)$('previewState').textContent='Preview paused — last image retained; retrying…';setTimeout(previewAge,1000);}
function togglePreview(){previewPaused=!previewPaused;drawBoxes();$('togglePreview').textContent=previewPaused?'Start preview':'Pause preview';clearTimeout(previewTimer);if(previewPaused)$('previewState').textContent='Preview paused by you — detection continues';else{previewStamp='';refreshPreview();}}
function startPreview(){refreshPreview();previewAge();}
const cameraNumbers=['framesize','wb_mode','brightness','contrast','saturation','quality','ae_level','aec_value','agc_gain','gainceiling','special_effect','view_mode','threshold'];
const cameraFlags=['colorbar','awb','awb_gain','aec','aec2','agc','vflip','hmirror','bpc','wpc','raw_gma','lenc','dcw'];
async function loadCamera(){try{const d=JSON.parse(await request('/status'));for(const k of cameraNumbers)if(d[k]!==undefined)$(k).value=d[k];for(const k of cameraFlags)if(d[k]!==undefined)$(k).checked=!!d[k];if(d.uptime_ms!==undefined)$('cameraInfo').textContent=`Uptime: ${Math.floor(d.uptime_ms/1000)} s | Last camera frame: ${d.preview_age_ms>=0?d.preview_age_ms+' ms ago':'unavailable'} | Frames: ${d.camera_frames??0} | Capture errors: ${d.camera_errors??0} | Sensor ID: ${d.sensor_pid}`;}catch(e){$('message').textContent=e.message;}}
async function applyCamera(key,value){for(let n=0;n<25;n++){try{return await post('/control',{var:key,val:value});}catch(e){if(e.status!==503||n===24)throw e;await delay(200);}}}
async function setCamera(key,value){if(cameraBusy){$('message').textContent='Wait for the current camera change to finish.';return;}cameraBusy=true;driving=false;resetSample();try{$('message').textContent=await applyCamera(key,value);if(key==='framesize'){previewStamp='';let completed=false;for(let n=0;n<40;n++){await delay(400);const d=JSON.parse(await request('/resolution-status'));if(d.state==='applied'||d.state==='failed'){$('message').textContent=d.message;completed=true;break;}}if(!completed)throw Error('Resolution is still pending; read camera settings before trying again.');}await loadCamera();}catch(e){$('message').textContent=e.message;}finally{cameraBusy=false;}}
async function restoreCameraDefaults(){if(cameraBusy)return;cameraBusy=true;driving=false;manualHeld=false;resetSample();$('restoreDefaults').disabled=true;try{$('message').textContent=await post('/camera-defaults');$('previewState').textContent='ESP32 restarting — reconnect Wi-Fi and refresh';previewStamp='';}catch(e){$('message').textContent=e.message;}finally{cameraBusy=false;$('restoreDefaults').disabled=false;}}
async function setDetection(action,value){driving=false;resetSample();try{$('message').textContent=await post('/cmd',{action,value:value?1:0});}catch(e){$('message').textContent=e.message;}}
async function captureNew(){await stopCar();$('capture').disabled=true;try{const target=JSON.parse(await post('/snapshot')).id;$('captureState').textContent=' Capturing…';for(let n=0;n<60;n++){await delay(500);const d=JSON.parse(await request('/snapshot'));if(d.id>=target){$('jpgLink').href=`/capture?id=${d.id}&download=1`;$('bmpLink').href=`/model-input?id=${d.id}&download=1`;$('downloads').hidden=false;$('captureState').textContent=' Ready';return;}}throw Error('Capture timed out');}catch(e){$('captureState').textContent=e.message;}finally{$('capture').disabled=false;}}
for(const button of document.querySelectorAll('[data-dir]')){button.addEventListener('pointerdown',async e=>{e.preventDefault();button.setPointerCapture(e.pointerId);manualHeld=true;try{await post('/cmd',{action:'move',dir:button.dataset.dir,speed:$('speed').value});if(manualHeld)driving=true;else await stopCar();}catch(e){manualHeld=false;$('message').textContent=e.message;}});for(const event of ['pointerup','pointercancel','lostpointercapture'])button.addEventListener(event,()=>{if(manualHeld)stopCar();});}
window.addEventListener('blur',()=>{if(manualHeld)stopCar();});
startPreview();loadSettings();loadCamera();poll();heartbeat();
</script></body></html>)HTML";
