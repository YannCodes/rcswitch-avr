/*
  rcswitch-avr - avr-c port of the RCSwitch library (https://github.com/sui77/rc-switch)
  Copyright (c) 2017 Yann Lochet
  This file incorporates work covered by the following copyright and permission notice:

  RCSwitch - Arduino library for remote control outlet switches
  Copyright (c) 2011 Suat Özgür.  All right reserved.

  Contributors:
  - Andre Koehler / info(at)tomate-online(dot)de
  - Gordeev Andrey Vladimirovich / gordeev(at)openpyro(dot)com
  - Skineffect / http://forum.ardumote.com/viewtopic.php?f=2&t=46
  - Dominik Fischer / dom_fischer(at)web(dot)de
  - Frank Oltmanns / <first name>.<last name>(at)gmail(dot)com
  - Max Horn / max(at)quendi(dot)de
  - Robert ter Vehn / <first name>.<last name>(at)gmail(dot)com
  
  Project home: https://github.com/sui77/rc-switch/

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _RCSwitch_h
#define _RCSwitch_h

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

// Number of maximum high/low changes per packet.
// We can handle up to (uint16_t) => 32 bit * 2 H/L changes per bit + 2 for sync
#define RCSWITCH_MAX_CHANGES 67

void sendTriState(const char* sCodeWord);
void sendBinary(const char* sCodeWord);
void send(uint16_t code, uint16_t length);
void delay_us(uint16_t us);

//Receiving
void enableReceive();
bool available();
void resetAvailable();

void enableTransmit(volatile uint8_t *port, uint8_t pin);
void disableTransmit();

/**
 * Description of a single pulse, which consists of a high signal
 * whose duration is "high" times the base pulse length, followed
 * by a low signal lasting "low" times the base pulse length.
 * Thus, the pulse overall lasts (high+low)*pulseLength
 */
struct HighLow
{
    uint16_t high;
    uint16_t low;
};

typedef struct HighLow HighLow;
   
/**
 * A "protocol" describes how zero and one bits are encoded into high/low
 * pulses.
 */
struct Protocol
{
    /** base pulse length in microseconds, e.g. 350 */
    uint16_t pulseLength;

    HighLow syncFactor;
    HighLow zero;
    HighLow one;

    /**
     * If true, interchange high and low logic levels in all transmissions.
     *
     * By default, RCSwitch assumes that any signals it sends or receives
     * can be broken down into pulses which start with a high signal level,
     * followed by a a low signal level. This is e.g. the case for the
     * popular PT 2260 encoder chip, and thus many switches out there.
     *
     * But some devices do it the other way around, and start with a low
     * signal level, followed by a high signal level, e.g. the HT6P20B. To
     * accommodate this, one can set invertedSignal to true, which causes
     * RCSwitch to change how it interprets any HighLow struct FOO: It will
     * then assume transmissions start with a low signal lasting
     * FOO.high*pulseLength microseconds, followed by a high signal lasting
     * FOO.low*pulseLength microseconds.
     */
    bool invertedSignal;
};
    
typedef struct Protocol Protocol;

void setProtocol(uint16_t nProtocol);

void transmit(HighLow pulses);

//removed static for testing
bool receiveProtocol(const uint16_t p, uint16_t changeCount);
    
struct Protocol protocol;

//separationLimit: minimum microseconds between received codes, closer codes are ignored.  
//added volatile for testing
const volatile static uint16_t nSeparationLimit = 4300;
    
volatile static uint16_t nReceiveTolerance;
volatile uint8_t nTransmitterPin;
volatile uint8_t *nPort;
volatile uint16_t nRepeatTransmit;
    
//removed static for testing
volatile long nReceivedValue;
volatile uint16_t nReceivedBitlength;
volatile uint16_t nReceivedDelay;
volatile uint16_t nReceivedProtocol;
    
volatile uint16_t update;
volatile uint16_t overflow;
    
/* 
 * timings[0] contains sync timing, followed by a number of bits
 */
volatile uint16_t timings[RCSWITCH_MAX_CHANGES];

#endif
