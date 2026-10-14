#pragma once
#define WIFI_AP 1
struct WifiMock{void mode(int){}void setSleep(bool){}void softAP(const char*,const char*,int){}};extern WifiMock WiFi;
