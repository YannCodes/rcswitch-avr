#include "RCSwitch.h"
#include <avr/io.h>

int main(void)
{
    DDRB |= (1 << PIN5);
    enableReceive();
    
    for(;;)
    {
        if(nReceivedValue == 123)
        {
            PORTB |= (1 << PIN5);
        }
        else if(nReceivedValue == 321)
        {
            PORTB &= ~(1 << PIN5);
        }
    }
}
