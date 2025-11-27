#include "pn532_wrapper.h"
#include <cstring>

PN532Reader::PN532Reader() {
    memset(&pn532, 0, sizeof(pn532)); 
}

bool PN532Reader::init_pn532()
{
    uint8_t buff_firmware[255];
    // PN532_SPI_Init(&pn532);
    PN532_I2C_Init(&pn532);
    // PN532_UART_Init(&pn532);
    if (PN532_GetFirmwareVersion(&pn532, buff_firmware) == PN532_STATUS_OK)
    {
        std::cerr << "Found PN532 with firmware version: " << int(buff_firmware[1]) << "." << int(buff_firmware[2]) << std::endl;
    }
    else
    {
        std::cerr << "Setup incomplete" << std::endl;
        return false;
    }
    PN532_SamConfiguration(&pn532);
    setup_complete = true; 
    return true; 
}

 bool PN532Reader::isInitialized() const {
    return setup_complete; 
 }


bool PN532Reader::waitForScan(){
     
    if(!isInitialized()){
        return false; 
    }
    
    // Check if a card is available to read
    uid_len = PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000);
    if (uid_len == PN532_STATUS_ERROR)
    {
        return false; 
    }
        
    for (uint8_t i = 0; i < uid_len; i++)
    {
       sprintf(uid_string + i * 2, "%02x", uid[i]); // stores contents of uid in uid_str in hex format (2-characters)
    }           
    // std::cerr << "Scanned card with uid:" << uid_string << std::endl;
        
    return true; 
}

std::string PN532Reader::getStringUID() const
{
    return uid_string; 
}

