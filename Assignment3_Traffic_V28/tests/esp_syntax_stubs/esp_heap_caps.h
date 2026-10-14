#pragma once
#include "Arduino.h"
enum{MALLOC_CAP_INTERNAL=1,MALLOC_CAP_8BIT=2,MALLOC_CAP_SPIRAM=4};inline size_t heap_caps_get_largest_free_block(int){return 0;}inline size_t heap_caps_get_free_size(int){return 0;}inline void*heap_caps_malloc(size_t n,int){return malloc(n);}inline void heap_caps_free(void*p){free(p);}
