#pragma once
#include <stdint.h>
#include <stddef.h>
// Read encoded SOF dimensions rather than trusting sensor status during transitions.
static bool jpegSize(const uint8_t *p,size_t n,int &w,int &h){
 if(!p||n<4||p[0]!=0xff||p[1]!=0xd8)return false;
 size_t i=2;
 while(i<n){
  if(p[i++]!=0xff)return false;
  while(i<n&&p[i]==0xff)i++;
  if(i>=n)return false;uint8_t marker=p[i++];
  if(marker==0xd9||marker==0xda)return false;
  if(marker==0x01||(marker>=0xd0&&marker<=0xd7))continue;
  if(n-i<2)return false;size_t len=((size_t)p[i]<<8)|p[i+1];
  if(len<2||len>n-i)return false;
  if(marker==0xc0||marker==0xc1||marker==0xc2){
   if(len<8)return false;h=((int)p[i+3]<<8)|p[i+4];w=((int)p[i+5]<<8)|p[i+6];return w>0&&h>0;
  }
  i+=len;
 }
 return false;
}
static bool resolutionSize(int id,int &w,int &h){
 static const int widths[]={160,128,176,240,320,400,640,800,1024,1280,1600};
 static const int heights[]={120,160,144,176,240,296,480,600,768,1024,1200};
 if(id<0||id>10)return false;w=widths[id];h=heights[id];return true;
}
