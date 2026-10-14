#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// Hue +/-20 degrees, saturation >=45%, value >=60; decoder supplies RGB.
static bool redPixel(uint8_t r,uint8_t g,uint8_t b){
 int low=g<b?g:b,delta=(int)r-low;
 return r>=60&&r>=g&&r>=b&&delta*100>=45*(int)r&&abs((int)g-b)*3<=delta;
}
// Green: hue roughly 70..170 degrees, saturation >=15%, value >=24.
// Require channel separation to reject neutral grey and near-black noise.
static bool greenPixel(uint8_t r,uint8_t g,uint8_t b){
 int low=r<b?r:b,delta=(int)g-low;
 return g>=24&&g>=r+5&&g>=b+5&&delta*100>=15*(int)g&&abs((int)b-r)*6<=5*delta;
}
struct RedRegion {int components=0,pixels=0,x=0,y=0,w=0,h=0,imageW=0,imageH=0;};
// 4x4 cells preserve counts from the decoded image; isolated one-pixel noise is ignored.
static RedRegion largestRedRegion(uint8_t *cells,uint16_t *queue,int width,int height,int minimum=100){
 int components=0;RedRegion best;best.imageW=width;best.imageH=height;int gw=(width+3)/4,gh=(height+3)/4;
 for(int start=0;start<gw*gh;start++){
  if(cells[start]<2)continue;
  int head=0,tail=0,sum=cells[start],xmin=start%gw,xmax=xmin,ymin=start/gw,ymax=ymin;
  cells[start]=0;queue[tail++]=start;
  while(head<tail){int v=queue[head++],x=v%gw,y=v/gw;
   for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
    int nx=x+dx,ny=y+dy;if(nx<0||ny<0||nx>=gw||ny>=gh)continue;
    int n=ny*gw+nx;if(cells[n]<2)continue;
    sum+=cells[n];cells[n]=0;queue[tail++]=n;
    if(nx<xmin)xmin=nx;if(nx>xmax)xmax=nx;if(ny<ymin)ymin=ny;if(ny>ymax)ymax=ny;
   }
  }
  if(sum>=minimum)components++;
  if(sum>best.pixels){best.pixels=sum;best.x=xmin*4;best.y=ymin*4;best.w=((xmax+1)*4<width?(xmax+1)*4:width)-best.x;best.h=((ymax+1)*4<height?(ymax+1)*4:height)-best.y;}
 }
 best.components=components;return best;
}
