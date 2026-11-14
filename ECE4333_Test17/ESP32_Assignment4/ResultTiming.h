#pragma once
#include <stdint.h>
// LED indicates the latest completed classification, not motor-data freshness.
inline bool stopIndicatorReady(bool observed,uint32_t completed,uint32_t now,bool enabled,bool busy){
 return observed && enabled && !busy && (uint32_t)(now-completed)<5000;
}
