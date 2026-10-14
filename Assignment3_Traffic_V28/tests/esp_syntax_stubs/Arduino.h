#pragma once
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <algorithm>
#include <type_traits>
using std::min;using std::max;
#define PROGMEM
#define SERIAL_8N1 0
#define pdMS_TO_TICKS(x) (x)
#define portMAX_DELAY 0
#define pdTRUE 1
#define pdPASS 1
struct String{std::string s;String(){}String(const char*x):s(x?x:""){}String(const std::string&x):s(x){}String(char x):s(1,x){}template<class T,typename std::enable_if<std::is_arithmetic<T>::value,int>::type=0>String(T x):s(std::to_string(x)){}String(float x,int):s(std::to_string(x)){}String(double x,int):s(std::to_string(x)){}const char*c_str()const{return s.c_str();}int toInt()const{return atoi(s.c_str());}size_t length()const{return s.size();}void reserve(size_t n){s.reserve(n);}void remove(size_t n){s.erase(n);}bool startsWith(const char*x)const{return s.find(x)==0;}bool endsWith(const char*x)const{return !s.empty()&&s.back()==*x;}String&operator+=(String x){s+=x.s;return *this;}friend String operator+(String a,String b){return a.s+b.s;}bool operator==(String x)const{return s==x.s;}bool operator!=(String x)const{return s!=x.s;}char operator[](int i)const{return s[i];}};
struct SerialMock{template<class...T>void begin(T...){}template<class...T>void printf(T...){}template<class T>void print(T){}template<class T>void println(T){}void println(){}bool available(){return false;}char read(){return 0;}};extern SerialMock Serial,Serial2;
struct EspMock{void restart(){}int getFreeHeap(){return 0;}int getFreePsram(){return 0;}};extern EspMock ESP;
inline uint32_t millis(){return 0;}inline void delay(int){}inline void*ps_malloc(size_t n){return malloc(n);}inline bool psramFound(){return true;}
using SemaphoreHandle_t=int;using QueueHandle_t=int;inline int xSemaphoreCreateMutex(){return 1;}inline int xSemaphoreTake(int,int){return 1;}inline void xSemaphoreGive(int){}inline int xQueueCreate(int,int){return 1;}inline void xQueueOverwrite(int,const void*){}inline void xQueueReset(int){}inline int xQueueReceive(int,void*,int){return 0;}inline void vTaskDelay(int){}inline int xTaskCreatePinnedToCore(void(*)(void*),const char*,int,void*,int,void*,int){return 1;}
