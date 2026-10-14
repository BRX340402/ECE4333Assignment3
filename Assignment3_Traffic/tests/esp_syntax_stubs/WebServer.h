#pragma once
#include "Arduino.h"
enum{HTTP_GET,HTTP_POST};struct Client{bool connected(){return true;}void stop(){}size_t write(const uint8_t*,size_t n){return n;}};
struct WebServer{WebServer(int){}template<class F>void on(const char*,int,F){}void begin(){}void handleClient(){}void send(int,const char*,String){}void send_P(int,const char*,const char*){}void sendHeader(const char*,String){}void setContentLength(size_t){}bool hasArg(const char*){return false;}String arg(const char*){return "";}Client client(){return {};}};
