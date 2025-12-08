#pragma once
#include <stdint.h>
#include <iostream>
#include <string>
#include <regex>
#include "pn532_wrapper.h"

class cli
{
public:
    cli(int portno, const char *server_ip);
    ~cli();

    void run();

private:
    int sockfd;
    int portno;
    const char *server_ip;
    bool connection = false;

    char buffer_receive[4096];

    std::unique_ptr<PN532Reader> rfid_reader;

    int connect_to_server();
    void send_data(const std::string &msg);
    bool recieve_data();
    bool admin_identification();

    void handle_newDoor(const std::string &);
    void handle_newUser(const std::string &);
    void handle_rmDoor(const std::string &);
    void handle_rmUser(const std::string &);
    void handle_exit(const std::string &);
    void handle_shutdown(const std::string &);
    void handle_mvUser(const std::string &);
    void handle_mvDoor(const std::string &);

    void printCommands() const;
};