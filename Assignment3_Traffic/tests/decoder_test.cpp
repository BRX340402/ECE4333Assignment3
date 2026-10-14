#include <cassert>
#include <vector>
#include <cstdio>
#include "../ESP32_Assignment2/CameraDecode.h"
static uint8_t component(int x,int y,int c){return (x*13+y*17+c*71)&255;}
int esp_jpg_decode(size_t,jpg_scale_t,size_t(*)(void*,size_t,uint8_t*,size_t),bool(*write)(void*,uint16_t,uint16_t,uint16_t,uint16_t,uint8_t*),void*arg){
 auto*d=(BoundedJpeg*)arg;assert(write(arg,0,0,d->w,d->h,nullptr));
 for(int y=0;y<d->h;y+=16)for(int x=0;x<d->w;x+=16){int w=std::min(16,d->w-x),h=std::min(16,d->h-y);std::vector<uint8_t> pixels(w*h*3);for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++)for(int c=0;c<3;c++)pixels[3*(yy*w+xx)+c]=component(x+xx,y+yy,c);assert(write(arg,x,y,w,h,pixels.data()));}return 0;
}
int main(){int dims[][2]={{160,120},{240,176},{320,240},{400,296},{640,480},{800,600},{1024,768},{1280,1024},{1600,1200}};int cases=0;
 for(auto&dim:dims)for(int mode=0;mode<4;mode++)for(int center:{0,500,1000}){camera_fb_t fb={nullptr,1,(size_t)dim[0],(size_t)dim[1]};std::vector<uint8_t> out(MODEL_SIDE*MODEL_SIDE*3,255);int w,h;ViewMap v;std::vector<uint8_t> cells(10000);std::vector<uint16_t> queue(10000);RedRegion red;assert(decodeModelInput(&fb,out.data(),w,h,v,mode,center,center,cells.data(),queue.data(),&red,100));
 // Independent full-frame colour count, using the same 4x4 connected-component definition.
 std::vector<uint8_t> reference(10000);int gw=(w+3)/4;for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++)if(redPixel(component(xx,yy,0),component(xx,yy,1),component(xx,yy,2)))reference[(yy/4)*gw+xx/4]++;
 RedRegion expected=largestRedRegion(reference.data(),queue.data(),w,h,100);assert(red.pixels==expected.pixels&&red.w==expected.w&&red.x==expected.x&&red.y==expected.y&&red.h==expected.h);
 for(int y=0;y<MODEL_SIDE;y++)for(int x=0;x<MODEL_SIDE;x++){int sx,sy;bool valid=viewPixel(v,x,y,sx,sy);for(int c=0;c<3;c++)assert(out[3*(y*MODEL_SIDE+x)+c]==(valid?component(sx,sy,2-c):0));}cases++;}
 printf("PASS: %d camera/crop cases; same-frame colour width/count verified; MCU sampling equals previous resize pixels, including padding, edge crops and repeated samples.\n",cases);
}
