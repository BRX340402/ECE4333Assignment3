from pathlib import Path
root=Path(__file__).resolve().parents[1]
s=(root/'ESP32_Assignment2/ESP32_Assignment2.ino').read_text()
# Exercise the current startup code, leaving every unrequested ISP setter null.
old=(root/'tests/driver_defaults_generated.cpp').read_text()
prefix=old[:old.index('bool cameraInit(){')]
prefix=prefix.replace('assert(n==FRAMESIZE_QVGA)','assert(n==FRAMESIZE_SVGA)')
prefix+='\nint orientationCalls=0;int setOrientation(sensor_t*,int v){assert(v==0);orientationCalls++;return 0;}\n'
functions=s[s.index('bool cameraInit(){'):s.index('void loadCalibration(')]
checks=old[old.index('int main(){'):]
checks=checks.replace('sensor.set_framesize=setSize;', 'sensor.set_framesize=setSize;sensor.set_vflip=setOrientation;sensor.set_hmirror=setOrientation;')
checks=checks.replace('received.jpeg_quality==12&&received.fb_count==1','received.jpeg_quality==10&&received.fb_count==2&&orientationCalls==2')
checks=checks.replace('PASS V11 driver defaults: no ISP overrides','PASS V12 vendor startup: quality10/two buffers/SVGA/final orientation; no white-balance or exposure overrides')
(root/'tests/vendor_defaults_generated.cpp').write_text(prefix+functions+checks)

# Interleave real cache/capture/colour code using injected sensor/decoder calls.
# The decoder schedules more captures while retaining its private JPEG copy.
stub=r'''
#include <cassert>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <iostream>
int allocations=0;bool failAllocation=false;
void *ps_malloc(size_t n){if(failAllocation)return nullptr;allocations++;return malloc(n);}
#include "../ESP32_Assignment2/JpegBuffer.h"
#include "../ESP32_Assignment2/JpegSize.h"
#include "../ESP32_Assignment2/RedDetector.h"
struct camera_fb_t{uint8_t*buf=nullptr;size_t len=0,width=0,height=0;int format=0;};
enum{PIXFORMAT_JPEG=0,portMAX_DELAY=0,pdTRUE=1};
#define pdMS_TO_TICKS(x) (x)
int cameraMutex=0,stateMutex=1,decoderMutex=2;bool locks[3]={};
int xSemaphoreTake(int m,int){assert(!locks[m]);locks[m]=true;return 1;}
void xSemaphoreGive(int m){assert(locks[m]);locks[m]=false;}
uint32_t now=1000;uint32_t millis(){return now;}
struct EndTask{};std::vector<int> budgets;
void vTaskDelay(int n){now+=n;if(--budgets.back()==0)throw EndTask{};}
void runTask(void(*fn)(void*),int n){budgets.push_back(n);try{fn(nullptr);}catch(EndTask&){}budgets.pop_back();}
std::atomic<bool>resolutionBusy{false},colorEnabled{true};std::atomic<int>redMinPixels{100};
uint16_t geometryEpoch=0,previewEpoch=0;
uint8_t *previewJpeg=nullptr;size_t previewLen=0,previewCapacity=0;uint32_t previewCaptured=0;
int previewWidth=0,previewHeight=0;uint32_t cameraFrameCount=0,cameraFrameErrors=0;
struct RedStatus{bool valid=false,seen=false,greenSeen=false;uint32_t captured=0,ms=0;uint16_t seq=0;RedRegion region,green;}redStatus;
uint8_t*redCells=nullptr,*greenCells=nullptr;uint16_t*redQueue=nullptr;
uint8_t sensorBytes[1200];camera_fb_t driver;bool driverHeld=false,badJpeg=false;int sensorSeq=0;
camera_fb_t*esp_camera_fb_get(){assert(locks[cameraMutex]&&!driverHeld);driverHeld=true;sensorSeq++;memset(sensorBytes,sensorSeq,sizeof(sensorBytes));
 const uint8_t header[]={0xff,0xd8,0xff,0xc0,0,8,8,0,240,1,64,3};memcpy(sensorBytes,header,sizeof(header));
 if(badJpeg)sensorBytes[0]=0;
 driver.buf=sensorBytes;driver.len=sizeof(sensorBytes);driver.width=320;driver.height=240;driver.format=PIXFORMAT_JPEG;return &driver;
}
void esp_camera_fb_return(camera_fb_t*p){assert(locks[cameraMutex]&&p==&driver&&driverHeld);driverHeld=false;memset(sensorBytes,0xcc,sizeof(sensorBytes));}
void captureTask(void*);
bool changeEpochDuringDecode=false;
bool decodeRed(const camera_fb_t*p,uint8_t*,uint16_t*,RedRegion&red,uint8_t*,RedRegion*green,int){
 assert(locks[decoderMutex]&&!locks[cameraMutex]&&!driverHeld);
 std::vector<uint8_t> before(p->buf,p->buf+p->len);
 auto frames=cameraFrameCount;runTask(captureTask,4);assert(cameraFrameCount==frames+4);
 assert(memcmp(before.data(),p->buf,p->len)==0); // New captures cannot overwrite processing input.
 if(changeEpochDuringDecode){xSemaphoreTake(stateMutex,0);geometryEpoch++;xSemaphoreGive(stateMutex);}
 red.pixels=321;green->pixels=222;return true;
}
'''
copy=s[s.index('bool copyPreview('):s.index('uint32_t copiedCaptured=0;')]
tasks=s[s.index('void captureTask(void*){'):s.index('void sendLivePreview(){')]
checks=r'''
int main(){
 runTask(captureTask,12);assert(cameraFrameCount==12&&cameraFrameErrors==0&&allocations==2&&!driverHeld);
 assert(previewJpeg[100]==12&&previewWidth==320&&previewHeight==240);
 camera_fb_t copy;size_t capacity=0;uint32_t stamp=0;uint16_t epoch=0;
 assert(copyPreview(copy,capacity,stamp,epoch));auto saved=std::vector<uint8_t>(copy.buf,copy.buf+copy.len);
 int before=allocations;assert(copyPreview(copy,capacity,stamp,epoch)&&allocations==before);
 assert(!copyPreview(copy,capacity,stamp,epoch,stamp));
 badJpeg=true;runTask(captureTask,1);badJpeg=false;assert(cameraFrameCount==12&&cameraFrameErrors==1&&previewJpeg[100]==12);
 failAllocation=true;runTask(captureTask,1);failAllocation=false;assert(cameraFrameCount==12&&cameraFrameErrors==2&&previewJpeg[100]==12);
 runTask(colourTask,1);assert(redStatus.valid&&redStatus.seen&&redStatus.greenSeen&&redStatus.region.pixels==321);
 assert(std::memcmp(copy.buf,saved.data(),saved.size())==0);
 redStatus.valid=false;changeEpochDuringDecode=true;runTask(colourTask,1);assert(!redStatus.valid);
 assert(!copyPreview(copy,capacity,stamp,epoch)); // Old settings epoch is not usable.
 changeEpochDuringDecode=false;runTask(captureTask,1);assert(copyPreview(copy,capacity,stamp,epoch));
 resolutionBusy=true;assert(!copyPreview(copy,capacity,stamp,epoch));resolutionBusy=false;
 now+=1300;assert(!copyPreview(copy,capacity,stamp,epoch));
 auto ptr=copy.buf;size_t cap=capacity;failAllocation=true;assert(!reserveJpeg(copy.buf,capacity,capacity+1)&&copy.buf==ptr&&capacity==cap);
 assert(!reserveJpeg(copy.buf,capacity,SIZE_MAX));free(copy.buf);free(previewJpeg);
 std::cout<<"PASS V12 capture/cache: 12 frames use 2 staging allocations; immutable reader copies during new captures; no camera lock during colour decode; invalid/allocation-failed frames retain cache; old settings/stale/resolution-busy frames rejected.\n";
}
'''
(root/'tests/capture_v12_generated.cpp').write_text(stub+copy+tasks+checks)
(root/'tests/jpeg_stubs').mkdir(exist_ok=True)
(root/'tests/jpeg_stubs/Arduino.h').write_text('#pragma once\n')
