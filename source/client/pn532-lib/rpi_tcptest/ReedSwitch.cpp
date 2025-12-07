#include "ReedSwitch.h"
#include <wiringPi.h>

// physPin = *physical pin number* (BOARD numbering)
ReedSwitch::ReedSwitch(int physPin)
{
    pin = physPin;
    pinMode(pin, INPUT);
    pullUpDnControl(pin, PUD_DOWN); // enable pulldown
}

// returns true, when magnet is present!
bool ReedSwitch::magnetPresent() const
{
    return digitalRead(pin) == LOW;
}

bool ReedSwitch::isDoorOpen() const
{
    return magnetPresent(); // magnet present -> door open.
}

bool ReedSwitch::isDoorClosed() const
{
    return !magnetPresent(); // magnet not present -> door not open.
}
