"""Insert the Test10 tablet mapping section (tools/map_core.js + tools/map_ui.js) into
ESP32_Assignment2/TrafficPage.h, then run tools/build_web_page.py. Safe to run repeatedly."""
from pathlib import Path
import re, subprocess, sys
root = Path(__file__).resolve().parents[1]
page_h = root / "ESP32_Assignment2/TrafficPage.h"
s = page_h.read_text()
head, rest = s.split('R"HTML(', 1)
html, tail = rest.split(')HTML";', 1)

SECTION = """<!--MAPSECTION--><section id="mapSection"><h2>Road mapping / 道路测图 (Test10)</h2>
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
"""
html = re.sub(r"<!--MAPSECTION-->.*?<!--/MAPSECTION-->\n?", "", html, flags=re.S)
anchor = next((a for a in ("<section><h2>Optional route scan", "<section><h2>Old snapshot table") if a in html), None)
assert anchor, "anchor section not found"
html = html.replace(anchor, SECTION + anchor, 1)
html = html.replace("<section><h2>Optional route scan / sensor table</h2>", "<section><h2>Old snapshot table (not needed for mapping)</h2>")

js = (root / "tools/map_core.js").read_text() + "\n" + (root / "tools/map_ui.js").read_text()
html = re.sub(r"/\*MAPCORE.*?/\*END MAPUI\*/\n?", "", html, flags=re.S)
i = html.rindex("</script>")
html = html[:i] + js + "\n" + html[i:]

# Test10 UNO identity
html = html.replace("UV===39", "UV===40").replace("Number(u.UV)===39", "Number(u.UV)===40")
html = html.replace("<small>Test 9</small>", "<small>Test10 mapping</small>")
assert ")HTML\"" not in html
page_h.write_text(head + 'R"HTML(' + html + ')HTML";' + tail)
print("page updated:", len(html), "bytes")
subprocess.run([sys.executable, str(root / "tools/build_web_page.py")], check=True)
