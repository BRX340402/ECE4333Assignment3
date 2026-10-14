#pragma once
#include <stdint.h>
// Cover the measured 5.8 s inference cycle; expire if classification stops.
static const uint32_t STOP_LED_HOLD_MS=10000;
struct IndicatorState {
 bool red=false,stop=false;uint32_t redAt=0,stopAt=0,blinkAt=0;
 void setRed(bool value,uint32_t now){if(value&&!red)blinkAt=now;red=value;redAt=now;}
 void setStop(bool value,uint32_t now){stop=value;stopAt=now;}
 uint8_t mode(uint32_t now)const{
  if(stop&&(uint32_t)(now-stopAt)<STOP_LED_HOLD_MS)return 2;
  if(red&&(uint32_t)(now-redAt)<1200)return 1;
  return 0;
 }
 bool on(uint32_t now)const{uint8_t m=mode(now);return m==2||(m==1&&((uint32_t)(now-blinkAt)/250)%2==0);}
};
