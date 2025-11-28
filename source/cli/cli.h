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
    bool connection = false;

    char buffer_receive[256]; // Evt større?

    int connect_to_server();
    void send_data(const std::string& msg);
    bool recieve_data();
    bool admin_identification();

   bool handle_newDoor(const std::string&);
   bool handle_newUser(const std::string&);
   bool handle_rmDoor(const std::string&);
   bool handle_rmUser(const std::string&);
   bool handle_getLog(const std::string&);
   bool handle_exit(const std::string&);
   bool handle_shutdown(const std::string&);

    void printCommands() const;

};