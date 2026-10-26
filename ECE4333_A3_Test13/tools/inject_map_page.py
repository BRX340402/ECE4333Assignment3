"""Build ESP32_Assignment2/TrafficPage.h from tools/page_src.html + the mapping parts
(tools/map_core.js, tools/map_ui.js), then gzip it with tools/build_web_page.py.
Edit page_src.html / map_*.js, never TrafficPage.h directly. Safe to run repeatedly."""
from pathlib import Path
import subprocess, sys
root = Path(__file__).resolve().parents[1]
GYRO = """<section id="gyroSection"><h2>Gyroscope 陀螺仪</h2>
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
"""
MAP = """<section id="mapSection"><h2>Road mapping / 道路测图 (Test13)</h2>
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
<p class="hint">Predicted (brown dashed) = kinematic model only: wheel PWM → speed, Euler integration on the UNO, no gyro. Fused (green) = gyro heading + the same wheel speed, as logged by the UNO (both unscaled, exactly what the car believed). Mapped = fused + this page\'s gyro offset/drift fit, distance scale and lap closure. The summary lists the distance of each track to the drawn centre line. Follow runs (vision target following) are listed in the run menu and show the same layers plus set-point vs gyro heading. Map: grey = assignment drawing; blue = line following, orange = crossing heading-hold, red = corner pivot, purple = search; black dots = all three sensors black (crossing); teal squares = detected corners; pink = stopped for red / STOP sign; red ring = car now. Start each run at corner C facing the STOP sign. Distances are scaled automatically from complete straights.</p>
</section>
"""
html = (root / "tools/page_src.html").read_text()
js = (root / "tools/map_core.js").read_text() + "\n" + (root / "tools/map_ui.js").read_text()
for marker, part in (("<!--GYROSECTION-->", GYRO), ("<!--MAPSECTION-->", MAP), ("/*MAPJS*/", js)):
    assert html.count(marker) == 1, marker
    html = html.replace(marker, part)
assert ')HTML"' not in html
(root / "ESP32_Assignment2/TrafficPage.h").write_text('#pragma once\nstatic const char TRAFFIC_PAGE[] PROGMEM=R"HTML(' + html + ')HTML";\n')
print("page built:", len(html), "bytes")
subprocess.run([sys.executable, str(root / "tools/build_web_page.py")], check=True)
