#include "ReedSwitch.h"
#include <wiringPi.h>

ReedSwitch::ReedSwitch(int physPin) : pin(physPin) {
    pinMode(pin, INPUT);
    pullUpDnControl(pin, PUD_UP);  // Antar reed mellem GND og input
}

bool ReedSwitch::isOpen() {
    return digitalRead(pin) == LOW; // low -> magnet væk -> døren åben
}
