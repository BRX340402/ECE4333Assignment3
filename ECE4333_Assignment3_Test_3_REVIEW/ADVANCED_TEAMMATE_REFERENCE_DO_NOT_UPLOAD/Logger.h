/* Logger.h - non-blocking serial output queue.
 * Lines are queued in a RAM ring buffer and drip-fed to the UART from loop(),
 * so printing never stalls the 100 Hz control loop. */
#ifndef LOGGER_H
#define LOGGER_H
#include <Arduino.h>
void Log_init(void);
void Log_write(const char *s, int n);           // queue raw bytes (dropped if full)
void Log_printf_P(const char *fmt_P, ...);      // queue a printf-style line (format in flash)
void Log_pump(void);                            // call every loop()
extern unsigned int log_dropped;
#endif
