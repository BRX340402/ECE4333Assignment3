#pragma once
#include "ModelShape.h"
#include "esp_camera.h"
#include "esp_jpg_decode.h"
#include "VisionGeometry.h"
#include "RedDetector.h"
#include <string.h>
static const int ANALYSIS_MAX_W=400,ANALYSIS_MAX_H=400;
struct BoundedJpeg{const uint8_t *src;size_t len;uint8_t *dst;int w,h;int xs[MODEL_SIDE],ys[MODEL_SIDE];uint8_t *redCells;};
static size_t readJpeg(void *arg,size_t index,uint8_t *buf,size_t len){auto*d=(BoundedJpeg*)arg;if(index>=d->len)return 0;if(len>d->len-index)len=d->len-index;if(buf)memcpy(buf,d->src+index,len);return len;}
static bool writeJpeg(void *arg,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t *data){
 auto*d=(BoundedJpeg*)arg;
 if(!data){if(!x&&!y)return w==d->w&&h==d->h;return true;}
 if(x+w>d->w||y+h>d->h)return false;
 if(d->redCells){int gw=(d->w+3)/4;for(int iy=0;iy<h;iy++)for(int ix=0;ix<w;ix++){const uint8_t*p=data+3*(iy*w+ix);if(redPixel(p[0],p[1],p[2]))d->redCells[((y+iy)/4)*gw+(x+ix)/4]++;}}
 // Copy only model samples from each decoded MCU; no full-resolution RGB buffer.
 for(int my=0;my<MODEL_SIDE;my++){int sy=d->ys[my];if(sy<y||sy>=y+h)continue;
  for(int mx=0;mx<MODEL_SIDE;mx++){int sx=d->xs[mx];if(sx<x||sx>=x+w)continue;
   const uint8_t*q=data+3*((sy-y)*w+sx-x);uint8_t*p=d->dst+3*(my*MODEL_SIDE+mx);
   p[0]=q[2];p[1]=q[1];p[2]=q[0];
  }
 }return true;
}
static bool decodeModelInput(const camera_fb_t *fb,uint8_t *dst,int &w,int &h,ViewMap &v,int mode,int cx,int cy,uint8_t *redCells=nullptr,uint16_t *queue=nullptr,RedRegion *region=nullptr,int minimum=100){
 int scale=0;while(scale<3&&((fb->width>>scale)>ANALYSIS_MAX_W||(fb->height>>scale)>ANALYSIS_MAX_H))scale++;
 w=fb->width>>scale;h=fb->height>>scale;v=makeView(w,h,mode,cx,cy);
 BoundedJpeg d={};d.src=fb->buf;d.len=fb->len;d.dst=dst;d.w=w;d.h=h;d.redCells=redCells;if(redCells)memset(redCells,0,10000);
 for(int i=0;i<MODEL_SIDE;i++){
  d.xs[i]=(i<v.padX||i>=v.padX+v.outW)?-1:v.x+((2*(i-v.padX)+1)*v.w)/(2*v.outW);
  d.ys[i]=(i<v.padY||i>=v.padY+v.outH)?-1:v.y+((2*(i-v.padY)+1)*v.h)/(2*v.outH);
 }
 memset(dst,0,MODEL_SIDE*MODEL_SIDE*3);
 bool ok=esp_jpg_decode(fb->len,(jpg_scale_t)scale,readJpeg,writeJpeg,&d)==ESP_OK;
 if(region){*region={};if(ok&&redCells&&queue)*region=largestRedRegion(redCells,queue,w,h,minimum);}
 return ok;
}
