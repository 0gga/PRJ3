#pragma once

class LED {
private:
    int pin;

public:
    LED(int physPin);
    void on();
    void off();
    void blink(int times, int delayMs);
};