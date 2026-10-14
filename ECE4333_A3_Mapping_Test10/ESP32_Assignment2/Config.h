#pragma once
#include <stdint.h>
#define A2_ENABLE_AI 1
#define A2_MODEL_HEADER "A2_StopSign_FOMO_104.h"
#define COLOR_INPUT_BGR 1 // legacy decoder order; verify with a red/blue object
#define CAMERA_MODEL_M5STACK_WIDE
static const char *AP_SSID = "ECE4333_A2_V2";
static const char *AP_PASSWORD = "12345678";
static const int AP_CHANNEL = 9;
static const int UART_RX = 33, UART_TX = 4;
// Test10 mapping build: must match LINK_BAUD in UNO_Mapping/MapConfig.h
static const uint32_t UNO_LINK_BAUD = 19200;
static const int MAP_UNO_VERSION = 40;        // UV reported by UNO_Mapping
static const uint32_t MAP_LOG_LINES = 2000;   // every UNO line is kept here (PSRAM ring, ~3 min)
static const uint16_t MAP_LOG_LINE_BYTES = 360; // > longest UNO line (352)
static const float AI_THRESHOLD = 0.70f;
static const uint32_t FRAME_INTERVAL_MS = 10;
// Legacy constants retained for source compatibility. FullUI3 calibration is
// managed by /calibrate and NVS a2-cam-v2; do not edit these to calibrate.
static const float STOP_K = 0.0f;
// 0: multiline Serial Monitor; 1: numeric Serial Plotter; 2: JSON lines.
#define A2_SERIAL_MODE 1

// Only red/green colour analysis and the 96x96 STOP model are included.
