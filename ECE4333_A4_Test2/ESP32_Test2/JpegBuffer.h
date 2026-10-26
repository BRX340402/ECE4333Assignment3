#pragma once
#include <Arduino.h>

// Each buffer has one owner. Grow only when needed; allocation failure keeps
// the old buffer intact. Shared-cache access is protected by stateMutex.
static bool reserveJpeg(uint8_t *&data,size_t &capacity,size_t required){
 if(!required)return false;
 if(data&&capacity>=required)return true;
 if(required>SIZE_MAX-4095)return false;
 size_t next=(required+4095)&~(size_t)4095;
 uint8_t *replacement=(uint8_t*)ps_malloc(next);
 if(!replacement)return false;
 free(data);data=replacement;capacity=next;return true;
}
