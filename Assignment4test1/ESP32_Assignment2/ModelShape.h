#pragma once
#include <stddef.h>
static const int MODEL_SIDE=96;
static const size_t MODEL_PIXELS=MODEL_SIDE*MODEL_SIDE;
static const size_t MODEL_RGB_BYTES=MODEL_PIXELS*3;
static_assert((MODEL_SIDE*3)%4==0,"BMP rows must be aligned");
