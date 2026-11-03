#pragma once
#include <stdint.h>
struct CalibrationSample {
 int32_t width=0,imageWidth=0,cm=0,resolution=-1;
 bool valid()const{return width>=8&&width<imageWidth&&imageWidth<=400&&imageWidth>=16&&cm>=5&&cm<=300&&resolution>=0&&resolution<=10;}
 float k()const{return valid()?cm*width/(float)imageWidth:0;}
};
struct CalibrationStore {
 uint32_t magic=0xA3C50001UL;
 CalibrationSample stop,light,green;
};

struct LegacyCalibrationStore {uint32_t magic=0xA3C30001UL;CalibrationSample stop,light;};
