#pragma once
// Main-loop owner only; no capture-task access. Preserves earliest records on overflow.
struct SurveyLog {
 char *data=nullptr;size_t capacity=0,used=0;uint32_t dropped=0;bool active=false;
 void begin(){if(data)return;capacity=1024*1024;data=(char*)heap_caps_malloc(capacity,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);if(!data)capacity=0;}
 void clear(){used=0;dropped=0;active=false;}
 void add(const char *source,const String &payload){
  if(!active)return;
  char head[80];int n=snprintf(head,sizeof(head),"{\"esp_ms\":%lu,\"source\":\"%s\",\"data\":",(unsigned long)millis(),source);
  size_t bytes=n+payload.length()+2;
  if(!data||bytes>capacity-used){dropped++;return;}
  memcpy(data+used,head,n);used+=n;memcpy(data+used,payload.c_str(),payload.length());used+=payload.length();data[used++]='}';data[used++]='\n';
 }
 String status()const{char b[150];snprintf(b,sizeof(b),"{\"bytes\":%u,\"capacity\":%u,\"dropped\":%lu,\"active\":%u}",(unsigned)used,(unsigned)capacity,(unsigned long)dropped,(unsigned)active);return String(b);}
};
