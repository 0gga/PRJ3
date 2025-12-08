#pragma once

class Led
{
private:
    int pin;

public:
    Led() = default;
    Led(int physPin);
    void on();
    void off();
    void blink(int delayMs);
};