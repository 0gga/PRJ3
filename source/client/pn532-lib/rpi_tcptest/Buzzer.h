#pragma once

class Buzzer {
private:
    int pin;

public:
    Buzzer(int physPin);
    void beep(int durationMs);
    void beepPattern(int repeats);
};