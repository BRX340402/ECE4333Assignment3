#pragma once
static const char TRAFFIC_PAGE[] PROGMEM=R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>ECE 4333 Robot Car Test13</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#101722;color:#edf3fa;font:16px system-ui,sans-serif}
main{max-width:980px;margin:auto;padding:0 18px 40px}
.topbar{position:sticky;top:0;z-index:5;background:#0d141f;border-bottom:1px solid #2a3a4f;padding:10px 18px 0}
.topbar>*{max-width:944px;margin-left:auto;margin-right:auto}
.toprow{display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap}
h1{font-size:22px;margin:4px 0}h1 small{color:#8fa6bf;font-weight:500}h2{font-size:19px;margin:0 0 12px}
.pill{background:#1c2939;border:1px solid #3a4f68;border-radius:999px;padding:6px 12px;font-size:14px}
.actions,nav{display:flex;align-items:center;gap:10px;flex-wrap:wrap}.actions{margin:8px 0}
section,details{background:#1c2939;border:1px solid #304156;border-radius:12px;padding:16px;margin:14px 0}
button,input,select{font:inherit;border:1px solid #52677f;border-radius:8px;padding:9px 12px;background:#26394e;color:#fff}
button{cursor:pointer;min-height:44px}button:disabled{opacity:.5;cursor:default}button.primary{background:#287b67;border-color:#3fa98e}.danger{background:#bd3b3b;border-color:#e06060}
button.big{font-size:18px;font-weight:700;padding:12px 22px}
nav{margin:6px 0 0;gap:4px}nav button{border-radius:10px 10px 0 0;border-bottom:none;background:#1c2939;min-width:120px}
nav button[aria-selected=true]{background:#287b67;border-color:#55baa2}
label{display:block;margin:12px 0}input[type=number]{width:110px}input[type=checkbox]{width:auto;transform:scale(1.3);margin-right:8px}
p{line-height:1.5;margin:10px 0}.hint,small{color:#b5c5d8}#message,#cal{color:#ffcf91;min-height:22px;overflow-wrap:anywhere;margin:4px 0 8px}
#live{display:block;width:100%;height:auto;min-height:180px;background:#080d15;border-radius:8px}
table{width:100%;border-collapse:collapse;font-size:14px}td,th{padding:10px 5px;text-align:left;border-bottom:1px solid #354459}th{color:#b5c5d8;font-weight:500}
summary{cursor:pointer;font-weight:600}a{color:#8cd7ff}.tablewrap{overflow:auto}#savedCal{white-space:pre-wrap;font-size:14px;line-height:1.7}
.stats{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;margin:6px 0 10px}.stats>div{background:#132031;border:1px solid #2c3f56;border-radius:10px;padding:10px}
.stats .num{display:block;font-size:28px;font-weight:700}.stats small{display:block}.red{color:#ff7b7b}.ok{color:#5be3a8}
.drivepad{display:grid;grid-template-columns:repeat(3,70px);gap:8px;margin:10px 0}.drivepad button{touch-action:none;user-select:none;min-height:60px;font-size:22px}
[hidden]{display:none!important}
@media(max-width:620px){main{padding:0 10px 30px}.topbar{padding:8px 10px 0}nav button{min-width:0;flex:1;padding:9px 6px}.stats{grid-template-columns:1fr 1fr}td,th{padding:9px 3px;font-size:12px}}
</style></head><body>
<div class="topbar">
<div class="toprow"><h1>Robot Car <small>Test13</small></h1><span id="runState" class="pill">Connecting…</span></div>
<div class="actions"><button class="danger big" onclick="stopCar()">STOP CAR 停车</button><button class="primary big" onclick="startCar()">Start 开始</button><button class="danger" onclick="emergencyStop()">E-STOP 急停</button><button onclick="clearEStop()">Reset E-stop 复位</button></div>
<p id="message" role="status"></p>
<nav role="tablist"><button id="driveTab" role="tab" aria-selected="true" onclick="switchTab('drive')">行驶 Drive</button><button id="mapTab" role="tab" aria-selected="false" onclick="switchTab('map')">地图 Map</button><button id="cameraTab" role="tab" aria-selected="false" onclick="switchTab('camera')">摄像头 Camera</button><button id="setupTab" role="tab" aria-selected="false" onclick="switchTab('setup')">设置 Settings</button></nav>
</div>
<main>
<div id="drivePanel" role="tabpanel">
<section id="gyroSection"><h2>Gyroscope 陀螺仪</h2>
<p id="mapGyro" style="font-weight:600">Gyro: waiting…</p>
<div style="display:flex;gap:18px;align-items:center;flex-wrap:wrap;margin:8px 0">
<svg width="128" height="128" viewBox="-64 -64 128 128" aria-label="heading dial"><circle r="58" fill="#0b1220" stroke="#4a5b70" stroke-width="2"/>
<text y="-44" text-anchor="middle" font-size="11" fill="#9fb3c8">0°</text><text x="-46" y="4" text-anchor="middle" font-size="11" fill="#9fb3c8">+90</text><text x="46" y="4" text-anchor="middle" font-size="11" fill="#9fb3c8">-90</text>
<g id="gyroNeedle"><polygon points="0,-48 7,4 0,-2 -7,4" fill="#ff6b6b"/><polygon points="0,30 5,4 0,-2 -5,4" fill="#6c7f95"/></g><circle r="4" fill="#edf3fa"/></svg>
<div><div id="gyroYaw" style="font-size:30px;font-weight:700">— °</div>
<div id="gyroDetail" class="hint">heading — · rate — °/s</div>
<div class="actions"><button id="gyroZeroBtn" onclick="mapZeroGyro()">Zero + recalibrate gyro 陀螺仪归零 (car still)</button></div>
<p id="gyroMsg" class="hint">Turn the car LEFT by hand: yaw must INCREASE. 用手把车向左转，读数应增大；否则改 Gyro Z sign。</p></div></div>
</section>

<section><h2>Stop on red 遇红停车</h2>
<div class="stats"><div><span class="num" id="redNow">—</span><small>Red pixels now 当前红色像素</small></div><div><span class="num" id="rpNow">—</span><small>Threshold RP 阈值</small></div><div><span class="num" id="trafficNow" style="font-size:18px">—</span><small>Traffic state 状态</small></div></div>
<label><input id="RS" type="checkbox" onchange="saveSettings()">Stop on red 遇红停车</label>
<label>Red pixel threshold RP 红色像素阈值 <input id="RP" onchange="saveSettings()" type="number" min="0" max="160000" value="100"></label>
<p class="hint">ON: while following the line, when the red region reaches RP pixels the car stops and the camera checks for a STOP sign. STOP sign (2 AI hits) → stand 3 s, then go on. Not a STOP sign (red light) → wait for 2 green frames. After driving on, the same red is ignored until it has left the view. OFF: red is ignored (pure mapping). Tune RP with the live number above: hold the red target where the car should stop and read the pixels. Changing a value stops the car and saves.</p>
<p class="hint">开启后：循迹时红色区域达到 RP 像素就停车，并用 AI 判断是否 STOP 牌：是 STOP 牌就停 3 秒后继续；不是（红灯）就等两帧绿灯再走。重新出发后，同一个红色目标离开视野前不会再触发。把红色目标放在希望停车的位置，看上面的实时像素数来设定 RP。修改设置会先停车再保存。</p>
</section>
<section><h2>Speed 速度</h2>
<label>Cruise PWM 巡航速度 (50–150) <input id="CS" onchange="$('DS').value=Math.min(Number($('DS').value),Number(this.value));saveSettings()" type="number" min="50" max="150" value="60"></label>
<p class="hint">Corners pivot at PWM 100. 55–70 recommended. 转角固定用 PWM 100 原地转；巡航建议 55–70。</p>
<p id="applied" class="hint"></p></section>
<section><h2>Camera pan at the intersection 路口摄像头转向</h2>
<p class="hint">The camera turns toward the STOP sign only while the car is near the intersection (position from the map), then returns to the centre. While it is turned the ultrasonic is ignored (it points at the sign, not at an obstacle). 靠近路口时摄像头转向停车牌，过路口后回正。转向期间超声波不用于避障。</p>
<label>Pan angle 转向角度 (° , 20-160, 90 = straight, 0 = off 关闭) <input id="PA" type="number" min="0" max="160" value="0"></label>
<label>Start looking this far before the intersection 提前多少厘米开始看 (cm) <input id="PL" type="number" min="20" max="120" value="60"></label>
<label>Passes 经过次数 <select id="PM"><option value="1">C to O (first, before the STOP sign) 第一次</option><option value="2">E to O (second pass) 第二次</option><option value="3">both 两次</option></select></label>
<label>Speed near the intersection 路口附近速度 (% of cruise, 40-100) <input id="PS" type="number" min="40" max="100" value="100"></label>
<div class="actions"><button onclick="panTest(0)">Test: left 左</button><button onclick="panTest(90)">Test: centre 正</button><button onclick="panTest(180)">Test: right 右</button><span class="hint">test angle: </span><input id="PT" type="number" min="0" max="180" value="70" style="width:70px"><button onclick="panTest(+$('PT').value)">Go 转</button></div>
<p id="panMsg" class="hint">Find the angle where the STOP sign fills the image, then type it in "Pan angle". 找到能看清停车牌的角度，填入“转向角度”。</p></section>
<section><h2>Overtaking 超车 (Assignment 4)</h2>
<p class="hint">Bug-style detour on the OUTSIDE of the track: the ultrasonic sees an obstacle, the car steps out 90°, drives alongside, steps back in, finds the line again and continues. Needs about 125 cm of straight track before the next corner or intersection; otherwise it waits behind the obstacle. 障碍物在前方时从赛道外侧绕行再回到线上；需要约 125 厘米直道，不够则在后面等待。</p>
<label><input id="OV" type="checkbox"> Overtaking ON 开启超车</label>
<label>Side 绕行侧 <select id="OS"><option value="1">Auto: outside of the next turn 自动(弯道外侧)</option><option value="2">Always left 左</option><option value="3">Always right 右</option></select></label>
<label>Obstacle detected at (cm, 15-80) 检测距离 <input id="OT" type="number" min="15" max="80" value="25"></label>
<label>Pass length: obstacle length + clearance (cm, 15-120) 并行长度 <input id="OP" type="number" min="15" max="120" value="15"></label>
<label>Wheel axle to line sensors (cm, 0-25) 车轴到巡线传感器距离 <input id="OJ" type="number" min="0" max="25" value="8"></label>
<p class="hint">Moving robot ahead: make the pass length longer (pass length x your speed / (your speed - its speed)). 前车在移动时加大并行长度。</p></section>
<section><h2>Race options 比赛选项</h2>
<label><input id="WG" type="checkbox" onchange="saveRace()">Wait for the green light before driving off 绿灯起步</label>
<label>Stop after laps 跑完圈数后停车 (0 = keep running 一直跑) <input id="LAPS" type="number" min="0" max="9" value="0" onchange="saveRace()"></label>
<p class="hint">Applies to <b>Start 开始</b> (line following). Wait for green needs Red / green detection on (Camera tab): after Start the car stands still (state WAIT FOR GREEN) until two green frames, then drives. Laps: the car counts a lap each time it finishes B-C and turns onto C-O-D, and stops there (start point, facing D). 勾选后点 Start：车先不动，看到两帧绿灯再出发；圈数 = 每次走完 B-C 并转上 C-O-D 记一圈，到圈数就停在起点。</p></section>
<section><h2>Target following 目标跟随</h2>
<div class="stats"><div><span class="num" id="fwSeen">—</span><small>Target seen 看到目标</small></div><div><span class="num" id="fwW">—</span><small>Width ‰ / stop 停止宽度</small></div><div><span class="num" id="fwSt" style="font-size:18px">—</span><small>State 状态</small></div></div>
<div class="actions"><button class="primary big" id="followBtn" onclick="startFollow()">Follow target 跟随目标</button></div>
<p class="hint">Uses the same camera and red/green detection settings as the Camera tab (nothing is changed). Put the coloured target in front of the car and press the button; STOP CAR stops it. Outer loop: the camera gives bearing and apparent width every 0.5 s. Inner loop: a gyro PD heading controller turns the car to the target heading, and the car slows down and stops when the target is as wide as the stop width. 摄像头设置与“摄像头”页相同。把彩色目标放在车前，点按钮；STOP CAR 停车。</p>
<label>Target colour 目标颜色 <select id="FC"><option value="g">Green 绿</option><option value="r">Red 红</option></select></label>
<label>Stop when the target is this wide 目标宽度达到此值停车 (‰ of image width, 40–900) <input id="FW" type="number" min="40" max="900" value="250"></label>
<label>Camera horizontal field of view 摄像头水平视角 (°, 20–140) <input id="FF" type="number" min="20" max="140" value="62"> <input id="FM" type="checkbox">image mirrored 图像镜像</label>
<details><summary>PD gains 参数</summary><label>Kp ×10 (PWM per degree ×10, 1–80) <input id="FKP" type="number" min="1" max="80" value="15"></label><label>Kd ×100 (PWM per °/s ×100, 0–50) <input id="FKD" type="number" min="0" max="50" value="10"></label>
<p class="hint">If the car turns AWAY from the target tick “image mirrored”. If it under-turns raise the field of view, if it overshoots lower it; if it wobbles raise Kd or lower Kp. 车背向目标时勾选“图像镜像”；转不够就加大视角，转过头就减小；摆动就加大 Kd 或减小 Kp。</p></details>
<p id="fwMsg" class="hint"></p></section>
<details><summary>Manual movement 手动驾驶</summary><label>Speed <input id="speed" type="number" value="60" min="0" max="180"></label><p class="hint">Hold an arrow to move; release to stop. 按住箭头移动，松开停车。</p><div class="drivepad"><span></span><button data-dir="1">↑</button><span></span><button data-dir="3">←</button><button class="danger" onclick="stopCar()">■</button><button data-dir="4">→</button><span></span><button data-dir="2">↓</button></div></details>
</div>
<div id="mapPanel" role="tabpanel" hidden>
<section id="mapSection"><h2>Road mapping / 道路测图 (Test13)</h2>
<p class="hint">This tablet records EVERY line the car sends over Wi-Fi while this page is open (also after a reload: data is kept on the tablet and the car buffers ~3 min). Keep the screen on. 本页打开期间平板自动记录全部数据，断线约3分钟内可自动补齐；请保持屏幕常亮。</p>
<p id="mapStatus" role="status">Waiting for the car…</p>
<label>Map of <select id="mapRun" onchange="mapPickRun(this.value)"><option value="live">Latest run (live)</option></select></label>
<div style="display:flex;gap:14px;flex-wrap:wrap;align-items:center;margin:6px 0"><b>Layers 图层:</b>
<label style="margin:0"><input id="mapLyD" type="checkbox" checked onchange="mapLayerChanged()">Drawing 图纸</label>
<label style="margin:0"><input id="mapLyP" type="checkbox" checked onchange="mapLayerChanged()"><span style="color:#c8a38f">Predicted 预测 (wheel model only)</span></label>
<label style="margin:0"><input id="mapLyF" type="checkbox" checked onchange="mapLayerChanged()"><span style="color:#5be3a8">Fused 融合 (gyro + wheel)</span></label>
<label style="margin:0"><input id="mapLyM" type="checkbox" checked onchange="mapLayerChanged()"><span style="color:#8cd7ff">Mapped 校正 (scaled, by state)</span></label></div>
<label style="display:block;margin:6px 0">Distance from the stopped car to the STOP sign / light, for the position estimate 停车时车到停车牌/红灯的距离 (cm) <input id="SR" type="number" min="5" max="150" value="35" style="width:80px" onchange="MAPSIGNRANGE=+this.value||35;saveA4();mapRefresh(true)"></label>
<div id="mapView"></div>
<div id="mapHeading"></div>
<div id="mapStates"></div>
<div id="mapTrans" style="overflow-x:auto"></div>
<div class="actions"><button class="primary" onclick="mapDownload('csv')">Download data CSV 数据</button><button onclick="mapDownload('raw')">Download all lines 全部原始数据</button></div>
<div class="actions"><button onclick="mapDownload('png')">Map PNG 地图图片</button><button onclick="mapDownload('svg')">Map SVG</button><button onclick="mapDownload('trans')">State transitions CSV 状态转换</button><button onclick="mapDownload('spng')">State timeline PNG 状态图</button><button onclick="mapDownload('hpng')">Heading chart PNG 航向图</button><button onclick="mapDownload('hsvg')">Heading SVG</button><button onclick="mapDownload('seg')">Segments CSV 分段表</button><button onclick="mapDownload('points')">Track points CSV 轨迹点</button><button onclick="mapDownload('summary')">Summary TXT 摘要</button></div>
<pre id="mapSummary" style="white-space:pre-wrap;font-size:13px"></pre>
<div class="tablewrap"><table id="mapSegTable"></table></div>
<details><summary>Track lengths (cm) 赛道实测长度</summary>
<p class="hint">Measured centre-line length of each straight, corner to corner. A-B and B-C are estimated from the drawing; measure them. Saved on this tablet.</p>
<label>C-O-D <input id="mapLen0" type="number" min="1" max="2000" step="0.1"></label> <label>D-E <input id="mapLen1" type="number" min="1" max="2000" step="0.1"></label> <label>E-O-A <input id="mapLen2" type="number" min="1" max="2000" step="0.1"></label> <label>A-B <input id="mapLen3" type="number" min="1" max="2000" step="0.1"></label> <label>B-C <input id="mapLen4" type="number" min="1" max="2000" step="0.1"></label>
<div class="actions"><button onclick="mapSaveRef()">Save lengths</button><button onclick="mapResetRef()">Drawing values</button></div></details>
<div class="actions"><button class="danger" onclick="mapClear()">Delete recording from tablet 清除平板上的记录</button></div>
<p class="hint">Predicted (brown dashed) = kinematic model only: wheel PWM → speed, Euler integration on the UNO, no gyro. Fused (green) = gyro heading + the same wheel speed, as logged by the UNO (both unscaled, exactly what the car believed). Mapped = fused + this page's gyro offset/drift fit, distance scale and lap closure. The summary lists the distance of each track to the drawn centre line. Follow runs (vision target following) are listed in the run menu and show the same layers plus set-point vs gyro heading. Map: grey = assignment drawing; blue = line following, orange = crossing heading-hold, red = corner pivot, purple = search; black dots = all three sensors black (crossing); teal squares = detected corners; pink = stopped for red / STOP sign; red ring = car now. Start each run at corner C facing the STOP sign. Distances are scaled automatically from complete straights.</p>
</section>

</div>
<div id="cameraPanel" role="tabpanel" hidden>
<section><div style="position:relative"><img id="live" width="320" height="240" alt="Live camera — connecting"><div id="boxes" style="position:absolute;inset:0;pointer-events:none"></div></div><small id="previewState">Connecting camera…</small> <button id="togglePreview" onclick="togglePreview()">Pause preview</button>
<div class="tablewrap"><table><thead><tr><th>Target</th><th>Detected</th><th>Pixels 像素</th><th>Width (px)</th><th>Distance (cm)</th></tr></thead><tbody>
<tr><td>Red 红</td><td id="redSeen">—</td><td id="redPixels">—</td><td id="redWidth">—</td><td id="redDistance">—</td></tr>
<tr><td>Green 绿</td><td id="greenSeen">—</td><td id="greenPixels">—</td><td id="greenWidth">—</td><td id="greenDistance">—</td></tr>
<tr><td>STOP sign</td><td id="stopSeen">—</td><td id="stopPixels">—</td><td id="stopWidth">—</td><td id="stopDistance">—</td></tr>
</tbody></table></div><small>Pixels = coloured area (compare with RP). The preview only streams while this tab is open; detection keeps running in the background. 画面只在本页打开时传输，识别始终在后台运行。</small>
<p id="visionHealth" class="hint"></p></section>
<section><h2>Camera settings</h2><p class="hint">Changing settings stops the car.</p>
<label><input id="detect" type="checkbox" checked onchange="setDetection('detect',this.checked)">STOP AI (96×96)</label>
<label><input id="color" type="checkbox" checked onchange="setDetection('color',this.checked)">Red / green detection 红绿识别</label>
<label>Resolution <select id="framesize" onchange="setCamera('framesize',this.value)"><option value="0">QQVGA 160×120</option><option value="3">HQVGA 240×176</option><option value="4">QVGA 320×240</option><option value="5">CIF 400×296</option><option value="6">VGA 640×480</option><option value="7" selected>SVGA 800×600</option><option value="8">XGA 1024×768</option><option value="9">SXGA 1280×1024</option><option value="10">UXGA 1600×1200</option></select></label>
<label>White balance <select id="wb_mode" onchange="setCamera('wb_mode',this.value)"><option value="0">Auto (0)</option><option value="1">Sunny (1)</option><option value="2">Cloudy (2)</option><option value="3">Office (3)</option><option value="4">Home (4)</option></select></label>
<label><input id="awb" type="checkbox" checked onchange="setCamera('awb',+this.checked)">Automatic white balance</label>
<label><input id="awb_gain" type="checkbox" checked onchange="setCamera('awb_gain',+this.checked)">White balance gain</label>
<label>Brightness <select id="brightness" onchange="setCamera('brightness',this.value)"><option>-2</option><option>-1</option><option selected>0</option><option>1</option><option>2</option></select></label>
<label>Contrast <select id="contrast" onchange="setCamera('contrast',this.value)"><option>-2</option><option>-1</option><option selected>0</option><option>1</option><option>2</option></select></label>
<label>Saturation <select id="saturation" onchange="setCamera('saturation',this.value)"><option>-2</option><option>-1</option><option selected>0</option><option>1</option><option>2</option></select></label>
<label><input id="vflip" type="checkbox" onchange="setCamera('vflip',+this.checked)">Flip vertically</label><label><input id="hmirror" type="checkbox" onchange="setCamera('hmirror',+this.checked)">Mirror horizontally</label>
<details><summary>Exposure, gain and advanced</summary>
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
<label><input id="colorbar" type="checkbox" onchange="setCamera('colorbar',+this.checked)">Sensor test colour bars (turn off before driving)</label>
<p id="cameraInfo" class="hint">Read actual camera settings to see uptime and frame age.</p>
</details>
<div class="actions"><button onclick="loadCamera()">Read actual camera settings</button><button id="restoreDefaults" onclick="restoreCameraDefaults()">Restore SVGA defaults (restarts ESP32)</button></div>
<button id="capture" onclick="captureNew()">Capture training photo</button><span id="captureState"></span><p id="downloads" hidden><a id="jpgLink">Open original JPEG</a> · <a id="bmpLink">Model input BMP</a></p>
</section>
<details><summary>Distance calibration 距离标定 (optional)</summary><p>Stop the car. Select a target, scan its width, enter the measured camera-to-target distance, then save. Not needed for Stop on red (it uses RP pixels).</p>
<label>Target <select id="profile" onchange="resetSample()"><option value="light">Red</option><option value="green">Green</option><option value="stop">STOP sign</option></select></label>
<label>Scanned width (px) <input id="width" readonly> / image <input id="image_w" readonly></label>
<label>Measured distance (cm) <input id="cm" type="number" min="5" max="300" value="30"></label>
<div class="actions"><button onclick="sampleWidth()">Scan target width</button><button id="saveCal" onclick="calibrate()" disabled>Save measured distance</button><button onclick="clearCalibration()">Clear selected target</button></div><p id="cal" role="status"></p><p id="sampleInfo" class="hint"></p><div id="savedCal">Loading calibration…</div></details>
</div>
<div id="setupPanel" role="tabpanel" hidden>
<section><h2>Line sensors 地面传感器</h2><p id="floor">Waiting for floor sensors…</p>
<label>Black range 黑线范围 <input id="BL" type="number" min="0" max="1022" value="600"> to <input id="BH" type="number" min="1" max="1023" value="1023"></label>
<div class="actions"><button onclick="sampleFloor('white')">Sample white floor 采白地</button><button onclick="sampleFloor('black')">Sample black line 采黑线</button><button onclick="applyFloor()">Apply measured threshold 应用</button></div>
<p id="floorCal" class="hint">Stop the car. Put ALL three sensors over each surface in turn at normal ride height. Each sample takes 6 fresh readings. The UNO adds ±40 hysteresis around the threshold.</p></section>
<section><h2>Gyroscope 陀螺仪</h2>
<label>Gyro Z sign 陀螺仪方向 <select id="GS"><option value="0">Negative raw Z = left</option><option value="1">Positive raw Z = left</option></select></label>
<p class="hint">Turning the car LEFT must make the yaw on the Drive tab INCREASE. Your car (GY-521 chip up): Positive raw Z = left. 车向左转时“行驶”页的航向应增大；你的车应选 Positive raw Z = left。</p></section>
<div hidden><input id="DS" type="number" value="40"><input id="SC" type="number" value="25"><input id="RA" type="number" value="180"><input id="SA" type="number" value="360"><input id="CT" type="number" value="1500"></div>
<button class="primary" onclick="saveSettings()">Save car settings 保存设置</button><p id="applied2" class="hint"></p>
</div>
</main>
<script>
const $=id=>document.getElementById(id),names=['OFF 停止','—','TRACK 循迹','—','RED: CHECKING STOP 红色·识别中','WAIT FOR GREEN 等绿灯','STOP SIGN 3 s 停车','CROSSING 交叉口','FAULT 故障','FOLLOW: TRACKING 跟随中','FOLLOW: ARRIVED 已到达','FOLLOW: NO TARGET, STOPPED 无目标·停','FOLLOW: SEARCHING 搜索目标'];
const keys=['SC','BL','BH','RA','SA','CT','GS','CS','DS','RP'],TABS=['drive','map','camera','setup'];let driving=false,latest=null,scannedToken=null,sampleResolution=null,currentTab='drive',previewTimer=null,manualHeld=false,previewBusy=false,previewPaused=false,previewURL=null,previewStamp='',previewLastShown=0,cameraBusy=false,starting=false;
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
 throw Error('Black/white readings overlap or are less than 60 ADC counts apart. Check height, light and each sensor.');
}
async function applyFloor(){try{const [low,high]=floorThreshold(floorSamples.white,floorSamples.black);$('BL').value=low;$('BH').value=high;if(await saveSettings())$('floorCal').textContent='Saved black range '+low+' to '+high+'.';}catch(e){$('floorCal').textContent=e.message;}}
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function request(url,data){const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),4000);try{const r=await fetch(url,{cache:'no-store',signal:controller.signal,...(data===undefined?{}:{method:'POST',body:new URLSearchParams(data)})});const t=await r.text();if(!r.ok){const e=Error(t);e.status=r.status;throw e;}return t;}finally{clearTimeout(timer);}}
async function post(url,data={}){return request(url,data);}
function switchTab(name){if(!TABS.includes(name))name='drive';currentTab=name;for(const t of TABS){$(t+'Panel').hidden=t!==name;$(t+'Tab').setAttribute('aria-selected',String(t===name));}try{localStorage.setItem('a3tab',name);}catch(e){}if(name==='camera'&&!previewPaused){previewStamp='';refreshPreview();}if(name==='map'&&typeof mapRefresh==='function')mapRefresh(true);}
async function stopCar(){driving=false;manualHeld=false;try{$('message').textContent=await post('/cmd',{action:'stop'});}catch(e){$('message').textContent='STOP was not delivered. Autonomous running may continue; use the physical power switch if needed.';}}
async function emergencyStop(){driving=false;manualHeld=false;try{$('message').textContent='Emergency stop command sent. Confirm E-stop on this page.';await post('/cmd',{action:'estop'});}catch(e){$('message').textContent='E-stop communication failed! Use physical power switch.';}}
async function clearEStop(){driving=false;try{await post('/cmd',{action:'clear_estop'});$('message').textContent='Reset requested. Press Start separately.';}catch(e){$('message').textContent=e.message;}}
async function startCar(){if(starting||settingsBusy){$('message').textContent='Wait for the pending command.';return;}
 starting=true;
 try{const wanted=readSettingsForm(),current=JSON.parse(await request('/traffic-settings'));
 if(!settingsMatch(current,wanted)&&!await saveSettings())return;
 const before=JSON.parse(await request('/traffic-status'));
 saveA4();await post('/traffic-start',{green:+$('WG').checked,laps:Math.max(0,Math.min(9,+$('LAPS').value||0)),pan:+$('PA').value||0,lead:+$('PL').value||60,passes:+$('PM').value,slow:+$('PS').value||100,ovt:$('OV').checked?+$('OS').value:0,trig:+$('OT').value||35,par:+$('OP').value||45,join:+$('OJ').value||0});let confirmed=false;
 for(let i=0;i<16;i++){await delay(300);const d=JSON.parse(await request('/traffic-status'));
 if(d.age_ms<1500&&d.uno?.M===6&&d.uno.RN!==before.uno?.RN){confirmed=true;break;}}
 if(!confirmed)throw Error('Start confirmation missing; inspect car state before retrying.');
 driving=true;$('message').textContent='UNO running 运行中';
 }catch(e){$('message').textContent=e.message;}
 finally{starting=false;}}
async function loadSettings(){try{const d=JSON.parse(await request('/traffic-settings'));keys.forEach(k=>$(k).value=d[k]);$('RS').checked=!!d.RS;}catch(e){$('message').textContent=e.message;}}
let settingsBusy=false;
function readSettingsForm(){const d={};keys.forEach(k=>{const n=Number($(k).value);if(!Number.isInteger(n))throw Error('Use whole numbers');d[k]=n;});d.RS=+$('RS').checked;return d;}
function settingsMatch(current,wanted){return !!current.applied&&[...keys,'RS'].every(k=>Number(current[k])===wanted[k]);}
async function saveSettings(){if(settingsBusy)return false;settingsBusy=true;driving=false;manualHeld=false;$('RS').disabled=$('CS').disabled=$('RP').disabled=true;
 try{const wanted=readSettingsForm();$('applied').textContent=$('applied2').textContent='Stopping car and applying settings…';
 await post('/cmd',{action:'stop'});
 let saved=false;for(let n=0;n<16;n++){try{await post('/traffic-settings',wanted);saved=true;break;}catch(e){if(e.status!==409)throw e;await delay(400);}}
 if(!saved)throw Error('UNO did not acknowledge OFF. Settings not applied.');
 for(let n=0;n<20;n++){const actual=JSON.parse(await request('/traffic-settings'));if(settingsMatch(actual,wanted)){$('message').textContent=`UNO confirmed 已确认: stop on red ${wanted.RS?'ON':'OFF'}, RP ${wanted.RP} px, cruise PWM ${wanted.CS}, gyro sign ${wanted.GS}, black ${wanted.BL}-${wanted.BH}. Press Start.`;return true;}await delay(400);}
 throw Error('Settings saved but UNO confirmation missing. Do not start; check the Test13 UNO is uploaded.');
 }catch(e){$('message').textContent=e.message;return false;}finally{settingsBusy=false;$('RS').disabled=$('CS').disabled=$('RP').disabled=false;}}
function resetSample(){scannedToken=null;sampleResolution=null;$('width').value='';$('image_w').value='';$('saveCal').disabled=true;$('sampleInfo').textContent='Scan the selected target first.';}
async function sampleWidth(){resetSample();try{const d=JSON.parse(await post('/traffic-sample',{profile:$('profile').value}));$('width').value=d.width;$('image_w').value=d.image_w;sampleResolution=d.framesize;scannedToken=d.token;$('saveCal').disabled=false;$('sampleInfo').textContent=`Scanned ${d.width} px. Enter the distance and save.`;}catch(e){$('cal').textContent=e.message;}}
async function calibrate(){try{if(scannedToken===null)throw Error('Scan the target first');$('cal').textContent=await post('/traffic-calibrate',{profile:$('profile').value,cm:$('cm').value,token:scannedToken});scannedToken=null;$('saveCal').disabled=true;}catch(e){$('cal').textContent=e.message;}}
async function clearCalibration(){try{$('cal').textContent=await post('/traffic-clear',{profile:$('profile').value});resetSample();}catch(e){$('cal').textContent=e.message;}}
function showSavedCalibration(d){const c=d.calibration||{},labels={light:'Red',green:'Green',stop:'STOP'};$('savedCal').textContent=['light','green','stop'].map(k=>{const v=c[k];return !v||!v.valid?`${labels[k]}: not calibrated`:`${labels[k]}: saved width ${v.width} px / image ${v.image_w} px, measured distance ${v.distance_cm} cm${v.active?'':' (resolution mismatch — rescan)'}`;}).join('\n');}
function showObjects(objects){for(const k of ['red','green','stop']){const v=objects[k]||{};$(k+'Seen').textContent=!v.valid?'—':v.seen?'Yes':'No';$(k+'Pixels').textContent=v.valid&&v.seen?v.pixels:'—';$(k+'Width').textContent=v.valid&&v.seen&&v.width>0?v.width:'—';$(k+'Distance').textContent=v.valid&&v.seen&&v.distance_cm>=0?v.distance_cm:'—';}}
const faultReasons={10:'Gyro direction reversed again after auto-correction — check Gyro Z sign (Settings)',11:'Line not found after the full search (too fast / tape too narrow / lifted?)',14:'Obstacle too close for the search creep'};
function faultMessage(u){const code=Number(u.FC);return code>0?`FAULT ${code} — ${faultReasons[code]||'Unknown fault'}. Press STOP CAR, fix the cause, place the car on the line, then Start.`:'FAULT — press STOP CAR before restarting.';}
async function poll(){try{const d=JSON.parse(await request('/traffic-status'));if(latest&&d.esp_ms<latest.esp_ms)$('message').textContent='ESP32 restart detected.';latest=d;
 const o=d.objects||{},u=d.uno||{},c=d.settings||{},red=o.red||{};
 showObjects(o);followLive(d);boxObjects=o;boxesAt=Date.now();drawBoxes();showSavedCalibration(d);
 const vh=d.vision_health||{};$('visionHealth').textContent=`Colour frame age: ${vh.age_ms??'—'} ms | Red regions: ${vh.red_regions??'—'} | Green regions: ${vh.green_regions??'—'} | Colour processing: ${vh.processing_ms??'—'} ms | STOP inference: ${o.stop?.inference_ms??'—'} ms`;
 const px=red.valid&&red.seen?Number(red.pixels):0,rp=Number(c.RP);
 $('redNow').textContent=red.valid?px:'—';$('redNow').className='num'+(red.valid&&px>=rp&&rp>=0?' red':'');$('rpNow').textContent=Number.isFinite(rp)?rp:'—';
 const stale=d.age_ms>=1500;
 $('trafficNow').textContent=stale?'—':((u.M===6||u.M===7)?(names[u.TS]||'—'):u.M===5?'MANUAL 手动':'OFF 停止');
 $('runState').textContent=!stale?(Number(u.UV)===43?((u.M===6||u.M===7)?(names[u.TS]||'RUNNING'):u.M===5?'Manual 手动':'Stopped 已停止')+(u.ES?' · E-STOP':''):'UNO VERSION MISMATCH — upload Test13 UNO_Mapping'):'UNO telemetry stale — motor state unknown';
 $('floor').textContent=`Live floor sensors 实时读数 L ${u.L??'—'} · M ${u.C??'—'} · R ${u.R??'—'} | threshold ${c.BL??'—'}–${c.BH??'—'}`;
 const ap=settingsBusy?'Applying settings…':c.applied?`UNO confirmed 已确认: stop on red ${c.RS?'ON':'OFF'} · RP ${c.RP} px · cruise PWM ${c.CS} · gyro sign ${c.GS}`:'Waiting for UNO settings confirmation';$('applied').textContent=$('applied2').textContent=ap;
 $('detect').checked=!!d.detect_enabled;$('color').checked=!!d.color_enabled;
 if(u.TS===8){driving=false;$('message').textContent=faultMessage(u);}
 if(scannedToken!==null&&sampleResolution!==d.framesize)resetSample();
 }catch(e){showObjects({});boxObjects={};drawBoxes();$('runState').textContent='Web connection lost — motor state unknown';}finally{setTimeout(poll,500);}}
async function heartbeat(){try{if(manualHeld)await post('/cmd',{action:'ping'});}catch(e){driving=false;manualHeld=false;$('message').textContent='Manual control connection lost; manual driving will time out.';}finally{setTimeout(heartbeat,300);}}
let boxObjects={},boxesAt=0;
function drawBoxes(){
 const layer=$('boxes');layer.innerHTML='';
 if(currentTab!=='camera'||previewPaused||!previewStamp||Date.now()-previewLastShown>1500||Date.now()-boxesAt>1500)return;
 for(const k of ['red','green','stop']){const v=boxObjects[k];
  if(!v?.box_valid||!(v.image_w>0&&v.image_h>0&&v.w>0&&v.h>0))continue;
  const age=Math.abs((Number(previewStamp)-Number(v.captured))|0);if(age>(k==='stop'?5000:1200))continue;
  const x=Math.max(0,v.x),y=Math.max(0,v.y),w=Math.min(v.w,v.image_w-x),h=Math.min(v.h,v.image_h-y);if(w<=0||h<=0)continue;
  const el=document.createElement('div');el.style.cssText=`position:absolute;left:${100*x/v.image_w}%;top:${100*y/v.image_h}%;width:${100*w/v.image_w}%;height:${100*h/v.image_h}%;border:2px solid ${k==='red'?'#ff4545':k==='green'?'#39ff88':'#ffcf40'};color:white;font:bold 12px sans-serif;box-sizing:border-box`;
  const label=document.createElement('span');label.textContent=k==='stop'?'STOP':k.toUpperCase()+(k==='red'&&v.pixels?' '+v.pixels+' px':'');label.style.cssText=`position:absolute;${k==='stop'?'bottom:0':'top:0'};left:0;background:#101722cc;white-space:nowrap`;el.appendChild(label);layer.appendChild(el);
 }
}
async function refreshPreview(){
 if(previewBusy||previewPaused)return;
 clearTimeout(previewTimer);
 if(currentTab!=='camera'){previewTimer=setTimeout(refreshPreview,1000);return;} // no image traffic while driving/mapping
 previewBusy=true;
 const started=Date.now(),controller=new AbortController();
 const timeout=setTimeout(()=>controller.abort(),8000);let nextURL=null;
 try{
  const r=await fetch(`http://${location.hostname}:81/preview?after=${encodeURIComponent(previewStamp)}&t=${started}`,{cache:'no-store',signal:controller.signal});
  if(r.status===204){if(previewLastShown&&Date.now()-previewLastShown>2000)$('previewState').textContent='Waiting for a new frame — last image retained';return;}
  if(!r.ok)throw Error(`Camera temporarily unavailable (${r.status})`);
  const blob=await r.blob();if(!blob.size)throw Error('Empty camera frame');
  nextURL=URL.createObjectURL(blob);
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
function previewAge(){drawBoxes();if(currentTab==='camera'&&!previewPaused&&previewLastShown&&Date.now()-previewLastShown>3000)$('previewState').textContent='Preview paused — last image retained; retrying…';setTimeout(previewAge,1000);}
function togglePreview(){previewPaused=!previewPaused;drawBoxes();$('togglePreview').textContent=previewPaused?'Start preview':'Pause preview';clearTimeout(previewTimer);if(previewPaused)$('previewState').textContent='Preview paused by you — detection continues';else{previewStamp='';refreshPreview();}}
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
/*MAPCORE (Test13) - mapping algorithm of PC_Mapping/draw_map.py (tested by tests/test_map_core.js) + Test11 predicted/fused tracks, heading chart, target-following runs*/
const MAPREF0=[{name:'C-O-D',h:0,len:320},{name:'D-E',h:-136.6,len:151.3},{name:'E-O-A',h:-240,len:220},{name:'A-B',h:-210,len:43.3},{name:'B-C',h:-120,len:125}];
const MAPCOL={1:'#2a7de1',2:'#f08c00',3:'#d6336c',4:'#ae3ec9',5:'#000000',6:'#ff8fab'};
function mapDecode(d){const st=d[13],k=d[14],m=st>>4,o={q:d[0],t:d[1],L:d[2],C:d[3],R:d[4],PL:d[5],PR:d[6],g:d[7]/10,yaw:d[8]/10,dist:d[9]/10,x:d[10],y:d[11],u:d[12],mode:m,fs:st&7,gy:(st>>3)&1,k:k};
 if(d.length>=19)o.sm=d[18];                                                // state-machine code, UV>=43
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
 mapStateAnalysis(res);
 res.closureRaw=res.lapEnds.map(e=>({fused:Math.hypot(res.fx[e],res.fy[e]),pred:res.hasPred?Math.hypot(res.px[e],res.py[e]):null}));
 return res;}

/* ---- Test13: state machine log, overtaking episodes, object positions (Assignment 4) ---- */
const SMNAME=['STANDBY','TRACK','CROSSING','CORNER TURN','LINE SEARCH','RED: CHECKING','STOP SIGN (3 s)','WAIT FOR GREEN','OVERTAKE: WAIT BEHIND','OVERTAKE: OUT','OVERTAKE: PASS','OVERTAKE: RETURN','FAULT','LAPS FINISHED','INTERSECTION APPROACH','MANUAL / FOLLOW'];
const SMCOL=['#adb5bd','#2f9e44','#f08c00','#e64980','#ae3ec9','#e03131','#c92a2a','#fa5252','#845ef7','#7048e8','#5f3dc4','#4c6ef5','#000000','#868e96','#1098ad','#ced4da'];
function mapSignRange(){return typeof MAPSIGNRANGE==='number'?MAPSIGNRANGE:35;}
function mapStateAnalysis(res){
 const run=res.run,N=run.length;res.hasSm=run.every(r=>r.sm!==undefined);res.trans=[];res.episodes=[];res.objects=[];res.offroad=[];
 if(!res.hasSm)return;
 const rad=i=>res.yaw[i]*Math.PI/180;
 let st=run[0].sm,i0=0;
 const pushEp=(a,b,ok)=>{const o={i0:a,i1:b,dur:res.t[b]-res.t[a],ok:ok,x0:res.fx[a],y0:res.fy[a],x1:res.fx[b],y1:res.fy[b]};
  const u=run[a].u>0?run[a].u:(a>0&&run[a-1].u>0?run[a-1].u:-1);
  if(u>0)o.obstacle={x:res.fx[a]+u*Math.cos(rad(a)),y:res.fy[a]+u*Math.sin(rad(a)),u:u};res.episodes.push(o);};
 for(let i=1;i<=N;i++){
  if(i<N&&run[i].sm===st)continue;
  const dwell=res.t[Math.min(i,N-1)]-res.t[i0];
  res.trans.push({i:i0,t:res.t[i0],from:i0?run[i0-1].sm:null,to:st,x:res.fx[i0],y:res.fy[i0],h:res.yaw[i0],dwell:dwell});
  // object positions: where the car stood for a STOP sign / red light + nominal range ahead
  if(st===6||st===7||st===5){const r0=mapSignRange(),sx=res.fx[i0]+r0*Math.cos(rad(i0)),sy=res.fy[i0]+r0*Math.sin(rad(i0));
   if(st===6)res.objects.push({kind:'stop',x:sx,y:sy,i:i0});else if(st===7)res.objects.push({kind:'light',x:sx,y:sy,i:i0});}
  if(st===4&&dwell>3)res.offroad.push({t:res.t[i0],dur:dwell});
  i0=i;if(i<N)st=run[i].sm;
 }
 // overtaking episodes: samples with state 8..11 (wait behind counts as part of the manoeuvre), ended by TRACK = success, FAULT = failed
 let a=-1;for(let i=0;i<N;i++){const o=run[i].sm>=8&&run[i].sm<=11;if(o&&a<0)a=i;if(!o&&a>=0){pushEp(a,i-1,run[i].sm!==12);a=-1;}}
 if(a>=0)pushEp(a,N-1,null);
 res.episodes.forEach(e=>{if(e.obstacle)res.objects.push({kind:'obstacle',x:e.obstacle.x,y:e.obstacle.y,i:e.i0});});
}
function mapStateLines(res){const L=[];
 if(!res.hasSm){L.push('state machine: not logged (UNO older than Test13)');return L;}
 const dur={};res.trans.forEach(t=>{dur[t.to]=(dur[t.to]||0)+t.dwell;});
 L.push('state machine: '+res.trans.length+' transitions; time in each state: '+Object.keys(dur).map(k=>SMNAME[k]+' '+dur[k].toFixed(1)+' s').join(', '));
 const ep=res.episodes;
 L.push('overtaking: '+ep.length+' manoeuvre(s)'+(ep.length?': '+ep.map((e,i)=>(i+1)+') '+e.dur.toFixed(1)+' s, '+(e.ok===null?'not finished':e.ok?'finished, back on the line':'FAILED')+(e.obstacle?', obstacle at ('+e.obstacle.x.toFixed(0)+', '+e.obstacle.y.toFixed(0)+') cm':'')).join('; '):' (none in this run)'));
 const sg=res.objects.filter(o=>o.kind==='stop'),lt=res.objects.filter(o=>o.kind==='light');
 L.push('estimated STOP sign position(s) (stopping pose + '+mapSignRange()+' cm ahead): '+(sg.length?sg.map(o=>'('+o.x.toFixed(0)+', '+o.y.toFixed(0)+')').join(', '):'none this run')+'; traffic light: '+(lt.length?lt.map(o=>'('+o.x.toFixed(0)+', '+o.y.toFixed(0)+')').join(', '):'start point (0, 0)'));
 const shortHold=res.trans.filter(t=>t.to===6&&t.dwell<2.9&&t.i+1<res.run.length).length;
 L.push('self-check of traffic rules: STOP holds shorter than 3 s: '+shortHold+'; off-road episodes longer than 3 s (line search): '+res.offroad.length+(res.offroad.length?' ('+res.offroad.map(o=>o.dur.toFixed(1)+' s at t='+o.t.toFixed(0)).join(', ')+')':''));
 return L;}
function mapTransRows(res){return (res.trans||[]).map(t=>({t_s:+t.t.toFixed(2),from:t.from===null?'':SMNAME[t.from],to:SMNAME[t.to],fused_x_cm:+t.x.toFixed(1),fused_y_cm:+t.y.toFixed(1),fused_heading_deg:+t.h.toFixed(1),time_in_state_s:+t.dwell.toFixed(2)}));}
function mapStateSVG(res){
 if(!res.hasSm)return '';
 const W=900,ml=150,mr=12,mt=30,rowH=20,used=[...new Set(res.run.map(r=>r.sm))].sort((a,b)=>a-b),H=mt+used.length*rowH+36,T=res.t[res.t.length-1]||1,X=t=>(ml+t/T*(W-ml-mr)).toFixed(1),o=[];
 o.push('<svg xmlns="http://www.w3.org/2000/svg" width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" font-family="sans-serif"><rect width="100%" height="100%" fill="#ffffff"/>');
 o.push('<text x="10" y="18" font-size="15" fill="#222">State machine: current state vs time</text>');
 used.forEach((u,k)=>{const y=mt+k*rowH;o.push('<line x1="'+ml+'" y1="'+(y+rowH-1)+'" x2="'+(W-mr)+'" y2="'+(y+rowH-1)+'" stroke="#eee"/><text x="'+(ml-6)+'" y="'+(y+14)+'" font-size="11" text-anchor="end" fill="#333">'+SMNAME[u]+'</text>');});
 res.trans.forEach(t=>{const k=used.indexOf(t.to),y=mt+k*rowH,w=Math.max(1.5,(t.dwell/T)*(W-ml-mr));o.push('<rect x="'+X(t.t)+'" y="'+(y+3)+'" width="'+w.toFixed(1)+'" height="'+(rowH-7)+'" fill="'+SMCOL[t.to]+'"/>');});
 for(let g=0;g<=T;g+=Math.max(5,Math.ceil(T/10/5)*5))o.push('<text x="'+X(g)+'" y="'+(H-16)+'" font-size="11" text-anchor="middle" fill="#555">'+g+'</text>');
 o.push('<text x="'+((ml+W-mr)/2)+'" y="'+(H-3)+'" font-size="12" text-anchor="middle" fill="#333">time (s)</text></svg>');return o.join('\n');}
function mapNum(v,d){return v===null||v===undefined||!isFinite(v)?'n/a':v.toFixed(d===undefined?1:d);}
function mapTrackLines(res){const L=[],e=res.err,f=k=>e[k]?'mean '+mapNum(e[k].mean)+', rms '+mapNum(e[k].rms)+', max '+mapNum(e[k].max)+' cm':'n/a (UNO older than Test11: no predicted pose logged)';
 L.push('track error = distance to the drawn centre line (grey):');
 L.push('  predicted (wheel model only): '+f('pred'));L.push('  fused (gyro + wheel model)  : '+f('fused'));L.push('  mapped (corrected + scaled) : '+f('mapped'));
 L.push('heading error on the straights vs the drawing (rms): predicted '+mapNum(res.hdg.pred)+' deg, fused/gyro '+mapNum(res.hdg.fused)+' deg');
 if(res.hasPred)L.push('predicted heading minus gyro heading: at the end '+mapNum(res.hdgEnd)+' deg, rms over the run '+mapNum(res.hdgRms)+' deg');
 mapStateLines(res).forEach(x=>L.push(x));
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
function mapSegRows(res,ref){let prevM=null,prevR=null;return res.segs.map((g,k)=>{const s=ref[g.label];let hs=0;for(let i=g.i0;i<=g.i1;i++)hs+=res.yawc[i];const len=g.len*res.scale;
 const mh=hs/(g.i1-g.i0+1),nrm=a=>((a+540)%360+360)%360-180,et=prevR===null?null:nrm(s.h-prevR),mt=prevM===null?null:nrm(mh-prevM);prevM=mh;prevR=s.h;
 const wrong=et!==null&&Math.abs(et)>30&&mt!==null&&Math.sign(et)!==Math.sign(mt);
 return{order:k+1,segment:s.name,complete:g.complete?1:0,expected_turn_deg:et===null?'':+et.toFixed(0),measured_turn_deg:mt===null?'':+mt.toFixed(0),turn_check:et===null?'':(wrong?'WRONG WAY':'ok'),ref_heading_deg:s.h,meas_heading_deg:+mh.toFixed(1),ref_length_cm:s.len,meas_length_cm:+len.toFixed(1),
  length_error_cm:g.complete?+(len-s.len).toFixed(1):'',t_start_s:+res.t[g.i0].toFixed(1),t_end_s:+res.t[g.i1].toFixed(1)};});}
function mapCSV(rows){if(!rows.length)return'';const k=Object.keys(rows[0]);return k.join(',')+'\n'+rows.map(r=>k.map(c=>r[c]===undefined||r[c]===null?'':r[c]).join(',')).join('\n')+'\n';}
function mapPointRows(res,ref){return res.run.map((r,i)=>({t_s:+res.t[i].toFixed(2),s_cm:+(res.dist[i]*res.scale).toFixed(1),x_cm:+res.xs[i].toFixed(1),y_cm:+res.ys[i].toFixed(1),
 heading_deg:+res.yawc[i].toFixed(1),fused_x_cm:+res.fx[i].toFixed(1),fused_y_cm:+res.fy[i].toFixed(1),fused_heading_deg:+res.yaw[i].toFixed(1),pred_x_cm:res.hasPred?+res.px[i].toFixed(1):'',pred_y_cm:res.hasPred?+res.py[i].toFixed(1):'',pred_heading_deg:res.hasPred?+res.kyaw[i].toFixed(1):'',state:res.hasSm?SMNAME[res.run[i].sm]:'',segment:res.labels[i]===null?'corner':ref[res.labels[i]].name,follow_state:r.fs,L:r.L,C:r.C,R:r.R,all_black:(r.k>>3)&1,ev_cross:(r.k>>4)&1}));}
function mapSVG(res,ref,title,opt){
 opt=Object.assign({drawing:true,pred:true,fused:true,mapped:true},opt||{});
 const rp=mapRefPoly(ref),ax=[].concat(rp.map(p=>p[0])),ay=[].concat(rp.map(p=>p[1]));
 if(opt.mapped){ax.push(...res.xs);ay.push(...res.ys);}if(opt.fused){ax.push(...res.fx);ay.push(...res.fy);}if(opt.pred&&res.hasPred){ax.push(...res.px);ay.push(...res.py);}
 const ent=[];
 if(opt.drawing)ent.push(['b','#999','drawing',85]);
 if(opt.pred&&res.hasPred)ent.push(['d','#8d6e63','predicted (wheel model)',170]);
 if(opt.fused)ent.push(['l','#2f9e44','fused (gyro + wheel)',150]);
 if(opt.mapped)[[1,'FOLLOW'],[2,'CROSS hold'],[3,'corner TURN'],[4,'SEARCH']].forEach(([st,nm])=>ent.push(['l',MAPCOL[st],'mapped '+nm,145]));
 if(res.hasSm&&res.episodes.length)ent.push(['l','#7048e8','overtaking path',120]);
 ent.push(['r','#e03131','car now',70]);
 {let lx=10,row=0;ent.forEach(e=>{if(lx+e[3]>900){lx=10;row++;}e.push(lx,row);lx+=e[3];});} // legend wraps onto new rows (own x and row per entry)
 const rows=ent[ent.length-1][5]+1;
 const x0=Math.min(...ax)-30,x1=Math.max(...ax)+30,y0=Math.min(...ay)-30,y1=Math.max(...ay)+30,W=900,k=W/(x1-x0),H=Math.round((y1-y0)*k)+70+(rows-1)*18;
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
 if(res.hasSm){res.episodes.forEach(e=>{const xs=[],ys=[];for(let i=e.i0;i<=e.i1;i++){xs.push(res.fx[i]);ys.push(res.fy[i]);}if(xs.length>1)o.push(line(xs,ys,'#7048e8',5,'',0.55));});
  res.objects.forEach(g=>{const cx=+X(g.x),cy=+Y(g.y);
   if(g.kind==='obstacle')o.push('<circle cx="'+cx+'" cy="'+cy+'" r="9" fill="#795548" stroke="#fff" stroke-width="1.5"/><text x="'+(cx+12)+'" y="'+(cy-6)+'" font-size="12" fill="#5d4037">obstacle</text>');
   else if(g.kind==='stop'){const pts=[0,1,2,3,4,5,6,7].map(j=>(cx+9*Math.cos(Math.PI/8+j*Math.PI/4)).toFixed(1)+','+(cy+9*Math.sin(Math.PI/8+j*Math.PI/4)).toFixed(1)).join(' ');o.push('<polygon points="'+pts+'" fill="#e03131" stroke="#fff" stroke-width="1.5"/><text x="'+cx+'" y="'+(cy+3.5)+'" font-size="7" font-weight="bold" fill="#fff" text-anchor="middle">STOP</text>');}
   else o.push('<rect x="'+(cx-5)+'" y="'+(cy-10)+'" width="10" height="20" rx="3" fill="#212529"/><circle cx="'+cx+'" cy="'+(cy-4)+'" r="3" fill="#fa5252"/><circle cx="'+cx+'" cy="'+(cy+4)+'" r="3" fill="#40c057"/>');});}
 const ex=opt.fused?res.fx:(opt.pred&&res.hasPred?res.px:res.xs),ey=opt.fused?res.fy:(opt.pred&&res.hasPred?res.py:res.ys),e=ex.length-1;
 o.push('<circle cx="'+X(ex[e])+'" cy="'+Y(ey[e])+'" r="5" fill="none" stroke="#e03131" stroke-width="2"/>');
 o.push('<circle cx="'+X(0)+'" cy="'+Y(0)+'" r="6" fill="#2b8a3e"/><text x="'+(+X(0)+8)+'" y="'+(+Y(0)+16)+'" font-size="13" fill="#2b8a3e">start C</text>');
 ent.forEach(([t,c,nm,w,lx,row])=>{const ly=H-12-(rows-1-row)*18;
  if(t==='r')o.push('<circle cx="'+(lx+5)+'" cy="'+(ly-3)+'" r="4" fill="none" stroke="'+c+'" stroke-width="2"/>');
  else if(t==='b')o.push('<rect x="'+lx+'" y="'+(ly-7)+'" width="14" height="8" fill="'+c+'" fill-opacity="0.35"/>');
  else o.push('<rect x="'+lx+'" y="'+(ly-4)+'" width="14" height="4" fill="'+c+'"/>');
  o.push('<text x="'+(lx+18)+'" y="'+ly+'" font-size="12">'+nm+'</text>');});
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

/*MAPUI - Test13 tablet recording / live map (predicted + fused + mapped layers, heading chart, target-following runs) / downloads (no PC needed)*/
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
function mapRunLabel(r,i){const d=((r[r.length-1].t-r[0].t)/1000).toFixed(0);return (r.kind===7?'Follow run ':'Run ')+(i+1)+' - '+r.length+' samples, '+d+' s';}
const MAPLY=['mapLyD','mapLyP','mapLyF','mapLyM'];
function mapLayers(){return{drawing:$('mapLyD').checked,pred:$('mapLyP').checked,fused:$('mapLyF').checked,mapped:$('mapLyM').checked};}
function mapLayerChanged(){try{localStorage.setItem('a3layers',JSON.stringify(MAPLY.map(id=>$(id).checked)));}catch(e){}mapRefresh(true);}
function mapLayerRestore(){try{const v=JSON.parse(localStorage.getItem('a3layers')||'null');if(Array.isArray(v)&&v.length===4)MAPLY.forEach((id,i)=>{$(id).checked=!!v[i];});}catch(e){}}
function mapStyle(svg){return svg.replace('<svg ','<svg style="width:100%;height:auto;border-radius:8px;margin:6px 0" ');}
function mapCurSVG(){const L=mapLayers();return mapRes.kind===7?mapFollowSVG(mapRes,'Follow run '+mapRes.no,L):mapSVG(mapRes,mapRef,'Run '+mapRes.no,L);}
function mapCurHeading(){return mapRes.kind===7?mapFollowCharts(mapRes)[0]:mapHeadingSVG(mapRes,mapRef);}
function mapRefresh(force){
 mapGyroReadout();
 const age=mapLastOk?((Date.now()-mapLastOk)/1000).toFixed(0):'-',live=mapSamples[mapSamples.length-1];
 const st=['IDLE','FOLLOW','CROSS','TURN','SEARCH','FAIL','RED STOP'];
 $('mapStatus').textContent='Recorded '+mapLines.length+' lines / '+mapSamples.length+' samples on this tablet'+
  (mapLost?' | LOST '+mapLost+' (offline > ~3 min)':' | nothing lost')+' | last contact '+age+' s ago'+
  (live?' | car: '+(live.mode===6?'AUTO '+(st[live.fs]||live.fs):live.mode===7?'FOLLOW '+(['','TRACK','ARRIVED','TARGET LOST','SEARCHING'][live.fs]||live.fs)+', target width '+live.tgtW+' per mille':live.mode===5?'MANUAL':'stopped')+', yaw '+live.yaw.toFixed(0)+' deg'+(live.mode===7?'':', L/C/R '+live.L+'/'+live.C+'/'+live.R):'')+(mapStatus221&&mapStatus221.LP?' | laps done '+mapStatus221.LP:'')+
  (live&&live.mode===6&&mapStatus221&&mapStatus221.SG!=null?' | route: on '+['C-O-D','D-E','E-O-A','A-B','B-C'][mapStatus221.SG]+', next corner '+['D RIGHT','E RIGHT','A LEFT','B LEFT','C LEFT'][mapStatus221.SG]:'')+
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
 if(!mapRunsCache.length){$('mapView').innerHTML='<p class="hint">No driving data yet. Press Start continuous run; the map appears here while the car drives.</p>';$('mapSummary').textContent='';$('mapSegTable').innerHTML='';$('mapHeading').innerHTML='';$('mapStates').innerHTML='';$('mapTrans').innerHTML='';mapRes=null;return;}
 const idx=sel.value==='live'?mapRunsCache.length-1:+sel.value,run=mapRunsCache[idx],lay=mapLayers();
 if(run.kind===7){
  mapRes=mapBuildFollow(run);mapRes.kind=7;mapRes.no=idx+1;
  $('mapView').innerHTML=mapStyle(mapFollowSVG(mapRes,'Follow run '+(idx+1),lay));
  $('mapHeading').innerHTML=mapFollowCharts(mapRes).map(mapStyle).join('');
  $('mapSummary').textContent=mapFollowSummary(mapRes,idx+1,mapRunsCache.length);
  $('mapSegTable').innerHTML='';$('mapStates').innerHTML='';$('mapTrans').innerHTML='';return;
 }
 mapRes=mapBuildRun(run,mapRef);mapRes.kind=6;mapRes.no=idx+1;
 $('mapView').innerHTML=mapStyle(mapSVG(mapRes,mapRef,'Run '+(idx+1),lay));
 $('mapHeading').innerHTML=mapStyle(mapHeadingSVG(mapRes,mapRef));
 $('mapSummary').textContent=mapSummary(mapRes,mapRef,idx+1,mapRunsCache.length);
 $('mapStates').innerHTML=mapStyle(mapStateSVG(mapRes));
 $('mapTrans').innerHTML=mapRes.trans&&mapRes.trans.length?'<details><summary><b>State transitions 状态转换 ('+mapRes.trans.length+')</b></summary><table style="width:100%;font-size:13px"><tr><th>t (s)</th><th>from</th><th>to</th><th>x, y (cm)</th><th>in state (s)</th></tr>'+mapRes.trans.map(t=>'<tr><td>'+t.t.toFixed(1)+'</td><td>'+(t.from===null?'':SMNAME[t.from])+'</td><td><b>'+SMNAME[t.to]+'</b></td><td>'+t.x.toFixed(0)+', '+t.y.toFixed(0)+'</td><td>'+t.dwell.toFixed(1)+'</td></tr>').join('')+'</table></details>':'';
 const rows=mapSegRows(mapRes,mapRef);
 $('mapSegTable').innerHTML=rows.length?'<tr><th>#</th><th>Segment</th><th>Complete</th><th>Turn expected/measured</th><th>Heading ref/meas</th><th>Length ref/meas (cm)</th></tr>'+
  rows.map(r=>'<tr><td>'+r.order+'</td><td>'+r.segment+'</td><td>'+(r.complete?'yes':'—')+'</td><td'+(r.turn_check==='WRONG WAY'?' style="color:#fa5252;font-weight:bold"':'')+'>'+(r.expected_turn_deg===''?'—':r.expected_turn_deg+' / '+r.measured_turn_deg+(r.turn_check==='WRONG WAY'?' WRONG WAY':''))+'</td><td>'+r.ref_heading_deg+' / '+r.meas_heading_deg+'</td><td>'+r.ref_length_cm+' / '+r.meas_length_cm+'</td></tr>').join(''):'';
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
   const cols=['tablet_ms','esp_idx','esp_ms','q','t_ms','L','C','R','PL','PR','gyro_rate_dps','yaw_deg','dist_cm','x_cm','y_cm','ultrasonic_cm','mode','follow_state','gyro_ok','L_seen','C_seen','R_seen','all_black','ev_cross','ev_turn','ev_search','ev_estop','pred_heading_deg','pred_x_cm','pred_y_cm','tgt_heading_err_deg','tgt_width_permille','tgt_bearing_deg'];
   const csv=cols.join(',')+'\n'+mapSamples.map(s=>[s.pc,s.idx,s.esp,s.q,s.t,s.L,s.C,s.R,s.PL,s.PR,s.g,s.yaw,s.dist,s.x,s.y,s.u,s.mode,s.fs,s.gy].concat([0,1,2,3,4,5,6,7].map(b=>(s.k>>b)&1)).concat([s.kh,s.kx,s.ky,s.tgtErr,s.tgtW,s.tgtBear]).map(v=>v===undefined||v===null?'':v).join(',')).join('\n')+'\n';
   mapSaveFile('map_samples_'+st+'.csv',csv,'text/csv');
  }else mapSaveFile('map_all_lines_'+st+'.txt','esp_idx\tesp_ms\ttablet_ms\tuno_line\n'+mapLines.join('\n')+'\n');
  mapDownloadedCount=mapLines.length;mapRefresh(false);return;
 }
 if(!mapRes){$('message').textContent='No driving run to export yet.';return;}
 const tag='run'+mapRes.no+'_'+st;
 if(kind==='svg')mapSaveFile('map_'+tag+'.svg',mapCurSVG(),'image/svg+xml');
 if(kind==='trans'){if(!mapRes.trans||!mapRes.trans.length){$('message').textContent='No state transitions in this run (needs the Test13 UNO).';return;}mapSaveFile('state_transitions_'+tag+'.csv',mapCSV(mapTransRows(mapRes)),'text/csv');}
 if(kind==='spng'){if(mapRes.kind===7||!mapRes.hasSm){$('message').textContent='No state log in this run.';return;}mapSavePNG(mapStateSVG(mapRes),'state_timeline_'+tag+'.png');}
 if(kind==='hsvg')mapSaveFile('heading_'+tag+'.svg',mapCurHeading(),'image/svg+xml');
 if(kind==='seg'){if(mapRes.kind===7){$('message').textContent='Segments table is for line-following runs only.';return;}mapSaveFile('map_segments_'+tag+'.csv',mapCSV(mapSegRows(mapRes,mapRef)),'text/csv');}
 if(kind==='points')mapSaveFile('map_points_'+tag+'.csv',mapCSV(mapRes.kind===7?mapFollowRows(mapRes):mapPointRows(mapRes,mapRef)),'text/csv');
 if(kind==='summary')mapSaveFile('map_summary_'+tag+'.txt',$('mapSummary').textContent+'\n');
 if(kind==='png')mapSavePNG(mapCurSVG(),'map_'+tag+'.png');
 if(kind==='hpng')mapSavePNG(mapCurHeading(),'heading_'+tag+'.png');
}
function mapSavePNG(svg,name){
 const img=new Image(),url=URL.createObjectURL(new Blob([svg],{type:'image/svg+xml'}));
 img.onload=()=>{try{const c=document.createElement('canvas');c.width=img.width*2;c.height=img.height*2;const g=c.getContext('2d');g.scale(2,2);g.drawImage(img,0,0);URL.revokeObjectURL(url);
  c.toBlob(b=>{if(!b){$('message').textContent='PNG export not supported here: use the SVG button.';return;}const u=URL.createObjectURL(b),a=document.createElement('a');a.href=u;a.download=name;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(u),60000);$('message').textContent='Saved '+name;},'image/png');}
  catch(e){$('message').textContent='PNG export failed on this browser: use the SVG button.';}};
 img.onerror=()=>{$('message').textContent='PNG export failed on this browser: use the SVG button.';};img.src=url;
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
mapRestore();mapLoadRefInputs();mapLayerRestore();mapRefresh(true);setInterval(mapPoll,500);
/*END MAPUI*/


// ---- Assignment 4 options (camera pan, overtaking) ----
const A4IDS=['PA','PL','PM','PS','OS','OT','OP','OJ'];
function saveA4(){try{localStorage.setItem('a3a4',JSON.stringify(A4IDS.map(i=>$(i).value).concat([+$('OV').checked,$('SR')?$('SR').value:'35'])));}catch(e){}}
function loadA4(){try{const v=JSON.parse(localStorage.getItem('a3a4')||'null');if(Array.isArray(v)&&v.length>=9){A4IDS.forEach((i,k)=>{$(i).value=v[k];});$('OV').checked=!!v[8];}}catch(e){}}
async function panTest(a){try{$('panMsg').textContent=await post('/cmd',{action:'servo',value:Math.max(0,Math.min(180,Math.round(a)))});$('panMsg').textContent='Camera turned to '+a+' degrees (stop the car first; a run resets it to centre).';}catch(e){$('panMsg').textContent=e.message;}}
A4IDS.concat(['OV']).forEach(i=>$(i).addEventListener('change',saveA4));

// ---- race options and target following (Test11) ----
function saveRace(){try{localStorage.setItem('a3race',JSON.stringify([+$('WG').checked,+$('LAPS').value||0]));}catch(e){}}
function loadRace(){try{const v=JSON.parse(localStorage.getItem('a3race')||'null');if(Array.isArray(v)){$('WG').checked=!!v[0];$('LAPS').value=v[1]||0;}}catch(e){}}
const FOLLOWIDS=['FC','FW','FF','FKP','FKD'];
function saveFollow(){try{localStorage.setItem('a3follow',JSON.stringify(FOLLOWIDS.map(i=>$(i).value).concat([+$('FM').checked])));}catch(e){}}
function loadFollow(){try{const v=JSON.parse(localStorage.getItem('a3follow')||'null');if(Array.isArray(v)&&v.length===6){FOLLOWIDS.forEach((i,k)=>{$(i).value=v[k];});$('FM').checked=!!v[5];}}catch(e){}}
FOLLOWIDS.concat(['FM']).forEach(i=>$(i).addEventListener('change',saveFollow));
async function startFollow(){
 if(starting||settingsBusy){$('message').textContent='Wait for the pending command.';return;}
 starting=true;
 try{
  const p={color:$('FC').value,w:Number($('FW').value),fov:Number($('FF').value),mirror:+$('FM').checked,kp:Number($('FKP').value),kd:Number($('FKD').value)};
  if(![p.w,p.fov,p.kp,p.kd].every(Number.isInteger))throw Error('Use whole numbers for the follow settings');
  saveFollow();
  const wanted=readSettingsForm(),current=JSON.parse(await request('/traffic-settings'));
  if(!settingsMatch(current,wanted)&&!await saveSettings())return;
  const before=JSON.parse(await request('/traffic-status'));
  await post('/follow-start',p);let confirmed=false;
  for(let i=0;i<16;i++){await delay(300);const d=JSON.parse(await request('/traffic-status'));
   if(d.age_ms<1500&&d.uno?.M===7&&d.uno.RN!==before.uno?.RN){confirmed=true;break;}}
  if(!confirmed)throw Error('Follow confirmation missing; inspect car state before retrying.');
  driving=true;$('message').textContent='Following the '+(p.color==='g'?'green':'red')+' target 跟随中';
 }catch(e){$('message').textContent=e.message;}
 finally{starting=false;}}
function followLive(d){
 const o=d.objects||{},t=o[$('FC').value==='r'?'red':'green']||{},u=d.uno||{};
 const w=t.valid&&t.seen&&t.image_w>0?Math.round(1000*t.width/t.image_w):null;
 $('fwSeen').textContent=!t.valid?'—':t.seen?'Yes':'No';$('fwSeen').className='num'+(t.valid&&t.seen?' ok':'');
 $('fwW').textContent=(w===null?'—':w)+' / '+$('FW').value;
 $('fwSt').textContent=d.age_ms<1500&&u.M===7?(names[u.TS]||'—'):'—';
}
let savedTab='drive';try{savedTab=localStorage.getItem('a3tab')||'drive';}catch(e){}
loadRace();loadFollow();loadA4();switchTab(savedTab);refreshPreview();previewAge();loadSettings();loadCamera();poll();heartbeat();
</script></body></html>
)HTML";
