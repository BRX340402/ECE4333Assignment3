#pragma once
#include "Arduino.h"
struct Preferences{bool begin(const char*,bool){return true;}size_t getBytesLength(const char*){return 0;}size_t getBytes(const char*,void*,size_t n){return n;}size_t putBytes(const char*,const void*,size_t n){return n;}int getInt(const char*,int n){return n;}float getFloat(const char*,float f){return f;}size_t putInt(const char*,int){return 4;}size_t putFloat(const char*,float){return 4;}};
