
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
bool copyPreview(camera_fb_t &out,size_t &capacity,uint32_t &captured,uint16_t &epoch,uint32_t after=0){
 bool ok=false;
 xSemaphoreTake(stateMutex,portMAX_DELAY);
 if(!resolutionBusy&&previewJpeg&&previewLen&&previewEpoch==geometryEpoch&&
    millis()-previewCaptured<1200&&(!after||previewCaptured!=after)&&
    reserveJpeg(out.buf,capacity,previewLen)){
  memcpy(out.buf,previewJpeg,previewLen);out.len=previewLen;
  out.width=previewWidth;out.height=previewHeight;out.format=PIXFORMAT_JPEG;
  captured=previewCaptured;epoch=previewEpoch;ok=true;
 }
 xSemaphoreGive(stateMutex);return ok;
}
void captureTask(void*){
 uint8_t *staging=nullptr;size_t capacity=0;
 for(;;){
  if(!resolutionBusy){
   xSemaphoreTake(cameraMutex,portMAX_DELAY);
   if(resolutionBusy){xSemaphoreGive(cameraMutex);vTaskDelay(pdMS_TO_TICKS(20));continue;}
   camera_fb_t *rf=esp_camera_fb_get();uint32_t stamp=millis();int w=0,h=0;
   bool ok=rf&&rf->format==PIXFORMAT_JPEG&&jpegSize(rf->buf,rf->len,w,h)&&
    w==(int)rf->width&&h==(int)rf->height;
   size_t n=ok?rf->len:0;
   ok=ok&&reserveJpeg(staging,capacity,n);
   if(ok)memcpy(staging,rf->buf,n);
   if(rf)esp_camera_fb_return(rf); // Return driver storage before any processing.
   xSemaphoreTake(stateMutex,portMAX_DELAY);
   if(ok){
    uint8_t *old=previewJpeg;size_t oldCapacity=previewCapacity;
    previewJpeg=staging;previewCapacity=capacity;previewLen=n;
    previewCaptured=stamp;previewWidth=w;previewHeight=h;previewEpoch=geometryEpoch;
    staging=old;capacity=oldCapacity;cameraFrameCount++;
   }else cameraFrameErrors++;
   xSemaphoreGive(stateMutex);xSemaphoreGive(cameraMutex);
  }
  vTaskDelay(pdMS_TO_TICKS(100));
 }
}
void colourTask(void*){
 camera_fb_t local={};size_t capacity=0;uint32_t lastStamp=0;uint16_t seq=0;
 for(;;){
  uint32_t start=millis(),captured=0;uint16_t epoch=0;
  if(!colorEnabled||!copyPreview(local,capacity,captured,epoch,lastStamp)){
   vTaskDelay(pdMS_TO_TICKS(40));continue;
  }
  lastStamp=captured;RedStatus next;next.seq=++seq;next.captured=captured;
  xSemaphoreTake(decoderMutex,portMAX_DELAY);
  next.valid=decodeRed(&local,redCells,redQueue,next.region,greenCells,&next.green,redMinPixels.load());
  xSemaphoreGive(decoderMutex);
  next.seen=next.valid&&next.region.pixels>=redMinPixels.load();
  next.greenSeen=next.valid&&next.green.pixels>=redMinPixels.load();next.ms=millis()-start;
  xSemaphoreTake(stateMutex,portMAX_DELAY);
  // A settings change must not republish a result from the previous geometry.
  if(!resolutionBusy&&colorEnabled&&epoch==geometryEpoch)redStatus=next;
  xSemaphoreGive(stateMutex);
  vTaskDelay(pdMS_TO_TICKS(100));
 }
}

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
