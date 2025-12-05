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

void Led::blink(int times, int delayMs)
{
    std::thread([this, times, delayMs]()
                {
        for (int i = 0; i < times; i++) {
        
        on();
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        off();
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    } })
        .detach();
}
