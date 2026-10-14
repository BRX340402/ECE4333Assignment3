#include "Protocol.h"
#include "Config.h"
#include "Logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ProtoState proto;

#define PROTO_BUF 112
static char buf[PROTO_BUF];
static uint8_t n = 0;
static bool inFrame = false;
static char frameType = 0;      // '$' own protocol, '{' A2 JSON

void Proto_init(void)
{
  memset(&proto, 0, sizeof(proto));
  proto.cmdMode = -1;
  proto.cmdCheckpoint = -1;
}

bool Proto_fresh(unsigned long now_ms)
{
  return proto.det.rx_ms != 0 && (now_ms - proto.det.rx_ms) < (unsigned long)DET_STALE_MS;
}

static void handleFrame(void)
{
  // optional checksum
  char *star = strchr(buf, '*');
  if (star && star[1] == 0 && !PROTO_REQUIRE_CHECKSUM)
  {
    *star = 0;                       // "$G*" typed by hand with no digits: accept
  }
  else if (star)
  {
    uint8_t cs = 0;
    for (char *p = buf; p < star; p++) cs ^= (uint8_t)*p;
    long rx = strtol(star + 1, NULL, 16);
    if ((uint8_t)rx != cs) { proto.bad++; return; }
    *star = 0;
  }
  else if (PROTO_REQUIRE_CHECKSUM)
  {
    proto.bad++;
    return;
  }

  char *tag = strtok(buf, ",");
  if (!tag) { proto.bad++; return; }
  switch (tag[0])
  {
  case 'D':
  {
    char *a = strtok(NULL, ",");
    char *b = strtok(NULL, ",");
    char *c = strtok(NULL, ",");
    if (!a) { proto.bad++; return; }
    proto.det.cls = (uint8_t)atoi(a);
    proto.det.dist_mm = b ? (int16_t)atoi(b) : 0;
    proto.det.bearing_deg = c ? (float)atoi(c) / 10.0f : 0.0f;
    proto.det.distKnown = proto.det.dist_mm > 0;
    proto.det.rx_ms = millis();
    if (proto.det.rx_ms == 0) proto.det.rx_ms = 1;
    proto.det.seq++;
    proto.newDet = true;
    break;
  }
  case 'M':
  {
    char *a = strtok(NULL, ",");
    if (a) proto.cmdMode = (int8_t)atoi(a);
    break;
  }
  case 'G': proto.cmdStart = true; break;
  case 'R': proto.cmdResume = true; break;
  case 'K':
  {
    char *a = strtok(NULL, ",");
    proto.cmdCheckpoint = a ? (int16_t)atoi(a) : 0;
    break;
  }
  default: proto.bad++; return;
  }
  proto.good++;
}


// ---- Assignment-2 JSON adapter (hand parsed: no ArduinoJson needed) ----
static bool jfind(const char *s, const char *key, long &out)
{
  char pat[8];
  snprintf(pat, sizeof(pat), "\"%s\":", key);
  const char *p = strstr(s, pat);
  if (!p) return false;
  out = atol(p + strlen(pat));
  return true;
}

static void pushDet(uint8_t cls, int dist_mm, float bearing, bool known)
{
  proto.det.cls = cls;
  proto.det.dist_mm = (int16_t)dist_mm;
  proto.det.bearing_deg = bearing;
  proto.det.distKnown = known;
  proto.det.rx_ms = millis();
  if (proto.det.rx_ms == 0) proto.det.rx_ms = 1;
  proto.det.seq++;
  proto.newDet = true;
}

static void handleJson(void)
{
  long n, a, v, p, r, x;
  if (!jfind(buf, "N", n)) { proto.bad++; return; }
  if (n == 200)
  {
    if (!jfind(buf, "A", a) || !jfind(buf, "V", v) || !jfind(buf, "P", p) || !jfind(buf, "R", r) || !jfind(buf, "X", x))
    { proto.bad++; return; }
    if (a && v && p >= STOP_MIN_P)
    {
      float bearing = (x >= 0) ? ((float)CAM_IMAGE_W_PX * 0.5f - (float)x) / (float)CAM_IMAGE_W_PX * CAM_HFOV_DEG : 0.0f;
      bool known = r >= 0;
      pushDet(CLS_STOP_SIGN, known ? (int)(r * 10) : ASSUMED_RED_DIST_MM, bearing, known);
    }
    else pushDet(CLS_NONE, 0, 0.0f, false);
    proto.good++;
  }
  else if (n == 203)
  {
    if (jfind(buf, "R", r) && r == 1) pushDet(CLS_RED, ASSUMED_RED_DIST_MM, 0.0f, false);
    proto.good++;
  }
  else if (n == 204)
  {
    /* Colour frame from current team ESP32 (G/R/D/SD/B). */
    long g = 0, red = 0, d = -1, sd = -1, b = 0, vv = 0;
    jfind(buf, "G", g);
    jfind(buf, "R", red);
    jfind(buf, "D", d);
    jfind(buf, "SD", sd);
    jfind(buf, "B", b);
    jfind(buf, "V", vv);
    float bearing_deg = (float)b * 0.05729578f; /* mrad -> deg */
    if (g == 1)
    {
      bool known = (d > 0);
      pushDet(CLS_GREEN, known ? (int)(d * 10) : 0, bearing_deg, known);
    }
    else if (red == 1 && vv == 1)
    {
      bool known = (sd > 0);
      pushDet(CLS_RED, known ? (int)(sd * 10) : ASSUMED_RED_DIST_MM, bearing_deg, known);
    }
    proto.good++;
  }
  // every other N (100, 101, 21x ...) belongs to the old A2 firmware: ignore silently
}

void Proto_poll(void)
{
  uint8_t budget = 64;                      // bound the time spent per call
  while (Serial.available() && budget--)
  {
    char c = (char)Serial.read();
    if (c == '$' || c == '{') { n = 0; inFrame = true; frameType = c; continue; }
    if (!inFrame) continue;
    bool end = (frameType == '{') ? (c == '}') : (c == '\n' || c == '\r');
    if (end)
    {
      if (n > 0)
      {
        buf[n] = 0;
        if (frameType == '{') handleJson(); else handleFrame();
      }
      inFrame = false;
      n = 0;
      continue;
    }
    if (n < PROTO_BUF - 1) buf[n++] = c;
    else { inFrame = false; n = 0; proto.bad++; }
  }
}

void Proto_sendStatus(uint8_t mode, uint8_t state, int pwm)
{
#if !SEND_STATUS_TO_ESP
  (void)mode; (void)state; (void)pwm;
  return;
#endif
  char body[24];
  int len = snprintf_P(body, sizeof(body), PSTR("S,%u,%u,%d"), mode, state, pwm);
  if (len <= 0) return;
  uint8_t cs = 0;
  for (int i = 0; i < len; i++) cs ^= (uint8_t)body[i];
  Log_printf_P(PSTR("$%s*%02X\n"), body, cs);
}
