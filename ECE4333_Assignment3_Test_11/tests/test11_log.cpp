#include <cassert>
#include "esp_syntax_stubs/Arduino.h"
#include "esp_syntax_stubs/esp_heap_caps.h"
#include "../ESP32_Assignment2/SurveyLog.h"
int main(){SurveyLog l;l.begin();assert(l.capacity==1024*1024);l.add("uno","{}");assert(l.used==0);l.active=true;l.add("uno","{\"N\":210}");std::string first(l.data,l.used);assert(first=="{\"esp_ms\":0,\"source\":\"uno\",\"data\":{\"N\":210}}\n");l.capacity=l.used+1;l.add("uno","{}");assert(l.dropped==1&&std::string(l.data,l.used)==first);l.clear();assert(!l.active&&!l.used&&!l.dropped);free(l.data);puts("PASS log exact JSON records, inactive gate, overflow preserves earlier data, dropped count and explicit reset");}
