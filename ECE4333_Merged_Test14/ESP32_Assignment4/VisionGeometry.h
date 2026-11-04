#pragma once
#include "ModelShape.h"
#include <stdint.h>
struct ViewMap { int x,y,w,h; int padX,padY,outW,outH; };
inline int boundi(int v,int lo,int hi){return v<lo?lo:v>hi?hi:v;}
// mode 0: full frame letterbox; 1: original centre square; 2/3: movable 2x/4x square.
inline ViewMap makeView(int w,int h,int mode,int cxPermille,int cyPermille){
 ViewMap v={0,0,w,h,0,0,MODEL_SIDE,MODEL_SIDE};
 if(mode){int side=w<h?w:h;if(mode==2)side/=2;if(mode==3)side/=4;
  v.w=v.h=side;v.x=boundi(w*cxPermille/1000-side/2,0,w-side);v.y=boundi(h*cyPermille/1000-side/2,0,h-side);
 }else if(w>=h){v.outH=(MODEL_SIDE*h+w/2)/w;v.padY=(MODEL_SIDE-v.outH)/2;}
 else {v.outW=(MODEL_SIDE*w+h/2)/h;v.padX=(MODEL_SIDE-v.outW)/2;}
 return v;
}
inline bool viewPixel(const ViewMap &v,int mx,int my,int &x,int &y){
 if(mx<v.padX||my<v.padY||mx>=v.padX+v.outW||my>=v.padY+v.outH)return false;
 x=v.x+((2*(mx-v.padX)+1)*v.w)/(2*v.outW);y=v.y+((2*(my-v.padY)+1)*v.h)/(2*v.outH);return true;
}

// Model boxes use exclusive right/bottom edges, unlike ColorBlob's inclusive bounds.
struct MappedBox {int x0,y0,x1,y1;bool clipped;};
inline bool mapModelBox(const ViewMap &v,int mx,int my,int mw,int mh,MappedBox &b){
 if(mw<=0||mh<=0)return false;
 int l=boundi(mx,v.padX,v.padX+v.outW),t=boundi(my,v.padY,v.padY+v.outH);
 int r=boundi(mx+mw,v.padX,v.padX+v.outW),d=boundi(my+mh,v.padY,v.padY+v.outH);
 if(r<=l||d<=t)return false;
 b.x0=v.x+(l-v.padX)*v.w/v.outW;b.y0=v.y+(t-v.padY)*v.h/v.outH;
 b.x1=v.x+((r-v.padX)*v.w+v.outW-1)/v.outW-1;
 b.y1=v.y+((d-v.padY)*v.h+v.outH-1)/v.outH-1;
 b.clipped=mx<=v.padX||my<=v.padY||mx+mw>=v.padX+v.outW||my+mh>=v.padY+v.outH;
 return true;
}
