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

    char buffer_receive[256];

    int connect_to_server();
    bool send_line(const std::string& line);
    bool recieve_line(std::string& out);

    void trim(std::string& s) const;
    void to_snake_case(std::string& input) const;

    
    bool starts_with(const std::string& s, const std::string& prefix) const;
    void format_newDoor(const std::string& doorData) const;
    void format_newUser(const std::string& userData) const;
    void format_rmDoor(const std::string& doorData)const;
    void format_rmUser(const std::string& userData)const;

    void printCommands() const;

};