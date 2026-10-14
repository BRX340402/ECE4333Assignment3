#pragma once
#include "ModelShape.h"
#include "Config.h"
#include "color_blob.h"
#include "VisionGeometry.h"
#include <string.h>
#include "RawDiagnostic.h"
#if A2_ENABLE_AI
#define EIDSP_USE_ESP_DSP 0
#include A2_MODEL_HEADER
static_assert(EI_CLASSIFIER_OBJECT_DETECTION==1 && EI_HAS_FOMO==1,"FOMO required");
static_assert(EI_CLASSIFIER_PROJECT_DEPLOY_VERSION==1,"Install bundled A2_StopSign_FOMO_104 v1.0.1 library; old model library detected");
static_assert(EI_CLASSIFIER_INPUT_WIDTH==MODEL_SIDE && EI_CLASSIFIER_INPUT_HEIGHT==MODEL_SIDE,"96x96 model required");
#endif
extern int analysisW,analysisH;
extern bool inputBGR;
extern ViewMap aiView;
extern float aiThreshold;
static const char *className(int id){switch(id){case 1:return "stop";case 6:return "red";case 7:return "blue";case 8:return "green";case 9:return "orange";default:return "other";}}
static uint32_t packedPixel(const uint8_t *p){return ((uint32_t)p[inputBGR?2:0]<<16)|((uint32_t)p[1]<<8)|p[inputBGR?0:2];}
static const int MAX_STOPS=4;
struct StopPrediction{ColorBlob box[MAX_STOPS];float confidence[MAX_STOPS];bool clipped[MAX_STOPS];int count;float best;uint32_t duration,dspMs,nnMs;int error;};
static const uint8_t *aiPixels;
static int signalData(size_t offset,size_t length,float *out){if(offset>MODEL_PIXELS||length>MODEL_PIXELS-offset)return -1;for(size_t i=0;i<length;i++)out[i]=(float)packedPixel(aiPixels+3*(offset+i));return 0;}
static bool inferFrame(const uint8_t *pixels,StopPrediction &out){
 out={};aiPixels=pixels;
#if A2_ENABLE_AI
 ei::signal_t sig;sig.total_length=MODEL_PIXELS;sig.get_data=signalData;ei_impulse_result_t result={};uint32_t start=millis();
 auto err=run_classifier(&sig,&result,false);out.duration=millis()-start;out.dspMs=result.timing.dsp;out.nnMs=result.timing.classification;out.error=(int)err;if(err!=EI_IMPULSE_OK)return false;
 for(uint32_t i=0;i<result.bounding_boxes_count;i++){
  const auto &b=result.bounding_boxes[i];if(!b.label||strcmp(b.label,"3")||b.value<=0)continue;
  if(b.value>out.best)out.best=b.value;
  if(b.value<aiThreshold)continue;
  MappedBox mapped;if(!mapModelBox(aiView,b.x,b.y,b.width,b.height,mapped))continue;
  int slot=out.count;if(slot>=MAX_STOPS){slot=0;for(int j=1;j<MAX_STOPS;j++)if(out.confidence[j]<out.confidence[slot])slot=j;if(b.value<=out.confidence[slot])continue;}else out.count++;
  out.box[slot]={(mapped.x0+mapped.x1)/2,(mapped.y0+mapped.y1)/2,mapped.x0,mapped.y0,mapped.x1,mapped.y1,(uint32_t)((mapped.x1-mapped.x0+1)*(mapped.y1-mapped.y0+1))};
  out.clipped[slot]=mapped.clipped;out.confidence[slot]=b.value;
 }
#endif
 return true;
}
