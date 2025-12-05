#pragma once
#include <stdint.h>
#include <iostream>

extern "C"
{
#include "pn532.h"
#include "pn532_rpi.h"
}

class PN532Reader{
public:
    PN532Reader(); 
    bool init_pn532();
    bool isInitialized() const; 
    bool waitForScan(); 
    std::string getStringUID() const;

private: 
    PN532 pn532;
    bool setup_complete = false;

    uint8_t uid[MIFARE_UID_MAX_LENGTH];
    int32_t uid_len = 0;
    char uid_string[32];
}; 