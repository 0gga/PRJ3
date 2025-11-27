#pragma once

#include <stdint.h>
#include <iostream>
#include <string>
#include <regex>

class cli {
public:
    cli(int portno, const char *server_ip);
    ~cli();

    void run();

private:
    int sockfd;
    int portno;
    const char *server_ip;

    char buffer_receive[256]; // Evt større?

    int connect_to_server();
    void send_data(const std::string& msg);
    bool recieve_data();

    // Helper function
    bool format(std::string& outMessage, const std::string& input);
    void to_snake_case(std::string& input) const;

    void printCommands() const;

};