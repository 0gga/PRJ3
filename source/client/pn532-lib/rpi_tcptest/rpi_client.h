#pragma once

#include <stdint.h>
#include <iostream>

extern "C"
{
#include "pn532.h"
#include "pn532_rpi.h"
}

class client
{
public:
    client(int portno, const char *server_ip);
    ~client();

    void run();

private:
    int sockfd;
    int portno;
    const char *server_ip;

    PN532 pn532;
    bool setup_complete = false;

    uint8_t uid[MIFARE_UID_MAX_LENGTH];
    int32_t uid_len = 0;
    char uid_string[32];
    char buffer_receive[256];
    bool has_read = false;

    void init_pn532();
    void get_uid();
    int connect_to_server();
    void send_data();
    bool recieve_data();
    void io_feedback();
};