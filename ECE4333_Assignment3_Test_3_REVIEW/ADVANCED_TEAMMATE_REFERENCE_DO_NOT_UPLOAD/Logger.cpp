#include "Logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOG_BUF 256
static char qbuf[LOG_BUF];
static volatile int qhead = 0, qtail = 0, qcount = 0;
unsigned int log_dropped = 0;

void Log_init(void) { qhead = qtail = qcount = 0; log_dropped = 0; }

void Log_write(const char *s, int n)
{
  if (n <= 0) return;
  if (qcount + n > LOG_BUF) { log_dropped++; return; }
  for (int i = 0; i < n; i++)
  {
    qbuf[qhead] = s[i];
    qhead = (qhead + 1) % LOG_BUF;
  }
  qcount += n;
}

void Log_printf_P(const char *fmt_P, ...)
{
  char tmp[128];
  va_list ap;
  va_start(ap, fmt_P);
  int n = vsnprintf_P(tmp, sizeof(tmp), fmt_P, ap);
  va_end(ap);
  if (n <= 0) return;
  if (n >= (int)sizeof(tmp)) n = sizeof(tmp) - 1;
  Log_write(tmp, n);
}

void Log_pump(void)
{
  int avail = Serial.availableForWrite();
  while (avail > 0 && qcount > 0)
  {
    int chunk = LOG_BUF - qtail;       // contiguous bytes
    if (chunk > qcount) chunk = qcount;
    if (chunk > avail) chunk = avail;
    Serial.write((const uint8_t *)&qbuf[qtail], chunk);
    qtail = (qtail + chunk) % LOG_BUF;
    qcount -= chunk;
    avail -= chunk;
  }
}
