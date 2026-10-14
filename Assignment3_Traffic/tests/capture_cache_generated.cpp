#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cstdint>
struct camera_fb_t {uint8_t*buf;size_t len,width,height;int format;};
const int PIXFORMAT_JPEG=0,portMAX_DELAY=0;int stateMutex=0;
void xSemaphoreTake(int,int){} void xSemaphoreGive(int){}
uint32_t now=1000;uint32_t millis(){return now;}
bool failAlloc=false;void*ps_malloc(size_t n){return failAlloc?nullptr:malloc(n);}
bool jpegSize(const uint8_t*p,size_t n,int&w,int&h){w=320;h=240;return n==4&&p[0]==255;}
bool resolutionBusy=false;uint8_t bytes[]={255,216,255,217};uint8_t*previewJpeg=bytes;
size_t previewLen=4;uint32_t previewCaptured=900,copiedCaptured=0;camera_fb_t copiedFrame={};
camera_fb_t *copyCameraFrame(){
 camera_fb_t *out=nullptr;
 xSemaphoreTake(stateMutex,portMAX_DELAY);
 if(!resolutionBusy&&previewJpeg&&millis()-previewCaptured<1200){
  int w=0,h=0;
  if(jpegSize(previewJpeg,previewLen,w,h)){
   copiedFrame={};copiedFrame.buf=(uint8_t*)ps_malloc(previewLen);
   if(copiedFrame.buf){memcpy(copiedFrame.buf,previewJpeg,previewLen);copiedFrame.len=previewLen;copiedFrame.width=w;copiedFrame.height=h;copiedFrame.format=PIXFORMAT_JPEG;copiedCaptured=previewCaptured;out=&copiedFrame;}
  }
 }
 xSemaphoreGive(stateMutex);return out;
}

int main(){auto*f=copyCameraFrame();assert(f&&f->width==320&&f->height==240&&copiedCaptured==900);assert(f->buf!=bytes&&!memcmp(f->buf,bytes,4));bytes[1]=0;assert(f->buf[1]==216);free(f->buf);resolutionBusy=true;assert(!copyCameraFrame());resolutionBusy=false;now=2200;assert(!copyCameraFrame());now=1000;bytes[0]=0;assert(!copyCameraFrame());bytes[0]=255;failAlloc=true;assert(!copyCameraFrame());}
