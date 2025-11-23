#pragma once
#include "pn532_wrapper.h"
#include <memory>


class client
{
public:
    client(int portno, const char *server_ip, std::string doorname);
    ~client();

    void run();

private:
    int sockfd;
    int portno;
    const char *server_ip;
    std::string doorname;

    std::unique_ptr<PN532Reader> rfid_reader; 
    char buffer_receive[256];

    int connect_to_server();
    void send_data(const std::string& uidstring);
    bool recieve_data();
    void io_feedback();
};