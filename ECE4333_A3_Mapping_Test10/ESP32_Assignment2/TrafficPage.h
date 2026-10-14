#pragma once
static const char TRAFFIC_PAGE[] PROGMEM=R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ECE 4333 Robot Car Test10 Mapping</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#101722;color:#edf3fa;font:16px system-ui,sans-serif}main{max-width:960px;margin:auto;padding:18px}header,.actions,nav{display:flex;align-items:center;gap:10px;flex-wrap:wrap}header{justify-content:space-between}h1{font-size:24px;margin:8px 0}h2{font-size:19px;margin:0 0 12px}section,details{background:#1c2939;border:1px solid #304156;border-radius:12px;padding:16px;margin:14px 0}button,input,select{font:inherit;border:1px solid #52677f;border-radius:7px;padding:9px;background:#26394e;color:#fff}button{cursor:pointer}button:disabled{opacity:.5;cursor:default}button.primary{background:#287b67}.danger{background:#bd3b3b}nav{margin:14px 0}nav button[aria-selected=true]{background:#287b67;border-color:#55baa2}label{display:block;margin:12px 0}input[type=number]{width:100px}input[type=checkbox]{width:auto}p{line-height:1.5;margin:10px 0}.hint,small{color:#b5c5d8}#message,#cal{color:#ffcf91;min-height:24px;overflow-wrap:anywhere}#live{display:block;width:100%;height:auto;min-height:180px;background:#080d15;border-radius:8px}table{width:100%;border-collapse:collapse;font-size:14px}td,th{padding:10px 5px;text-align:left;border-bottom:1px solid #354459}th{color:#b5c5d8;font-weight:500}summary{cursor:pointer}a{color:#8cd7ff}.tablewrap{overflow:auto}#savedCal{white-space:pre-wrap;font-size:14px;line-height:1.7}.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}.drivepad{display:grid;grid-template-columns:repeat(3,65px);gap:7px;margin:10px 0}.drivepad button{touch-action:none;user-select:none}[hidden]{display:none!important}@media(max-width:620px){.grid{grid-template-columns:1fr}main{padding:12px}td,th{padding:9px 3px;font-size:12px}}
</style></head><body><main>
<header><h1>Robot Car <small>Test10 mapping</small></h1><span id="runState">Connecting…</span></header>
<div class="actions"><button class="danger" onclick="stopCar()">STOP CAR</button><button class="danger" onclick="emergencyStop()">EMERGENCY STOP (latched)</button><button onclick="clearEStop()">Reset E-stop</button><button class="primary" onclick="startCar()">Start continuous run</button></div>
<p class="hint">Browser stop needs Wi-Fi. Reset only clears latch; it does NOT restart the car. Keep the physical power switch accessible.</p>
<!--MAPSECTION--><section id="mapSection"><h2>Road mapping / 道路测图 (Test10)</h2>
<p class="hint">This tablet records EVERY line the car sends over Wi-Fi while this page is open (also after a reload: data is kept on the tablet and the car buffers ~3 min). Keep the screen on. 本页打开期间平板自动记录全部数据，断线约3分钟内可自动补齐；请保持屏幕常亮。</p>
<p id="mapGyro" style="font-weight:600">Gyro: waiting…</p>
<div style="display:flex;gap:18px;align-items:center;flex-wrap:wrap;margin:8px 0">
<svg width="128" height="128" viewBox="-64 -64 128 128" aria-label="heading dial"><circle r="58" fill="#0b1220" stroke="#4a5b70" stroke-width="2"/>
<text y="-44" text-anchor="middle" font-size="11" fill="#9fb3c8">0°</text><text x="-46" y="4" text-anchor="middle" font-size="11" fill="#9fb3c8">+90</text><text x="46" y="4" text-anchor="middle" font-size="11" fill="#9fb3c8">-90</text>
<g id="gyroNeedle"><polygon points="0,-48 7,4 0,-2 -7,4" fill="#ff6b6b"/><polygon points="0,30 5,4 0,-2 -5,4" fill="#6c7f95"/></g><circle r="4" fill="#edf3fa"/></svg>
<div><div id="gyroYaw" style="font-size:30px;font-weight:700">— °</div>
<div id="gyroDetail" class="hint">heading — · rate — °/s</div>
<div class="actions"><button id="gyroZeroBtn" onclick="mapZeroGyro()">Zero + recalibrate gyro 陀螺仪归零 (car still)</button></div>
<p id="gyroMsg" class="hint">Turn the car LEFT by hand: yaw must INCREASE. 用手把车向左转，读数应增大；否则改 Gyro Z sign。</p></div></div>
<p id="mapStatus" role="status">Waiting for the car…</p>
<label>Map of <select id="mapRun" onchange="mapPickRun(this.value)"><option value="live">Latest run (live)</option></select></label>
<div id="mapView"></div>
<div class="actions"><button class="primary" onclick="mapDownload('csv')">Download data CSV 数据</button><button onclick="mapDownload('raw')">Download all lines 全部原始数据</button></div>
<div class="actions"><button onclick="mapDownload('png')">Map PNG 地图图片</button><button onclick="mapDownload('svg')">Map SVG</button><button onclick="mapDownload('seg')">Segments CSV 分段表</button><button onclick="mapDownload('points')">Track points CSV 轨迹点</button><button onclick="mapDownload('summary')">Summary TXT 摘要</button></div>
<pre id="mapSummary" style="white-space:pre-wrap;font-size:13px"></pre>
<div class="tablewrap"><table id="mapSegTable"></table></div>
<details><summary>Track lengths (cm) 赛道实测长度</summary>
<p class="hint">Measured centre-line length of each straight, corner to corner. A-B and B-C are estimated from the drawing; measure them. Saved on this tablet.</p>
<label>C-O-D <input id="mapLen0" type="number" min="1" max="2000" step="0.1"></label> <label>D-E <input id="mapLen1" type="number" min="1" max="2000" step="0.1"></label> <label>E-O-A <input id="mapLen2" type="number" min="1" max="2000" step="0.1"></label> <label>A-B <input id="mapLen3" type="number" min="1" max="2000" step="0.1"></label> <label>B-C <input id="mapLen4" type="number" min="1" max="2000" step="0.1"></label>
<div class="actions"><button onclick="mapSaveRef()">Save lengths</button><button onclick="mapResetRef()">Drawing values</button></div></details>
<div class="actions"><button class="danger" onclick="mapClear()">Delete recording from tablet 清除平板上的记录</button></div>
<p class="hint">Map: grey = assignment drawing; blue = line following, orange = crossing heading-hold, red = corner pivot, purple = search; black dots = all three sensors black (crossing); teal squares = detected corners; red ring = car now. Start each run at corner C facing the STOP sign. Distances are scaled automatically from complete straights.</p>
</section><!--/MAPSECTION-->
<section><h2>Old snapshot table (not needed for mapping)</h2>
<label><input type="checkbox" id="scanEnabled"> Record a separate route scan when Start is pressed</label>
<div class="actions"><button onclick="finishScan()">Finish scan and stop car</button><button onclick="downloadLog()">Download selected CSV</button></div>
<label>Scan session <select id="scanSession" onchange="renderScan()"></select></label><p id="logStatus">No scans yet</p>
<p class="hint">Follow the complete figure-eight route, then press Finish scan. This records the route you drive; it cannot determine whether every path was visited. Keep this page open and Wi-Fi connected for data collection. Wi-Fi loss does not stop autonomous driving, but missing samples cannot be recovered. Download each table before reloading. Up to 40,000 rows per scan and 5 scans per page. Coordinates are estimates, not measurements.</p>
<div style="overflow:auto;max-height:300px"><table id="scanTable"></table></div>
<label>Line controller <select id="teamPD" onchange="setTeamPD(this.value)"><option value="0">Test 9 line recovery (gyro optional)</option><option value="1">Teammate PD (experimental)</option></select></label><p id="controllerStatus">Waiting for UNO</p><p class="hint">Change controller while stopped. Both modes retain Test 2 traffic decisions and use Test 9 recovery. Both black-reading polarities are supported after floor calibration.</p></section>
<p id="poseInfo" class="hint">Awaiting position telemetry…</p>
<section><h2>Estimated trajectory</h2><canvas id="posePlot" width="760" height="360" style="width:100%;background:#101722"></canvas><p id="poseQuality">Place the car at the traffic light facing forward before Start. Each Start resets the origin.</p><p class="hint">Blue: wheel-command prediction. Green: gyro-aided estimate. +X forward, +Y left. Calibrate PoseConfig.h before interpreting these estimates as centimetres. No coordinate destination or manual position correction.</p><button onclick="clearPosePlot()">Clear display</button></section>
<p id="message" role="status"></p><p id="visionHealth" class="hint"></p>
<section><div style="position:relative"><img id="live" width="320" height="240" alt="Live camera — connecting"><div id="boxes" style="position:absolute;inset:0;pointer-events:none"></div></div><small id="previewState">Connecting camera…</small> <button id="togglePreview" onclick="togglePreview()">Pause preview</button>
<div class="tablewrap"><table><thead><tr><th>Target</th><th>Detected</th><th>Pixels</th><th>Width (px)</th><th>Distance (cm)</th></tr></thead><tbody>
<tr><td>Red</td><td id="redSeen">—</td><td id="redPixels">—</td><td id="redWidth">—</td><td id="redDistance">—</td></tr>
<tr><td>Green</td><td id="greenSeen">—</td><td id="greenPixels">—</td><td id="greenWidth">—</td><td id="greenDistance">—</td></tr>
<tr><td>STOP sign</td><td id="stopSeen">—</td><td id="stopPixels">—</td><td id="stopWidth">—</td><td id="stopDistance">—</td></tr>
</tbody></table></div><small>Pixels = coloured area; width is used for distance. STOP uses its matched red region after AI confirmation. “—” means unavailable or not calibrated.</small></section>
<nav role="tablist"><button id="carTab" role="tab" aria-selected="true" aria-controls="carPanel" onclick="switchTab('car')">Car control</button><button id="cameraTab" role="tab" aria-selected="false" aria-controls="cameraPanel" onclick="switchTab('camera')">Camera & calibration</button></nav>
<div id="carPanel" role="tabpanel">
<section><h2>Optional gyro navigation</h2><label><input id="gyroNav" type="checkbox" onchange="setGyroNavigation(this.checked?1:0)">Enable gyro-assisted search and heading hold</label><button id="zeroYaw" onclick="setGyroNavigation(2)">Zero heading and position (stopped)</button><p id="gyroNavStatus">Waiting for UNO confirmation. Default: OFF.</p><p class="hint">OFF: sensor-based line tracking and timed search using the saved angles; gyro readings cannot trigger FAULT 10 or steer the wheels. ON: measured search angles, direction gate and heading hold. Stop before changing. Enabling resets the local origin and heading; Start also resets them. Zero does not recalibrate sensor bias: keep the car still during power-on. There is no automatic map/waypoint navigation.</p></section>
<section><h2>Continuous figure-eight</h2><p>Start follows the line without colour calibration. Uncalibrated red slows the car but keeps it moving. Calibrated red stops within the chosen distance and waits for green; green needs no distance calibration. Confirmed STOP signs pause for 3 seconds, then continue. No lap limit.</p><p class="hint">Autonomous running continues after Wi-Fi disconnects or this page closes. STOP requires a working connection; local faults still stop the car. Manual driving retains its connection timeout.</p>
<label>Autonomous cruise speed (PWM 50–150) <input id="CS" onchange="$('DS').value=Math.min(Number($('DS').value),Number(this.value));saveSettings()" type="number" min="50" max="150" value="60"></label>
<label>Reduced speed (PWM 0 to cruise speed) <input id="DS" onchange="saveSettings()" type="number" min="0" max="150" value="40"></label>
<label>Red action threshold (pixels) <input id="RP" onchange="saveSettings()" type="number" min="0" max="160000" value="100"></label>
<p class="hint">Stop / slow only when the detected red area reaches this value. Use the Red Pixels reading below the preview, not width. Zero adds no extra area limit; the detector minimum still applies. Reduced speed 0 stops on red in the original mode too. After STOP is confirmed, crossing uses cruise speed if reduced speed is 0.</p>
<label><input id="RS" type="checkbox" onchange="saveSettings()">Stop on red, then scan for STOP</label>
<p class="hint">Enabled: stop on a fresh red detection only when Red Pixels reaches the saved threshold, regardless of distance calibration. Two accumulated STOP frames release the car to cross; missed frames do not reset the count. Scanning continues while stopped. Plain red or an uncertain scan keeps it stopped; two distinct valid green-only observations within six seconds release either the red verification hold or the red-light wait. Disabled: use the original distance-based behaviour above. Changing this option or cruise speed stops the car and saves automatically. Wait for UNO confirmed, then press Start again. Try PWM 60 for a slower cruise.</p>
<label>Stopping distance (cm) <input id="SC" type="number" min="5" max="150" value="25"></label>
<details><summary>Line and search settings</summary><p id="floor">Waiting for floor sensors…</p>
<label>Black range <input id="BL" type="number" min="0" max="1022" value="600"> to <input id="BH" type="number" min="1" max="1023" value="1023"></label>
<div class="actions"><button onclick="sampleFloor('white')">Sample white floor</button><button onclick="sampleFloor('black')">Sample black line</button><button onclick="applyFloor()">Apply measured threshold</button></div><p id="floorCal">Stop the car. Place ALL three sensors over each surface in turn, at normal ride height. Each sample collects 6 fresh readings.</p>
<label>First right search (degrees) <input id="RA" type="number" min="30" max="360" value="180"></label>
<label>Reverse-left sweep (degrees) <input id="SA" type="number" min="60" max="360" value="360"></label>
<label>Minimum straight crossing (ms) <input id="CT" type="number" min="300" max="5000" value="1500"></label>
<label>Gyro Z sign <select id="GS"><option value="0">Negative raw Z = left</option><option value="1">Positive raw Z = left</option></select></label>
<small>Choose a black range that excludes the grey floor. Search: right RA degrees (30–360), then reverse left SA degrees (60–360, at least RA). If no line is found, return right by SA minus RA degrees to the original heading before a clearance-checked nudge. A centre line must remain visible for 60 ms, then survive a 220 ms low-speed handover. Left+centre steers left. A brief line loss first follows the last correction. Opposite-direction reacquisition is rejected when the gyro is healthy. With a working gyro, left turns must increase yaw. Without a gyro, scans use approximate motor-on times, not measured angles.</small></details>
<button onclick="saveSettings()">Save car settings</button><p id="applied" class="hint"></p></section>
<details><summary>Manual movement</summary><label>Speed <input id="speed" type="number" value="60" min="0" max="180"></label><p class="hint">Hold an arrow to move; release to stop.</p><div class="drivepad"><span></span><button data-dir="1">↑</button><span></span><button data-dir="3">←</button><button class="danger" onclick="stopCar()">■</button><button data-dir="4">→</button><span></span><button data-dir="2">↓</button></div></details>
</div>
<div id="cameraPanel" role="tabpanel" hidden>
<section><h2>Distance calibration</h2><p>Stop the car. Select a target, scan its width, enter the measured camera-to-target distance, then save. Keep the target still. STOP requires AI confirmation.</p>
<label>Target <select id="profile" onchange="resetSample()"><option value="light">Red</option><option value="green">Green</option><option value="stop">STOP sign</option></select></label>
<label>Scanned width (px) <input id="width" readonly> / image <input id="image_w" readonly></label>
<label>Measured distance (cm) <input id="cm" type="number" min="5" max="300" value="30"></label>
<button onclick="sampleWidth()">Scan target width</button><button id="saveCal" onclick="calibrate()" disabled>Save measured distance</button><button onclick="clearCalibration()">Clear selected target</button><p id="cal" role="status"></p><p id="sampleInfo" class="hint"></p><div id="savedCal">Loading calibration…</div></section>
<section><h2>Camera settings · SVGA startup</h2><p class="hint">Initializes at high resolution, then starts capture at SVGA 800×600. Verify distance calibration on your hardware. JPEG quality 10, two frame buffers and the uploaded camera orientation are retained. Camera white balance uses driver defaults. Changing settings stops the car.</p>
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

let gyroNavBusy=false;
async function setGyroNavigation(value){
 if(gyroNavBusy)return;gyroNavBusy=true;$('gyroNav').disabled=$('zeroYaw').disabled=true;
 try{
  const before=JSON.parse(await request('/traffic-status'));
  if(before.age_ms>=1500||before.uno?.M!==0)throw Error('Press STOP CAR and wait for OFF first.');
  await post('/cmd',{action:'gyro_navigation',value});let confirmed=false;
  for(let i=0;i<15;i++){await delay(250);const d=JSON.parse(await request('/traffic-status')),u=d.uno||{};
   if(d.age_ms<1500&&u.UV===40&&u.M===0&&(value===0?u.GN===0:(u.ZR!==before.uno.ZR&&(value===2||u.GN===1)))){
    $('gyroNav').checked=!!u.GN;
    $('gyroNavStatus').textContent=value===2?'UNO confirmed: heading and position zeroed.':('UNO confirmed: gyro navigation '+(u.GN?'ON, origin reset':'OFF'));
    if(value>0)clearPosePlot();confirmed=true;break;
   }
  }
  if(!confirmed)throw Error('UNO confirmation missing; do not assume the option changed.');
 }catch(e){$('gyroNavStatus').textContent=e.message;}
 finally{gyroNavBusy=false;$('gyroNav').disabled=$('zeroYaw').disabled=false;}
}
let floorSamples={},floorSampling=false;
async function sampleFloor(kind){
 if(floorSampling)return;floorSampling=true;
 try{const rows=[];let prev=null;
 for(let n=0;n<30&&rows.length<6;n++){
  const d=JSON.parse(await request('/traffic-status')),u=d.uno||{};
  if(d.age_ms>=1500||u.M!==0||u.PL!==0||u.PR!==0)throw Error('Stop the car and wait for fresh UNO OFF telemetry.');
  if(u.UT!==prev&&[u.L,u.C,u.R].every(Number.isFinite)){rows.push([u.L,u.C,u.R]);prev=u.UT;}
  if(rows.length<6)await delay(450);
 }
 if(rows.length<6)throw Error('Not enough fresh sensor samples.');
 floorSamples[kind]=rows.flat();$('floorCal').textContent=kind+' captured: '+Math.min(...floorSamples[kind])+' to '+Math.max(...floorSamples[kind]);
 }catch(e){$('floorCal').textContent=e.message;}finally{floorSampling=false;}
}
function floorThreshold(w,b){
 if(!w?.length||!b?.length)throw Error('Sample both surfaces first.');
 const wMin=Math.min(...w),wMax=Math.max(...w),bMin=Math.min(...b),bMax=Math.max(...b);
 if(bMin-wMax>=60)return [Math.round((bMin+wMax)/2),1023];
 if(wMin-bMax>=60)return [0,Math.round((wMin+bMax)/2)];
 throw Error('Black/white readings overlap or have less than 60 ADC counts separation. Check height, ambient light and each sensor; a single common threshold cannot separate these samples.');
}
async function applyFloor(){try{const [low,high]=floorThreshold(floorSamples.white,floorSamples.black);$('BL').value=low;$('BH').value=high;if(await saveSettings())$('floorCal').textContent='Saved black range '+low+' to '+high+'. Press Start after UNO confirmation.';}catch(e){$('floorCal').textContent=e.message;}}
let posePoints=[],poseRun=null,poseUT=null;
function clearPosePlot(){posePoints=[];drawPose();}
function updatePose(d){const u=d.uno||{};if(d.age_ms>=1500){$('poseQuality').textContent='Telemetry stale; plot frozen.';return;}if(u.UT===poseUT)return;if(poseRun!==u.RN||(poseUT!==null&&u.UT<poseUT))posePoints=[];poseRun=u.RN;poseUT=u.UT;
 if(u.M===6&&[u.X,u.Y,u.KX,u.KY].every(Number.isFinite)){posePoints.push([u.X,u.Y,u.KX,u.KY]);if(posePoints.length>3000)posePoints.shift();}
 $('poseQuality').textContent='Run '+u.RN+' | '+(u.GY?'Gyro available':'Gyro unavailable: wheel model only')+' | Estimated X '+u.X+' cm, Y '+u.Y+' cm, heading '+u.H+' deg. Position accuracy depends on calibration.';drawPose();}
function drawPose(){const c=$('posePlot'),g=c.getContext('2d');g.clearRect(0,0,c.width,c.height);const all=[[0,0],...posePoints.flatMap(p=>[[p[0],p[1]],[p[2],p[3]]])];let xmin=Math.min(...all.map(p=>p[0])),xmax=Math.max(...all.map(p=>p[0])),ymin=Math.min(...all.map(p=>p[1])),ymax=Math.max(...all.map(p=>p[1]));const scale=Math.min((c.width-60)/Math.max(100,xmax-xmin),(c.height-60)/Math.max(100,ymax-ymin));const xy=p=>[30+(p[0]-xmin)*scale,c.height-30-(p[1]-ymin)*scale];
 function path(points,color){if(!points.length)return;g.strokeStyle=color;g.lineWidth=2;g.beginPath();points.forEach((p,i)=>{const a=xy(p);i?g.lineTo(...a):g.moveTo(...a);});g.stroke();}
 path(posePoints.map(p=>[p[2],p[3]]),'#68afff');path(posePoints.map(p=>[p[0],p[1]]),'#54e2aa');g.fillStyle='#eee';g.font='14px sans-serif';g.fillText('X forward / Y left; cm',15,20);const o=xy([0,0]);g.fillRect(o[0]-3,o[1]-3,6,6);g.fillText('Origin',o[0]+6,o[1]-6);
}
let controllerBusy=false;
async function setTeamPD(value){controllerBusy=true;try{await post('/cmd',{action:'team_pd',value});for(let i=0;i<12;i++){await delay(300);const d=JSON.parse(await request('/traffic-status'));if(d.age_ms<1500&&Number(d.uno?.PD)===Number(value)){$('message').textContent='Controller confirmed. Press Start separately.';return;}}throw Error('Controller confirmation missing');}catch(e){$('message').textContent=e.message;}finally{controllerBusy=false;}}
// SCAN_LOG_BEGIN
let scanRuns=[],activeScan=null,scanStarting=false,scanFinishing=false;
function flattenRecord(value,prefix='',out={}){
 if(value!==null&&typeof value==='object'&&!Array.isArray(value)){
  for(const [k,v] of Object.entries(value))flattenRecord(v,prefix?prefix+'.'+k:k,out);
 }else out[prefix]=typeof value==='object'?JSON.stringify(value):value;
 return out;
}
function newScan(){
 if(activeScan)throw Error('Finish the current scan first.');
 if(scanRuns.length>=5)throw Error('Download all scans, then reload to start another.');
 const run={id:Date.now()+'_'+(scanRuns.length+1),rows:[],status:'starting',started:Date.now(),lastUT:null,lastRN:null,lastSensorUT:null,segment:1};
 scanRuns.push(run);activeScan=run;
 const opt=document.createElement('option');opt.value=String(scanRuns.length-1);opt.textContent='Scan '+scanRuns.length+' — '+new Date().toLocaleTimeString();$('scanSession').appendChild(opt);$('scanSession').value=opt.value;renderScan();return run;
}
function appendScan(row){
 const run=activeScan;if(!run)return;
 if(run.rows.length>=40000){run.status='capacity reached — incomplete';activeScan=null;renderScan();return;}
 run.rows.push({scan_id:run.id,host_iso:new Date().toISOString(),elapsed_s:(Date.now()-run.started)/1000,segment:run.segment,...row});renderScan();
}
function recordSample(d){
 const run=activeScan;if(!run)return;
 const u=d.uno||{},q=d.sensors||{},fresh=d.age_ms<1500&&Number.isFinite(u.UT);
 const changed=fresh&&(u.UT!==run.lastUT||u.RN!==run.lastRN);
 if(changed&&run.lastUT!==null&&(u.UT<run.lastUT||u.RN!==run.lastRN))run.segment++;
 appendScan({event:'sample',uno_fresh:+fresh,uno_new:+changed,sensors_fresh:+(d.sensors_age_ms<2500&&Number.isFinite(q.UT)),sensors_new:+(Number.isFinite(q.UT)&&q.UT!==run.lastSensorUT),...flattenRecord(d)});
 if(fresh){run.lastUT=u.UT;run.lastRN=u.RN;}run.lastSensorUT=q.UT;
}
function scanGap(message){if(activeScan)appendScan({event:'connection_gap',detail:message,uno_fresh:0,uno_new:0,sensors_fresh:0,sensors_new:0});}
function renderScan(){
 const run=scanRuns[Number($('scanSession').value)];if(!run)return;
 $('logStatus').textContent=run.status+' | '+run.rows.length+' rows | preview: last 20 rows. CSV contains all available fields.';
 const fields=['elapsed_s','event','uno.UT','uno.TS','uno.L','uno.C','uno.R','sensors.U','sensors.V','uno.YA','sensors.GR10','uno.PL','uno.PR','objects.red.pixels','objects.green.pixels','objects.stop.seen','uno_fresh'];
 const table=$('scanTable');table.textContent='';let tr=document.createElement('tr');
 for(const key of fields){const th=document.createElement('th');th.textContent=key;tr.appendChild(th);}table.appendChild(tr);
 for(const row of run.rows.slice(-20)){tr=document.createElement('tr');for(const key of fields){const td=document.createElement('td');td.textContent=row[key]??'';tr.appendChild(td);}table.appendChild(tr);}
}
function scanCSV(run){
 const names=new Set();for(const row of run.rows)for(const key of Object.keys(row))names.add(key);const cols=[...names];
 const quote=v=>'"'+String(v??'').replace(/"/g,'""')+'"';
 return '\ufeff'+[cols,...run.rows.map(row=>cols.map(k=>row[k]))].map(row=>row.map(quote).join(',')).join('\r\n');
}
function downloadLog(){const run=scanRuns[Number($('scanSession').value)];if(!run?.rows.length){$('logStatus').textContent='No recorded rows to download.';return;}const url=URL.createObjectURL(new Blob([scanCSV(run)],{type:'text/csv;charset=utf-8'}));const a=document.createElement('a');a.href=url;a.download='route_scan_'+run.id+'.csv';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);}
async function finishScan(){
 if(scanFinishing)return;scanFinishing=true;
 try{
  await post('/cmd',{action:'stop'});let stopped=false;
  for(let i=0;i<16;i++){await delay(300);const d=JSON.parse(await request('/traffic-status'));recordSample(d);const u=d.uno||{};
   if(d.age_ms<1500&&u.M===0&&u.PL===0&&u.PR===0){stopped=true;break;}}
  if(!stopped)throw Error('UNO stop confirmation missing');
  driving=false;if(activeScan){const finished=activeScan;appendScan({event:'finish_confirmed',detail:'Route completion confirmed by operator; UNO stopped.'});if(activeScan)finished.status='finished — operator confirmed';activeScan=null;renderScan();}
  $('message').textContent='UNO stopped. Select the scan and download its CSV table.';
 }catch(e){scanGap(e.message);$('message').textContent='Scan finish NOT confirmed: '+e.message+'. Autonomous driving may continue. Use physical stop if needed. Partial CSV remains available.';}
 finally{scanFinishing=false;}
}
window.addEventListener('beforeunload',e=>{if(scanRuns.some(r=>r.rows.length)){e.preventDefault();e.returnValue='';}});
// SCAN_LOG_END

const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function request(url,data){const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),4000);try{const r=await fetch(url,{cache:'no-store',signal:controller.signal,...(data===undefined?{}:{method:'POST',body:new URLSearchParams(data)})});const t=await r.text();if(!r.ok){const e=Error(t);e.status=r.status;throw e;}return t;}finally{clearTimeout(timer);}}
async function post(url,data={}){return request(url,data);}
function switchTab(name){currentTab=name;for(const t of ['car','camera']){$(t+'Panel').hidden=t!==name;$(t+'Tab').setAttribute('aria-selected',String(t===name));}}
async function stopCar(){driving=false;manualHeld=false;try{$('message').textContent=await post('/cmd',{action:'stop'});}catch(e){$('message').textContent='STOP was not delivered. Autonomous running may continue; use the physical power switch if needed.';}}
async function emergencyStop(){driving=false;manualHeld=false;try{$('message').textContent='Emergency stop command sent. Confirm ES=1 and PWM 0 on this page.';await post('/cmd',{action:'estop'});}catch(e){$('message').textContent='E-stop communication failed! Use physical power switch.';}}
async function clearEStop(){driving=false;try{await post('/cmd',{action:'clear_estop'});$('message').textContent='Reset requested. Confirm ES=0 and OFF before pressing Start.';}catch(e){$('message').textContent=e.message;}}
async function startCar(){if(scanStarting||scanFinishing||settingsBusy){$('message').textContent='Wait for the pending command.';return;}
 if(activeScan){$('message').textContent='Finish the current scan before restarting.';return;}
 scanStarting=true;let run=null;
 try{const wanted=readSettingsForm(),current=JSON.parse(await request('/traffic-settings'));
 if(!settingsMatch(current,wanted)&&!await saveSettings())return;
 const before=JSON.parse(await request('/traffic-status'));
 if($('scanEnabled').checked){run=newScan();recordSample(before);}
 await post('/traffic-start');let confirmed=false;
 for(let i=0;i<16;i++){await delay(300);const d=JSON.parse(await request('/traffic-status'));recordSample(d);
 if(d.age_ms<1500&&d.uno?.M===6&&d.uno.RN!==before.uno?.RN){confirmed=true;break;}}
 if(!confirmed)throw Error('Start confirmation missing; inspect car state before retrying.');
 driving=true;if(run){run.status='recording';appendScan({event:'start_confirmed'});}
 $('message').textContent='UNO running'+(run?' — route scan recording.':'');
 }catch(e){if(run){run.status='start uncertain — still recording';appendScan({event:'start_unconfirmed',detail:e.message});}$('message').textContent=e.message;}
 finally{scanStarting=false;renderScan();}}
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
 throw Error('Settings saved but UNO confirmation missing. Do not start; check matching UNO Test 9.');
 }catch(e){$('message').textContent=e.message;return false;}finally{settingsBusy=false;$('RS').disabled=$('CS').disabled=$('DS').disabled=$('RP').disabled=false;}}
function resetSample(){scannedToken=null;sampleResolution=null;$('width').value='';$('image_w').value='';$('saveCal').disabled=true;$('sampleInfo').textContent='Scan the selected target first.';}
async function sampleWidth(){resetSample();try{const d=JSON.parse(await post('/traffic-sample',{profile:$('profile').value}));$('width').value=d.width;$('image_w').value=d.image_w;sampleResolution=d.framesize;scannedToken=d.token;$('saveCal').disabled=false;$('sampleInfo').textContent=`Scanned ${d.width} px. Enter the distance and save.`;}catch(e){$('cal').textContent=e.message;}}
async function calibrate(){try{if(scannedToken===null)throw Error('Scan the target first');$('cal').textContent=await post('/traffic-calibrate',{profile:$('profile').value,cm:$('cm').value,token:scannedToken});scannedToken=null;$('saveCal').disabled=true;}catch(e){$('cal').textContent=e.message;}}
async function clearCalibration(){try{$('cal').textContent=await post('/traffic-clear',{profile:$('profile').value});resetSample();}catch(e){$('cal').textContent=e.message;}}
function showSavedCalibration(d){const c=d.calibration||{},labels={light:'Red',green:'Green',stop:'STOP'};$('savedCal').textContent=['light','green','stop'].map(k=>{const v=c[k];return !v||!v.valid?`${labels[k]}: not calibrated`:`${labels[k]}: saved width ${v.width} px / image ${v.image_w} px, measured distance ${v.distance_cm} cm${v.active?'':' (resolution mismatch — rescan)'}`;}).join('\n');}
function showObjects(objects){for(const k of ['red','green','stop']){const v=objects[k]||{};$(k+'Seen').textContent=!v.valid?'—':v.seen?'Yes':'No';$(k+'Pixels').textContent=v.valid&&v.seen?v.pixels:'—';$(k+'Width').textContent=v.valid&&v.seen&&v.width>0?v.width:'—';$(k+'Distance').textContent=v.valid&&v.seen&&v.distance_cm>=0?v.distance_cm:'—';}}
const faultReasons={1:'Colour data older than 1.5 s',2:'Invalid or ambiguous colour frame',3:'Red target lost during approach (over 0.7 s)',4:'STOP inference reported invalid data',5:'STOP verification timed out (20 s)',6:'Junction crossing did not finish',7:'Legacy lift fault (not used by Test 1 search)',8:'Gyroscope unavailable during search or crossing',9:'Line recovery exceeded its time limit (120 s)',10:'Yaw changed opposite to the search direction',11:'Line not found after up to five recovery attempts',12:'Crossing heading deviated by more than 40 degrees',13:'Unclassified control fault',14:'Obstacle too close for forward reposition'};
function faultMessage(u){const code=Number(u.FC),stage=names[u.FS]||'unknown stage';return Number.isInteger(code)&&code>0?`FAULT ${code} — ${faultReasons[code]||'Unknown fault'} | From: ${stage}. Stop, correct the cause, then restart.`:'FAULT — detailed reason unavailable; upload matching UNO Test 1. Stop before restarting.';}
async function poll(){try{const d=JSON.parse(await request('/traffic-status'));if(latest&&d.esp_ms<latest.esp_ms)$('message').textContent='ESP32 restart detected. UNO state is shown separately; scan has a data gap.';latest=d;updatePose(d);recordSample(d);showObjects(d.objects||{});boxObjects=d.objects||{};boxesAt=Date.now();drawBoxes();showSavedCalibration(d);const vh=d.vision_health||{};$('visionHealth').textContent=`Colour frame age: ${vh.age_ms??'—'} ms | Red regions: ${vh.red_regions??'—'} | Green regions: ${vh.green_regions??'—'} | Colour processing: ${vh.processing_ms??'—'} ms | STOP inference: ${d.objects?.stop?.inference_ms??'—'} ms`;const u=d.uno||{},c=d.settings||{};if(!gyroNavBusy)$('gyroNav').checked=!!u.GN;$('controllerStatus').textContent=`UNO controller: ${u.PD?'Teammate PD':'Test 2'} | E-stop ${u.ES?'LATCHED':'clear'}`;if(!controllerBusy)$('teamPD').value=String(u.PD||0);$('runState').textContent=d.age_ms<1500?(Number(u.UV)===40?(names[u.TS]||'Waiting for UNO'):'UNO VERSION MISMATCH — upload Test10 UNO_Mapping'):'UNO telemetry stale — motor state unknown';$('poseInfo').textContent=`Predicted X=${u.KX??'—'} cm, Y=${u.KY??'—'} cm, heading=${u.KH??'—'}° | Gyro-aided X=${u.X??'—'} cm Y=${u.Y??'—'} cm heading=${u.H??'—'}° | PWM L=${u.PL??'—'}, R=${u.PR??'—'} | E-stop=${u.ES?'LATCHED':'clear'} (${u.ER??0}) | Gyro=${u.GY?'OK':'unavailable'} | FR=${u.FR??'—'} B`;$('floor').textContent=`Floor L ${u.L} · M ${u.C} · R ${u.R} | Gyro ${u.GY?'OK':'unavailable'} | Yaw ${u.YA}° | Search: ${u.SE?[`right (up to ${c.RA??180}°)`,`left (${c.SA??360}° total)`,'return right toward start','tiny forward nudge (if clear)'][u.SP]||'unknown':'idle'} (${u.GN&&u.GY?'gyro measured':'timed search'})`;$('applied').textContent=settingsBusy?'Applying settings…':c.applied?`UNO confirmed: stop on red ${c.RS?'ON':'OFF'} | Cruise PWM ${c.CS} | Reduced PWM ${c.DS} | Red threshold ${c.RP} px`:'Waiting for UNO mode / speed confirmation';$('detect').checked=!!d.detect_enabled;$('color').checked=!!d.color_enabled;if(u.TS===8){driving=false;$('message').textContent=faultMessage(u);}if(scannedToken!==null&&sampleResolution!==d.framesize)resetSample();}catch(e){scanGap(e.message);showObjects({});boxObjects={};drawBoxes();$('runState').textContent='Web connection lost — motor state unknown; reconnect to inspect';}finally{setTimeout(poll,500);}}
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







/*MAPCORE - same algorithm as PC_Mapping/draw_map.py (kept in sync; tested by tests/test_map_core.js)*/
const MAPREF0=[{name:'C-O-D',h:0,len:320},{name:'D-E',h:-136.6,len:151.3},{name:'E-O-A',h:-240,len:220},{name:'A-B',h:-210,len:43.3},{name:'B-C',h:-120,len:125}];
const MAPCOL={1:'#2a7de1',2:'#f08c00',3:'#d6336c',4:'#ae3ec9',5:'#000000'};
function mapDecode(d){const st=d[13],k=d[14];return{q:d[0],t:d[1],L:d[2],C:d[3],R:d[4],PL:d[5],PR:d[6],g:d[7]/10,yaw:d[8]/10,dist:d[9]/10,x:d[10],y:d[11],u:d[12],mode:st>>4,fs:st&7,gy:(st>>3)&1,k:k};}
function mapRuns(S){const runs=[];let cur=[],p=null;
 for(const r of S){if(r.mode!==6){if(cur.length)runs.push(cur);cur=[];p=null;continue;}
  if(p&&(r.q<p.q||r.t<p.t||r.t-p.t>3000)){runs.push(cur);cur=[];}cur.push(r);p=r;}
 if(cur.length)runs.push(cur);return runs.filter(r=>r.length>=5);}
function mapStraight(run,yaw,dist){
 const fwd=run.map(r=>r.PL>0&&r.PR>0&&(r.fs===1||r.fs===2)),out=[],N=run.length;let i=0;
 while(i<N){if(!fwd[i]){i++;continue;}let j=i,sum=yaw[i],c=1;
  while(j+1<N&&fwd[j+1]&&Math.abs(yaw[j+1]-sum/c)<=8){j++;sum+=yaw[j];c++;}
  if(j-i+1>=6)out.push({i0:i,i1:j,head:sum/c,len:dist[j]-dist[i]});i=j+1;}
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
 if(fixedScale){scale=fixedScale;how='fixed';}
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
 const lapEnds=[];for(let k=1;k<segs.length;k++)if(segs[k].label===0&&segs[k-1].label===n-1)lapEnds.push(segs[k].i0);
 const closure=[];let s0=0;
 for(const e of lapEnds){const ex=xs[e],ey=ys[e];closure.push(Math.hypot(ex,ey));const d0=dist[s0],d1=dist[e],dd=d1>d0?d1-d0:1;
  for(let i=s0;i<N;i++){const f=i>=e?1:(dist[i]-d0)/dd;xs[i]-=ex*f;ys[i]-=ey*f;}s0=e;}
 const cnt=b=>run.reduce((a,r)=>a+((r.k>>b)&1),0);
 const yawMoved=run.some(r=>r.yaw!==0),gyOk=run.filter(r=>r.gy).length;
 if(!yawMoved)notes.push('GYRO NOT WORKING: heading stayed exactly 0 for the whole run, so no map can be built and the car cannot recognise corners or the old track. Check the IMU line on this page (IM=0 means no chip found).');
 else if(gyOk===0&&run.some(r=>r.k!==undefined))notes.push('Samples do not carry the gyro flag (older UNO firmware).');
 if(segs.some(g=>!g.inOrder))notes.push('Segments marked (?) are out of the expected C>D>E>A>B order: the car took a wrong branch or reversed there.');
 return{run,t,yaw,yawc,dist,xs,ys,segs,labels,scale,how,gyroTxt,laps:lapEnds.length,closure,notes,gaps,
  counts:{cross:cnt(4),turn:cnt(5),search:cnt(6),allBlack:cnt(3)}};}
function mapRefPoly(ref){const p=[[0,0]];let x=0,y=0;for(const s of ref){const h=s.h*Math.PI/180;x+=s.len*Math.cos(h);y+=s.len*Math.sin(h);p.push([x,y]);}return p;}
function mapSummary(res,ref,runNo,runCount){const L=[];
 L.push('Test10 map summary (run '+runNo+' of '+runCount+', '+res.run.length+' samples, '+res.t[res.t.length-1].toFixed(1)+' s, UNO sample gaps '+res.gaps+')');
 L.push('distance scale: x'+res.scale.toFixed(3)+' ('+res.how+')');
 L.push('  -> measured speed factor ~ '+(0.33*res.scale).toFixed(3)+' cm/s per PWM (UNO uses 0.33; MapConfig.h LEFT/RIGHT_CM_S_PER_PWM)');
 L.push(res.gyroTxt);
 L.push('laps completed: '+res.laps+(res.closure.length?'; closure error before correction: '+res.closure.map(e=>e.toFixed(1)+' cm').join(', '):''));
 L.push('segment order seen: '+(res.segs.map(g=>ref[g.label].name+(g.inOrder?'':'(?)')).join(' > ')||'(none yet)'));
 L.push('crossing holds: '+res.counts.cross+', corner turns: '+res.counts.turn+', searches: '+res.counts.search+', all-black hits: '+res.counts.allBlack);
 res.notes.forEach(s=>L.push('NOTE: '+s));return L.join('\n');}
function mapSegRows(res,ref){return res.segs.map((g,k)=>{const s=ref[g.label];let hs=0;for(let i=g.i0;i<=g.i1;i++)hs+=res.yawc[i];const len=g.len*res.scale;
 return{order:k+1,segment:s.name,complete:g.complete?1:0,ref_heading_deg:s.h,meas_heading_deg:+(hs/(g.i1-g.i0+1)).toFixed(1),ref_length_cm:s.len,meas_length_cm:+len.toFixed(1),
  length_error_cm:g.complete?+(len-s.len).toFixed(1):'',t_start_s:+res.t[g.i0].toFixed(1),t_end_s:+res.t[g.i1].toFixed(1)};});}
function mapCSV(rows){if(!rows.length)return'';const k=Object.keys(rows[0]);return k.join(',')+'\n'+rows.map(r=>k.map(c=>r[c]===undefined||r[c]===null?'':r[c]).join(',')).join('\n')+'\n';}
function mapPointRows(res,ref){return res.run.map((r,i)=>({t_s:+res.t[i].toFixed(2),s_cm:+(res.dist[i]*res.scale).toFixed(1),x_cm:+res.xs[i].toFixed(1),y_cm:+res.ys[i].toFixed(1),
 heading_deg:+res.yawc[i].toFixed(1),segment:res.labels[i]===null?'corner':ref[res.labels[i]].name,follow_state:r.fs,L:r.L,C:r.C,R:r.R,all_black:(r.k>>3)&1,ev_cross:(r.k>>4)&1}));}
function mapSVG(res,ref,title){
 const rp=mapRefPoly(ref),ax=res.xs.concat(rp.map(p=>p[0])),ay=res.ys.concat(rp.map(p=>p[1]));
 const x0=Math.min(...ax)-30,x1=Math.max(...ax)+30,y0=Math.min(...ay)-30,y1=Math.max(...ay)+30,W=900,k=W/(x1-x0),H=Math.round((y1-y0)*k)+70;
 const X=x=>((x-x0)*k).toFixed(1),Y=y=>((y1-y)*k+40).toFixed(1),o=[];
 o.push('<svg xmlns="http://www.w3.org/2000/svg" width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" font-family="sans-serif">');
 o.push('<rect width="100%" height="100%" fill="#ffffff"/>');
 o.push('<text x="10" y="22" font-size="16" fill="#222">'+title+' - scale x'+res.scale.toFixed(2)+' - grid 50 cm</text>');
 for(let g=Math.floor(x0/50)*50;g<=x1;g+=50)o.push('<line x1="'+X(g)+'" y1="40" x2="'+X(g)+'" y2="'+H+'" stroke="#eee"/>');
 for(let g=Math.floor(y0/50)*50;g<=y1;g+=50)o.push('<line x1="0" y1="'+Y(g)+'" x2="'+W+'" y2="'+Y(g)+'" stroke="#eee"/>');
 o.push('<polyline fill="none" stroke="#999" stroke-width="10" stroke-opacity="0.35" stroke-linejoin="round" points="'+rp.map(p=>X(p[0])+','+Y(p[1])).join(' ')+'"/>');
 o.push('<polyline fill="none" stroke="#adb5bd" stroke-width="1" stroke-dasharray="4 3" points="'+res.run.map(r=>X(r.x*res.scale)+','+Y(r.y*res.scale)).join(' ')+'"/>');
 let i=1;while(i<res.xs.length){const c=MAPCOL[res.run[i].fs]||MAPCOL[1],pts=[X(res.xs[i-1])+','+Y(res.ys[i-1])];
  while(i<res.xs.length&&(MAPCOL[res.run[i].fs]||MAPCOL[1])===c){pts.push(X(res.xs[i])+','+Y(res.ys[i]));i++;}
  o.push('<polyline fill="none" stroke="'+c+'" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" points="'+pts.join(' ')+'"/>');}
 res.run.forEach((r,j)=>{if((r.k>>3)&1)o.push('<circle cx="'+X(res.xs[j])+'" cy="'+Y(res.ys[j])+'" r="2.5" fill="#212529"/>');});
 res.segs.slice(1).forEach(g=>o.push('<rect x="'+(X(res.xs[g.i0])-3.5)+'" y="'+(Y(res.ys[g.i0])-3.5)+'" width="7" height="7" fill="#1098ad"/>'));
 const e=res.xs.length-1;o.push('<circle cx="'+X(res.xs[e])+'" cy="'+Y(res.ys[e])+'" r="5" fill="none" stroke="#e03131" stroke-width="2"/>');
 o.push('<circle cx="'+X(0)+'" cy="'+Y(0)+'" r="6" fill="#2b8a3e"/><text x="'+(+X(0)+8)+'" y="'+(+Y(0)+16)+'" font-size="13" fill="#2b8a3e">start C</text>');
 let lx=10;const ly=H-12;
 [[1,'FOLLOW'],[2,'CROSS hold'],[3,'corner TURN'],[4,'SEARCH']].forEach(([s,nm])=>{o.push('<rect x="'+lx+'" y="'+(ly-4)+'" width="14" height="4" fill="'+MAPCOL[s]+'"/><text x="'+(lx+18)+'" y="'+ly+'" font-size="12">'+nm+'</text>');lx+=115;});
 o.push('<rect x="'+lx+'" y="'+(ly-7)+'" width="14" height="8" fill="#999" fill-opacity="0.35"/><text x="'+(lx+18)+'" y="'+ly+'" font-size="12">drawing</text>');lx+=85;
 o.push('<circle cx="'+(lx+5)+'" cy="'+(ly-3)+'" r="2.5" fill="#212529"/><text x="'+(lx+12)+'" y="'+ly+'" font-size="12">3 black</text>');lx+=80;
 o.push('<rect x="'+lx+'" y="'+(ly-7)+'" width="7" height="7" fill="#1098ad"/><text x="'+(lx+11)+'" y="'+ly+'" font-size="12">corner</text>');lx+=65;
 o.push('<circle cx="'+(lx+5)+'" cy="'+(ly-3)+'" r="4" fill="none" stroke="#e03131" stroke-width="2"/><text x="'+(lx+13)+'" y="'+ly+'" font-size="12">car now</text>');
 o.push('</svg>');return o.join('\n');}
/*END MAPCORE*/

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
 const st=['IDLE','FOLLOW','CROSS','TURN','SEARCH','FAIL'];
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

</script></body></html>)HTML";
