/*
 * Protocol.h - our own ESP32 <-> Arduino serial protocol (ASCII, newline framed).
 *
 *   ESP32 -> Arduino
 *     $D,<cls>,<dist_mm>,<bearing_dd>*HH   detection result
 *            cls: 0 none | 1 red (colour only, not yet classified) | 2 green light
 *                 3 stop sign (AI confirmed) | 4 red traffic light (AI confirmed)
 *                 5 colour target (follow mode)
 *            dist_mm: ground distance camera->object, 0 = unknown
 *            bearing_dd: tenths of a degree, + = object to the LEFT of image centre
 *     $M,<0|1>*HH   select mode (0 figure-8, 1 follow)
 *     $G*HH         go (start without waiting for green)
 *     $R*HH         resume after a latched emergency stop
 *     $K,<id>*HH    checkpoint: log a pose record now (match it to a floor mark)
 *
 *   Assignment-2 ESP32 JSON frames are also understood (no ESP32 change needed):
 *     {"N":200,"S":..,"A":..,"V":..,"P":..,"R":<cm,-1 unknown>,"X":..,...}  stop-sign result
 *     {"N":203,"R":0|1}                                                     red-region heartbeat
 *     {"N":204,"G":0|1,"R":0|1,"D":cm,"SD":cm,"B":mrad,...}  colour + green (uses D/SD/B when present)
 *
 *   Arduino -> ESP32
 *     $S,<mode>,<state>,<pwm>*HH           status, 1 Hz and on every state change
 *
 *   HH = two hex digits, XOR of every character between '$' and '*'.
 *   The checksum is optional on frames you type by hand (PROTO_REQUIRE_CHECKSUM 0).
 *   Every other line the Arduino prints (CSV log lines) does not start with '$',
 *   so the ESP32 can simply ignore it.
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <Arduino.h>

enum { CLS_NONE = 0, CLS_RED = 1, CLS_GREEN = 2, CLS_STOP_SIGN = 3, CLS_RED_LIGHT = 4, CLS_TARGET = 5 };

struct Detection
{
  uint8_t cls;
  int16_t dist_mm;
  float bearing_deg;       // CCW (left) positive
  bool distKnown;          // false = dist_mm is only an assumed value
  unsigned long rx_ms;     // arrival time (0 = never)
  unsigned long seq;       // increments per frame
};

struct ProtoState
{
  Detection det;
  bool newDet;             // set per frame, consumed by the main loop
  int8_t cmdMode;          // -1 none, else 0/1
  bool cmdResume;
  bool cmdStart;
  int16_t cmdCheckpoint;   // -1 none
  unsigned long good, bad;
};
extern ProtoState proto;

void Proto_init(void);
void Proto_poll(void);
bool Proto_fresh(unsigned long now_ms);   // is proto.det recent enough to use?
void Proto_sendStatus(uint8_t mode, uint8_t state, int pwm);
#endif
