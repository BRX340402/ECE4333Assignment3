#pragma once
#include <Arduino.h>
#include "Hardware.h"
// Test16 (Assignment 5 task 1): shield KEY on an interrupt, ELEGOO IR remote (NEC, pin-change interrupt), battery LED.
//  KEY (pin 2, INT0, falling edge): starts the state picked with the remote, or stops the car if it is running.
//  IR  (pin 9, PCINT1): NEC frames decoded in the interrupt; the command byte is handed to the sketch.
//      ELEGOO remote: 1 = figure-8 line following, 2 = cone / target following, OK = start, * or # = stop.
//  LED (WS2812 on pin 4): battery health from the A3 divider: green >= 7.6 V, yellow >= 7.0 V, red below (2S Li-ion).
volatile bool keyHit=false;volatile uint32_t keyAt=0;
void keyIsr(){uint32_t t=millis();if(t-keyAt>250){keyHit=true;keyAt=t;}}
volatile uint8_t irCmd=0;volatile bool irNew=false;
static volatile uint32_t irT=0,irBits=0;static volatile uint8_t irCount=0xFF;
ISR(PCINT0_vect){                       // pin 9 = PB1: a NEC mark ends on the rising edge (receiver output is active low);
                                        // each bit = time between two rising edges (space + next mark): 1.125 ms = 0, 2.25 ms = 1
 if(!(PINB&_BV(1)))return;              // only rising edges: measure the full bit period (mark + space)
 uint32_t now=micros(),d=now-irT;irT=now;
 if(d>4700&&d<5500){irCount=0;irBits=0;return;}   // 4.5 ms leader space + first 0.56 ms mark (rising edge to rising edge)
 if(irCount>=32)return;
 if(d>900&&d<1400){irCount++;}                     // 0: 1.125 ms
 else if(d>2000&&d<2600){irBits|=(uint32_t)1<<irCount;irCount++;} // 1: 2.25 ms
 else{irCount=0xFF;return;}
 if(irCount==32){uint8_t c=irBits>>16,nc=irBits>>24;if((uint8_t)(c^nc)==0xFF){irCmd=c;irNew=true;}}
}
void keyIrBegin(){
 pinMode(KEY_PIN,INPUT_PULLUP);attachInterrupt(digitalPinToInterrupt(KEY_PIN),keyIsr,FALLING);
 pinMode(IR_PIN,INPUT);PCMSK0|=_BV(PCINT1);PCICR|=_BV(PCIE0);
 pinMode(RGB_PIN,OUTPUT);
}
// one WS2812 pixel, 16 MHz: 20 cycles per bit, T0H 5 cycles (0.31 us), T1H 13 cycles (0.81 us)
void ledSend(uint8_t r,uint8_t g,uint8_t b){
 uint8_t v[3]={g,r,b},hi=PORTD|_BV(4),lo=PORTD&~_BV(4);uint8_t s=SREG;cli();
 for(uint8_t i=0;i<3;i++){uint8_t x=v[i],n=8;
  asm volatile("1: out %[p],%[hi]\n nop\n nop\n nop\n sbrs %[x],7\n out %[p],%[lo]\n lsl %[x]\n nop\n nop\n nop\n nop\n nop\n nop\n out %[p],%[lo]\n nop\n nop\n nop\n dec %[n]\n brne 1b\n"
   :[x]"+r"(x),[n]"+r"(n):[p]"I"(_SFR_IO_ADDR(PORTD)),[hi]"r"(hi),[lo]"r"(lo));}
 SREG=s;
}
uint8_t batteryTenths(){return (uint8_t)(analogRead(BATTERY_PIN)*BATTERY_SCALE*1.08f*10);} // ELEGOO divider + 8 % correction
void batteryLed(uint8_t v10){if(v10>=76)ledSend(0,24,0);else if(v10>=70)ledSend(24,14,0);else ledSend(30,0,0);}
