#pragma once
#include "pn532_wrapper.h"
#include "Buzzer.h"
#include "Led.h"
#include "ReedSwitch.h"
#include <memory>

class client
{
public:
    client(int portno, const char *server_ip, std::string doorname);
    ~client();

    void run();
    void initLeds();

private:
    int sockfd;
    int portno;
    const char *server_ip;
    std::string doorname;

    std::unique_ptr<Buzzer> buzzer;
    std::unique_ptr<Led> Led_Green;
    std::unique_ptr<Led> Led_Yellow;
    std::unique_ptr<Led> Led_Red;
    std::unique_ptr<ReedSwitch> RS;
    std::unique_ptr<PN532Reader> rfid_reader;
    char buffer_receive[256];

    int connect_to_server();
    void send_data(const std::string &uidstring);
    bool recieve_data();
    void io_feedback();
};