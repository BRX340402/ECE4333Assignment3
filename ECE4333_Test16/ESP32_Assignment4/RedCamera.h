#pragma once
#include "RedDetector.h"
#include "esp_jpg_decode.h"
struct RedJpeg {const uint8_t *src;size_t len;uint8_t *cells;int w,h;uint8_t *green=nullptr;};
static size_t redRead(void *arg,size_t index,uint8_t *buf,size_t len){auto*d=(RedJpeg*)arg;if(index>=d->len)return 0;if(len>d->len-index)len=d->len-index;if(buf)memcpy(buf,d->src+index,len);return len;}
static bool redWrite(void *arg,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t *data){
 auto*d=(RedJpeg*)arg;if(!data){if(!x&&!y)return w==d->w&&h==d->h;return true;}
 if(x+w>d->w||y+h>d->h)return false;
 int gw=(d->w+3)/4;
 for(int iy=0;iy<h;iy++)for(int ix=0;ix<w;ix++){const uint8_t*p=data+3*(iy*w+ix);int cell=((y+iy)/4)*gw+(x+ix)/4;if(redPixel(p[0],p[1],p[2]))d->cells[cell]++;if(d->green&&greenPixel(p[0],p[1],p[2]))d->green[cell]++;}
 return true;
}
static bool decodeRed(const camera_fb_t *fb,uint8_t*cells,uint16_t*queue,RedRegion &out,uint8_t *green=nullptr,RedRegion *greenOut=nullptr,int minimum=100){
 if(!fb||fb->format!=PIXFORMAT_JPEG)return false;
 int scale=0;while(scale<3&&((fb->width>>scale)>400||(fb->height>>scale)>400))scale++;
 int w=fb->width>>scale,h=fb->height>>scale;if(w>400||h>400)return false;
 memset(cells,0,10000);if(green)memset(green,0,10000);RedJpeg d;d.src=fb->buf;d.len=fb->len;d.cells=cells;d.w=w;d.h=h;d.green=green;
 if(esp_jpg_decode(fb->len,(jpg_scale_t)scale,redRead,redWrite,&d)!=ESP_OK)return false;
 out=largestRedRegion(cells,queue,w,h,minimum);if(green&&greenOut)*greenOut=largestRedRegion(green,queue,w,h,minimum);return true;
}
