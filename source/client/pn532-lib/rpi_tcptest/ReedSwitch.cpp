#include "ReedSwitch.h"
#include <wiringPi.h>

ReedSwitch::ReedSwitch(int physPin) : pin(physPin)
{
    pinMode(pin, INPUT);
    pullUpDnControl(pin, PUD_UP); // Antager reed mellem GND og input
}

bool ReedSwitch::isOpen()
{
    return digitalRead(pin);
}
