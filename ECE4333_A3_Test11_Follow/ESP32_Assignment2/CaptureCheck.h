#pragma once
#include "ModelShape.h"
// Independent old full-RGB path, run ONLY on a requested snapshot for diagnosis.
struct ReferenceJpeg {const uint8_t *src;size_t len;uint8_t *dst;int w,h;};
static size_t referenceRead(void*a,size_t i,uint8_t*b,size_t n){auto*d=(ReferenceJpeg*)a;if(i>=d->len)return 0;if(n>d->len-i)n=d->len-i;if(b)memcpy(b,d->src+i,n);return n;}
static bool referenceWrite(void*a,uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint8_t*p){auto*d=(ReferenceJpeg*)a;if(!p){if(!x&&!y)return w==d->w&&h==d->h;return true;}if(x+w>d->w||y+h>d->h)return false;for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++){uint8_t*q=d->dst+3*((y+yy)*d->w+x+xx);const uint8_t*t=p+3*(yy*w+xx);q[0]=t[2];q[1]=t[1];q[2]=t[0];}return true;}
static int checkSnapshotInput(const camera_fb_t *fb,const uint8_t *input,int w,int h,const ViewMap &v){
 uint8_t*full=(uint8_t*)ps_malloc(w*h*3);if(!full)return -1;
 int scale=0;while(scale<3&&((fb->width>>scale)>400||(fb->height>>scale)>400))scale++;
 ReferenceJpeg d={fb->buf,fb->len,full,w,h};int result=-1;
 if(esp_jpg_decode(fb->len,(jpg_scale_t)scale,referenceRead,referenceWrite,&d)==ESP_OK){result=1;for(int y=0;y<MODEL_SIDE;y++)for(int x=0;x<MODEL_SIDE;x++){int sx,sy;bool valid=viewPixel(v,x,y,sx,sy);for(int c=0;c<3;c++)if(input[3*(y*MODEL_SIDE+x)+c]!=(valid?full[3*(sy*w+sx)+c]:0))result=0;}}
 free(full);return result;
}
