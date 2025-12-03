#include "Led.h"
#include <wiringPi.h>
#include <thread>

LED::LED(int physPin) : pin(physPin) {
    pinMode(pin, OUTPUT);
}

void LED::on() {
    digitalWrite(pin, HIGH);
}

void LED::off() {
    digitalWrite(pin, LOW);
}

void LED::blink(int times, int delayMs) {
    std::thread([this, times, delayMs]() {
        for (int i = 0; i < times; i++) {
        
        on();
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        off();
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }
    }).detach();
}
