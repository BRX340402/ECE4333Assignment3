#pragma once
#include "ModelShape.h"
#include <cmath>
#include <cstring>
// Observational only: no filtering, cell merging, detection, or UNO decisions use this data.
struct RawDiagnostic {
 bool available;
 unsigned count;
 float score,x,y,w,h;
};
static RawDiagnostic rawDiagnostic={};
static void recordRawStop(const char *label,float score,float xmin,float ymin,float xmax,float ymax){
 if(!label || strcmp(label,"3") || !std::isfinite(score) || score<0 || score>1) return;
 rawDiagnostic.count++;
 if(rawDiagnostic.available && score<=rawDiagnostic.score)return;
 rawDiagnostic.available=true;rawDiagnostic.score=score;
 rawDiagnostic.x=xmin*MODEL_SIDE;rawDiagnostic.y=ymin*MODEL_SIDE;
 rawDiagnostic.w=(xmax-xmin)*MODEL_SIDE;rawDiagnostic.h=(ymax-ymin)*MODEL_SIDE;
}
#define A2_FOMO_RAW_OBSERVER(label,score,xmin,ymin,xmax,ymax) recordRawStop(label,score,xmin,ymin,xmax,ymax)
