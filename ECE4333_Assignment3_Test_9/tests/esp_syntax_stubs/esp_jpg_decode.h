#pragma once
#include <stddef.h>
#include <stdint.h>
typedef int jpg_scale_t;

int esp_jpg_decode(size_t,jpg_scale_t,size_t(*)(void*,size_t,uint8_t*,size_t),bool(*)(void*,uint16_t,uint16_t,uint16_t,uint16_t,uint8_t*),void*);
