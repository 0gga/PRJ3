#include "Led.h"
#include <wiringPi.h>
#include <thread>

Led::Led(int physPin) : pin(physPin)
{
    pinMode(pin, OUTPUT);
}

void Led::on()
{
    digitalWrite(pin, HIGH);
}

void Led::off()
{
    digitalWrite(pin, LOW);
}

void Led::blink(int delayMs)
{
    on();
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    off();
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
}