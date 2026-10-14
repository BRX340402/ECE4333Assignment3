#pragma once
#include <stddef.h>
#define A2_FOMO_RAW_DIAGNOSTIC_API 1
#define EI_CLASSIFIER_OBJECT_DETECTION 1
#define EI_HAS_FOMO 1
#define EI_CLASSIFIER_PROJECT_DEPLOY_VERSION 1
#define EI_CLASSIFIER_INPUT_WIDTH 96
#define EI_CLASSIFIER_INPUT_HEIGHT 96
#define EI_IMPULSE_OK 0
namespace ei { struct signal_t {size_t total_length;int(*get_data)(size_t,size_t,float*);}; }
struct Box {const char* label;float value;int x,y,width,height;};
struct ei_impulse_result_t {struct{uint32_t dsp,classification;}timing;Box*bounding_boxes;uint32_t bounding_boxes_count;};
extern Box fixture[3];extern int classifierError;
inline int run_classifier(ei::signal_t*s,ei_impulse_result_t*r,bool){if(s->total_length!=9216)return 99;r->bounding_boxes=fixture;r->bounding_boxes_count=3;r->timing={2,100};return classifierError;}

const char*ei_classifier_inferencing_categories[]={"3"};
