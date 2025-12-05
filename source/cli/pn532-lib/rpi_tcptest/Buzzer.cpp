#include "Buzzer.h"
#include <wiringPi.h>
#include <thread>

Buzzer::Buzzer(int physPin) : pin(physPin)
{
    pinMode(pin, OUTPUT);
}

void Buzzer::beep(int durationMs)
{
    std::thread([this, durationMs]()
                {
        digitalWrite(pin, HIGH);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
        digitalWrite(pin, LOW); })
        .detach();
}

void Buzzer::beepPattern(int repeats)
{
    std::thread([this, repeats]()
                {
        for (int i = 0; i < repeats; i++) {
            beep(200);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        } })
        .detach();
}
