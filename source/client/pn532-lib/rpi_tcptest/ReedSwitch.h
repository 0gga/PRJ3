#pragma once

class ReedSwitch {
private:
    int pin;

public:
    ReedSwitch(int physPin);
    bool isOpen();
};