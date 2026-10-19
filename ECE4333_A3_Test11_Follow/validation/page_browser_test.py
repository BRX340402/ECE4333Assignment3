import json,re,sys
from playwright.sync_api import sync_playwright
html=open('/home/claude/pkg/Test11/ESP32_Assignment2/TrafficPage.h').read().split('R"HTML(',1)[1].split(')HTML";',1)[0]
lines=[l for l in open('/tmp/run.jsonl').read().split('\n') if l]+[l for l in open('/tmp/pt/follow.jsonl').read().split('\n') if l]
calls=[];state={'M':0,'RN':1}
def uno():
    return dict(M=state['M'],TS=9 if state['M']==7 else 0,ID=0,L=100,C=900,R=100,GY=1,UV=41,FR=500,UT=123456,FC=0,PL=0,PR=0,ES=0,RN=state['RN'],GN=1,ZR=0,MQ=0,IM=1,IW=104,IQ=0,IB=0,CB=0,GL=0,GF=0,SG=0,LP=1)
def handler(route):
    req=route.request;u=req.url.split('car.test',1)[1];path=u.split('?')[0]
    def j(o,c=200):route.fulfill(status=c,content_type='application/json',body=json.dumps(o))
    if path=='/' or path=='/index.html' or path=='/traffic':
        return route.fulfill(status=200,content_type='text/html',body=html)
    if path=='/traffic-status':
        obj={k:dict(valid=1,seen=1 if k=='green' else 0,pixels=900,width=60,distance_cm=-1,box_valid=1,x=10,y=10,w=60,h=40,image_w=240,image_h=176,captured=1) for k in ('red','green','stop')}
        return j(dict(esp_ms=1,sensors={},sensors_age_ms=10,vision_health={},uno=uno(),age_ms=100,calibration={},settings=dict(applied=1,SC=25,BL=600,BH=1023,RA=180,SA=360,CT=1500,GS=1,CS=60,DS=30,RP=100,RS=0),framesize=5,objects=obj,detect_enabled=True,color_enabled=True))
    if path=='/traffic-settings':
        return j(dict(applied=1,SC=25,BL=600,BH=1023,RA=180,SA=360,CT=1500,GS=1,CS=60,DS=30,RP=100,RS=0))
    if path=='/map-log':
        body='#lines=%d oldest=0 error=\n'%len(lines)+'\n'.join('%d\t%d\t%s'%(i,i*100,l) for i,l in enumerate(lines))
        return route.fulfill(status=200,content_type='text/plain',body=body)
    if path in('/follow-start','/traffic-start','/cmd'):
        calls.append((path,req.post_data or u));
        if path=='/follow-start':state['M']=7;state['RN']+=1
        if path=='/traffic-start':state['M']=6;state['RN']+=1
        return route.fulfill(status=200,content_type='text/plain',body='ok')
    return route.fulfill(status=200,content_type='application/json',body='{}')
errs=[]
with sync_playwright() as p:
    b=p.chromium.launch(args=['--disable-gpu']);pg=b.new_page(viewport={'width':480,'height':900})
    pg.on('pageerror',lambda e:errs.append(str(e)));pg.on('console',lambda m:errs.append('console:'+m.text) if m.type=='error' else None)
    pg.route('http://car.test/**',handler);pg.goto('http://car.test/');pg.wait_for_timeout(1500)
    pg.screenshot(path='/tmp/pt/drive.png',full_page=True)
    # follow button
    pg.fill('#FW','300');pg.select_option('#FC','r');pg.check('#FM');pg.click('#followBtn');pg.wait_for_timeout(2500)
    print('follow calls',calls)
    print('message:',pg.inner_text('#message'))
    # race options
    pg.check('#WG');pg.fill('#LAPS','2');state['M']=0;pg.evaluate("startCar()");pg.wait_for_timeout(2500);print('start calls',calls[-1])
    # map tab
    pg.evaluate("switchTab('map')");pg.wait_for_timeout(2500)
    opts=pg.eval_on_selector_all('#mapRun option','o=>o.map(x=>[x.value,x.textContent])');print('runs:',opts)
    pg.screenshot(path='/tmp/pt/map_tab.png',full_page=True)
    print('summary:\n'+pg.inner_text('#mapSummary'))
    errs0=list(errs)
    # layer toggle
    pg.uncheck('#mapLyP');pg.wait_for_timeout(500);print('pred layer gone:', 'predicted (wheel model)' not in pg.inner_html('#mapView'))
    pg.check('#mapLyP')
    # select follow run
    fv=[v for v,t in opts if 'Follow' in t]
    pg.select_option('#mapRun',fv[0]);pg.wait_for_timeout(800)
    print('follow summary:\n'+pg.inner_text('#mapSummary'))
    pg.screenshot(path='/tmp/pt/follow_tab.png',full_page=True)
    b.close()
print('ERRORS',errs)
