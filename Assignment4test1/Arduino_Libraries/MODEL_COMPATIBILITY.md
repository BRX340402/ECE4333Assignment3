# 模型与SDK修补
模型为用户1.0.3导出，96×96 int8 YOLO-Pro，类别3。权重未改。
继承：portable DSP；旧GCC全局cmath；float sparse辅助函数O0规避GCC5 ICE。
FAST新增：generated CPP、conv.cpp、depthwise_conv.cpp顶端有A2_FAST_OPT标记的ESP32 O3 pragma；无fast-math。generated CPP的A2_COOPERATIVE调度检查间隔由20改成50ms，仍调用vTaskDelay(1)。
应用EIMemory.cpp可选预留内部arena并在不足时用PSRAM。详情见README。
