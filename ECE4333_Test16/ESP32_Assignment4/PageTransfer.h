#pragma once
#include <stddef.h>
#include <stdint.h>
// ESP32 flash is memory mapped. No full-page RAM copy and no JPEG re-encoding.
template<class Client,class Clock,class Yield>
size_t writePageBody(Client &client,const uint8_t *data,size_t length,Clock clock,Yield yield){
 size_t sent=0;uint32_t started=clock();
 while(sent<length&&client.connected()&&uint32_t(clock()-started)<8000){
  size_t chunk=length-sent;if(chunk>1024)chunk=1024;
  size_t n=client.write(data+sent,chunk);
  if(n>chunk)break;
  sent+=n;yield(); // Retry partial/zero writes while connected, until deadline.
 }
 if(sent!=length)client.stop();
 return sent;
}
