/*
  rcswitch-avr - avr-c port of the RCSwitch library (https://github.com/sui77/rc-switch/)
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
  - Andreas Steinel / A.<lastname>(at)gmail(dot)com
  - Max Horn / max(at)quendi(dot)de
  - Robert ter Vehn / <first name>.<last name>(at)gmail(dot)com
  - Johann Richard / <first name>.<last name>(at)gmail(dot)com
  - Vlad Gheorghe / <first name>.<last name>(at)gmail(dot)com https://github.com/vgheo
  
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

#include "RCSwitch.h"

/* Format for protocol definitions:
 * {pulselength, Sync bit, "0" bit, "1" bit}
 * 
 * pulselength: pulse length in microseconds, e.g. 350
 * Sync bit: {1, 31} means 1 high pulse and 31 low pulses
 *     (perceived as a 31*pulselength long pulse, total length of sync bit is
 *     32*pulselength microseconds), i.e:
 *      _
 *     | |_______________________________ (don't count the vertical bars)
 * "0" bit: waveform for a data bit of value "0", {1, 3} means 1 high pulse
 *     and 3 low pulses, total length (1+3)*pulselength, i.e:
 *      _
 *     | |___
 * "1" bit: waveform for a data bit of value "1", e.g. {3,1}:
 *      ___
 *     |   |_
 *
 * These are combined to form Tri-State bits when sending or receiving codes.
 */

static const struct Protocol proto [] PROGMEM = {
    { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },    // protocol 1
    { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },    // protocol 2
    { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },    // protocol 3
    { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },    // protocol 4
    { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },    // protocol 5
    { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true }      // protocol 6 (HT6P20B)
};

enum {
    numProto = sizeof(proto) / sizeof(proto[0])
};

/**
 * Sets the protocol to send, from a list of predefined protocols
 */
void setProtocol(uint16_t nProtocol)
{
    if (nProtocol < 1 || nProtocol > numProto)
    {
        nProtocol = 1;
    }
    memcpy_P(&protocol, &proto[nProtocol-1], sizeof(Protocol));
}  

/**
 * Enable transmissions
 */
void enableTransmit(volatile uint8_t *port, uint8_t pin)
{
    setProtocol(1);
    nRepeatTransmit = 10;  
    nTransmitterPin = pin;
    nPort = port;
}

/**
 * Disable transmissions
 */
void disableTransmit()
{
    nTransmitterPin = -1;
}

/**
 * Send a tristate code word consisting of the letter 0, 1, F
 */
void sendTriState(const char* sCodeWord)
{
    // turn the tristate code word into the corresponding bit pattern, then send it
    uint16_t code = 0;
    uint16_t length = 0;
    for (const char* p = sCodeWord; *p; p++)
    {
        code <<= 2L;
        switch (*p)
        {
            case '0':
                // bit pattern 00
                break;
            case 'F':
                // bit pattern 01
                code |= 1L;
                break;
            case '1':
                // bit pattern 11
                code |= 3L;
                break;
        }
        length += 2;
    }
    send(code, length);
}

/**
 * Send a binary code word consisting of the letter 0, 1
 */
void sendBinary(const char* sCodeWord)
{
    // turn the tristate code word into the corresponding bit pattern, then send it
    uint16_t code = 0;
    uint16_t length = 0;
    for (const char* p = sCodeWord; *p; p++)
    {
        code <<= 1L;
        if (*p != '0')
            code |= 1L;
        length++;
    }
    send(code, length);
}

/**
 * Transmit the first 'length' bits of the integer 'code'. The
 * bits are sent from MSB to LSB, i.e., first the bit at position length-1,
 * then the bit at position length-2, and so on, till finally the bit at position 0.
 */
void send(uint16_t code, uint16_t length)
{
    if (nTransmitterPin == -1)
	return;

    /* TODO :  make sure the receiver is disabled while we transmit
      disableReceive();
    */

    for (uint16_t nRepeat = 0; nRepeat < nRepeatTransmit; nRepeat++)
    {
        for (int8_t i = length-1; i >= 0; i--)
        {
            if (code & (1L << i))
                transmit(protocol.one);
            else
                transmit(protocol.zero);
        }
        transmit(protocol.syncFactor);
    }

    /* TODO : enable receiver again if we just disabled it
      enableReceive();
    */
}

/*
  The current implementation is taken from Arduino's delayMicroseconds()
  See https://github.com/arduino/Arduino/blob/master/hardware/arduino/avr/cores/arduino/wiring.c#L119
*/

/* Delay for the given number of microseconds.  Assumes a 8 or 16 MHz clock. */
void delay_us(uint16_t us)
{
    // call = 4 cycles + 2 to 4 cycles to init us(2 for constant delay, 4 for variable)

    // calling avrlib's delay_us() function with low values (e.g. 1 or
    // 2 microseconds) gives delays longer than desired.
    //delay_us(us);
#if F_CPU == 16000000L
    // for the 16 MHz clock on most Arduino boards

    // for a one-microsecond delay, simply return.  the overhead
    // of the function call takes 14 (16) cycles, which is 1us
    if (us <= 1) return; //  = 3 cycles, (4 when true)

    // the following loop takes 1/4 of a microsecond (4 cycles)
    // per iteration, so execute it four times for each microsecond of
    // delay requested.
    us <<= 2; // x4 us, = 4 cycles

    // account for the time taken in the preceeding commands.
    // we just burned 19 (21) cycles above, remove 5, (5*4=20)
    // us is at least 8 so we can substract 5
    us -= 5; // = 2 cycles,

#elif F_CPU == 8000000L
    // for the 8 MHz internal clock

    // for a 1 and 2 microsecond delay, simply return.  the overhead
    // of the function call takes 14 (16) cycles, which is 2us
    if (us <= 2) return; //  = 3 cycles, (4 when true)

    // the following loop takes 1/2 of a microsecond (4 cycles)
    // per iteration, so execute it twice for each microsecond of
    // delay requested.
    us <<= 1; //x2 us, = 2 cycles

    // account for the time taken in the preceeding commands.
    // we just burned 17 (19) cycles above, remove 4, (4*4=16)
    // us is at least 6 so we can substract 4
    us -= 4; // = 2 cycles

#else
#error "F_CPU value different from 8000000 and 16000000 or not defined!"
#endif

    // busy wait
    __asm__ __volatile__ (
	"1: sbiw %0,1" "\n\t" // 2 cycles
	"brne 1b" : "=w" (us) : "0" (us) // 2 cycles
	);
    // return = 4 cycles
}

/**
 * Transmit a single high-low pulse.
 */
void transmit(HighLow pulses)
{
    //TODO: allow to send invertedSignal

    *nPort |= (1<<nTransmitterPin);
    delay_us(protocol.pulseLength * pulses.high);
    *nPort &= ~(1<<nTransmitterPin);
    delay_us(protocol.pulseLength * pulses.low);
}

void enableReceive()
{
    nReceivedValue = 0;
    nReceivedBitlength = 0;
    nReceivedDelay = 0;
    nReceivedProtocol = 0;
    nReceiveTolerance = 60;
    
    //setup interrupt pin (INT0) as input
    DDRD &= ~(1<<PIN2);
    //INT0 triggered on any logical change
    EICRA |= (1<<ISC00);
    //enable INT0
    EIMSK |= (1<<INT0);
    //enable interrupt
    sei();
    
    overflow = 0;
    //setup timer with 256 prescaler
    TCCR0B |= (1<<CS02);
    //setup overflow interrupt
    TIMSK0 |= (1<<TOIE0);
    //reset the counter
    TCNT0=0;
}

bool available()
{
    return nReceivedValue != 0;
}

void resetAvailable()
{
    nReceivedValue = 0;
}

/* helper function for the receiveProtocol method */
static inline uint16_t diff(uint16_t A, uint16_t B)
{
    return abs(A - B);
}

//removed static for testing
bool receiveProtocol(const uint16_t p, uint16_t changeCount)
{
    Protocol pro;
    memcpy_P(&pro, &proto[p-1], sizeof(Protocol));
    uint16_t code = 0;

    //Assuming the longer pulse length is the pulse captured in timings[0]
    const uint16_t syncLengthInPulses =  ((pro.syncFactor.low) > (pro.syncFactor.high)) ? (pro.syncFactor.low) : (pro.syncFactor.high);
    const uint16_t delay = timings[0] / syncLengthInPulses;
    const uint16_t delayTolerance = delay * nReceiveTolerance / 100;
    
    /* For protocols that start low, the sync period looks like
     *               _________
     * _____________|         |XXXXXXXXXXXX|
     *
     * |--1st dur--|-2nd dur-|-Start data-|
     *
     * The 3rd saved duration starts the data.
     *
     * For protocols that start high, the sync period looks like
     *
     *  ______________
     * |              |____________|XXXXXXXXXXXXX|
     *
     * |-filtered out-|--1st dur--|--Start data--|
     *
     * The 2nd saved duration starts the data
     */
    const uint16_t firstDataTiming = (pro.invertedSignal) ? (2) : (1);

    for (uint16_t i = firstDataTiming; i < changeCount - 1; i += 2)
    {
        code <<= 1;
	
        if (diff(timings[i], delay * pro.zero.high) < delayTolerance &&
            diff(timings[i + 1], delay * pro.zero.low) < delayTolerance)
        {
            // zero
        }
	
        else if (diff(timings[i], delay * pro.one.high) < delayTolerance &&
                   diff(timings[i + 1], delay * pro.one.low) < delayTolerance)
        {
            // one
            code |= 1;
        }

        else
        {
            // Failed
            return false;
        }
    }

    if (changeCount > 7) // ignore very short transmissions: no device sends them, so this must be noise
    {
        nReceivedValue = code;
        nReceivedBitlength = (changeCount - 1) / 2;
        nReceivedDelay = delay;
        nReceivedProtocol = p;
        return true;
    }

    return false;
}

ISR(INT0_vect)
{
    static uint16_t  changeCount = 0;
    static uint16_t  repeatCount = 0;
    uint16_t  time = TCNT0;
    uint16_t duration;

#if F_CPU == 8000000
    duration = time * 32;
#elif F_CPU == 16000000
    duration = time * 16;
#else
#error "F_CPU value different from 8000000 and 16000000 or not defined!"
#endif
  
    if(overflow)
    {
#if F_CPU == 8000000
        duration += (8192 * overflow);
#elif F_CPU == 16000000
        duration += (4096 * overflow);
#else
#error "F_CPU value different from 8000000 and 16000000 or not defined!"
#endif
        overflow = 0;
    }

    if (duration > nSeparationLimit)
    {
	// A long stretch without signal level change occurred. This could
	// be the gap between two transmission.
        if (diff(duration, timings[0]) < 200)
        {
            // This long signal is close in length to the long signal which
            // started the previously recorded timings; this suggests that
            // it may indeed by a a gap between two transmissions (we assume
            // here that a sender will send the signal multiple times,
            // with roughly the same gap between them).
            repeatCount++;
            if (repeatCount == 2)
            {
                for(uint16_t  i = 1; i <= numProto; i++)
                {
                    if (receiveProtocol(i, changeCount))
                    {
                        // receive succeeded for protocol i
                        break;
                    }
                }
                repeatCount = 0;
            }
        }
        changeCount = 0;
    }
 
    // detect overflow
    if (changeCount >= RCSWITCH_MAX_CHANGES)
    {
	changeCount = 0;
	repeatCount = 0;
    }

    //record the signal duration
    timings[changeCount++] = duration;
  
    //reset timer
    TCNT0 = 0;
}

ISR(TIMER0_OVF_vect)
{
    overflow++;
}
