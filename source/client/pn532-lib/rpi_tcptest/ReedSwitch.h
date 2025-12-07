#pragma once

class ReedSwitch
{
private:
    int pin;

public:
    ReedSwitch() = default;
    ReedSwitch(int physPin);
    bool magnetPresent() const;
    bool isDoorOpen() const;
    bool isDoorClosed() const;
};