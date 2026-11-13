#pragma once
#include <stdint.h>
#include <stdarg.h>
#include <avr/pgmspace.h>
// Tiny replacement for snprintf_P. Linking avr-libc's vfprintf costs about 1.3-1.5 KB of
// flash; the four serial messages only need %d %u %ld %lu and literal text, which this does
// in roughly 200 bytes. Same contract as snprintf: returns the length the whole text needs
// (so the caller can detect truncation) and always NUL-terminates inside cap.
// Format string lives in flash (PSTR). Supported: %d %u %ld %lu %%.
struct LinkOut{
 char *p;int cap,n;
 void put(char c){if(n<cap-1)p[n]=c;n++;}
 void num(uint32_t v,bool neg){
  char b[11];uint8_t i=0;
  do{b[i++]='0'+(uint8_t)(v%10);v/=10;}while(v);
  if(neg)put('-');
  while(i)put(b[--i]);
 }
};
static int linkFmt(char *out,int cap,const char *fmtP,...){
 LinkOut o={out,cap,0};
 va_list ap;va_start(ap,fmtP);
 for(char c;(c=pgm_read_byte(fmtP++))!=0;){
  if(c!='%'){o.put(c);continue;}
  c=pgm_read_byte(fmtP++);
  bool lng=false;
  if(c=='l'){lng=true;c=pgm_read_byte(fmtP++);}
  if(c=='%'){o.put('%');continue;}
  if(c=='d'){
   int32_t v=lng?(int32_t)va_arg(ap,long):(int32_t)va_arg(ap,int);
   o.num(v<0?0u-(uint32_t)v:(uint32_t)v,v<0);
  }else{
   uint32_t v=lng?(uint32_t)va_arg(ap,unsigned long):(uint32_t)va_arg(ap,unsigned);
   o.num(v,false);
  }
 }
 va_end(ap);
 if(cap>0)out[o.n<cap?o.n:cap-1]=0;
 return o.n;
}
