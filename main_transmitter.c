#include "RCSwitch.h"
#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    DDRB |= (1<<PIN4);
    enableTransmit(&PORTB,PIN4);
        
    for(;;) {
        send(123,7);
        _delay_ms(1000);
        send(321,9);
        _delay_ms(1000);
    }
}
