#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <cinttypes> //for PRIu8

extern "C"
{
#include "pn532.h"
#include "pn532_rpi.h"
}
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// SOCKET LIBS:
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void init_pn532(uint8_t *buff_, PN532 &pn532_, bool &setup_complete_)
{
    // PN532_SPI_Init(&pn532);
    PN532_I2C_Init(&pn532_);
    // PN532_UART_Init(&pn532);
    if (PN532_GetFirmwareVersion(&pn532_, buff_) == PN532_STATUS_OK)
    {
        std::cerr << "Found PN532 with firmware version: " << int(buff_[1]) << "." << int(buff_[2]) << std::endl;
    }
    else
    {
        std::cerr << "Setup incomplete" << std::endl;
        return;
    }
    PN532_SamConfiguration(&pn532_);
    std::cerr << "Waiting for RFID/NFC card...\r\n"
              << std::endl;
    setup_complete_ = true;
}

void get_uid(uint8_t *uid_, int32_t &uid_len_, PN532 &pn532_, bool &has_read_, char *uid_string)
{
    while (!has_read_)
    {
        // Check if a card is available to read
        uid_len_ = PN532_ReadPassiveTarget(&pn532_, uid_, PN532_MIFARE_ISO14443A, 1000);
        if (uid_len_ == PN532_STATUS_ERROR)
        {
            // std::cerr << "." << std::endl; //optimize this add sleep periode to reduce cpu usage (100ms-1s)
            // fflush(stdout);
            usleep(100000); // 100k microsec -> 100ms -> 0.1s (blocking call)
            continue;
        }
        else
        {
            for (uint8_t i = 0; i < uid_len_; i++)
            {
                sprintf(uid_string + i * 2, "%02x", uid_[i]); // stores contents of uid in uid_str in hex format (2-characters)
            }
            std::cerr << "Scanned card with uid:" << uid_string << std::endl;
            has_read_ = true;
        }
    }
}

int connect_to_server(const char *server_ip, int portno)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    // struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    // serv_addr.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
    {
        perror("Error converting server ip");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        return -1;
    }

    std::cerr << "connection established" << std::endl;

    return sockfd;
}

void send_data(int sockfd, char *buffer_send)
{
    std::string msg_nullterm(buffer_send);
    msg_nullterm.push_back('\n'); // null terminate, the uid aswell

    ssize_t n = write(sockfd, msg_nullterm.c_str(), strlen(msg_nullterm.c_str()));
    if (n < 0)
    {
        perror("ERROR writing to socket");
        return;
    }
}

bool recieve_data(int sockfd, char *buffer_receive, size_t len)
{
    long total = 0; // antallet af bytes modtaget
    ssize_t n = 0;  // antallet af bytes læst

    while (total < len - 1)
    {
        std::cerr << "in while loop" << std::endl;
        n = read(sockfd, buffer_receive + total, 1); // send 1 byte at a time
        if (n < 0)
        {
            perror("ERROR reading from socket");
            return false;
        }

        // unsigned char c = buffer_receive[total];
        // printf("Byte[%ld] = 0x%02X ('%c')\n", total, c, (c >= 32 && c <= 126) ? c : '.');

        // end of line check
        if (buffer_receive[total] == '\n' || buffer_receive[total] == '\r')
        {
            break;
        }

        total += n;
    }

    buffer_receive[total] = '\0'; // null termination

    // std::cerr << "Recieved data: " << buffer_receive << std::endl;

    return true;
}

void io_feedback(char *response)
{
    if (strcmp(response, "Godkendt") == 0)
    {
        std::cerr << "GODKENDT!! velkommen :))" << std::endl;
    }
    else if (strcmp(response, "Afvist") == 0)
    {
        std::cerr << "AFVIST!! øv bøv, slem slem slem :((" << std::endl;
    }
    else
    {
        std::cerr << "Nothing detected :/" << std::endl;
        // håndtering af fejlstrenge ?
    }
}

int main(int argc, char **argv)
{
    uint8_t uid[MIFARE_UID_MAX_LENGTH];
    int32_t uid_len = 0;
    char uid_string[32];
    bool has_read = false;
    char buffer_receive[256];

    uint8_t buff_firmware[255];
    PN532 pn532;
    bool setup_complete = false;

    init_pn532(buff_firmware, pn532, setup_complete);

    const char *server_ip = (argc > 1) ? argv[1] : "172.16.15.3";

    int sockfd;
    while ((sockfd = connect_to_server(server_ip, 9000)) < 0)
    {
        std::cerr << "trying to connect.. 2 seconds wait..";
        sleep(2); // sleeps 2 seconds -> blocking call
    }

    while (setup_complete)
    {
        if (!has_read)
            get_uid(uid, uid_len, pn532, has_read, uid_string);

        if (has_read)
        {
            send_data(sockfd, uid_string);
            if (recieve_data(sockfd, buffer_receive, sizeof(buffer_receive)))
            {
                io_feedback(buffer_receive);
            }
            has_read = false;
        }
    }

    close(sockfd);

    return 0;
}

/*
    json data;
    data["access"] = "granted";
    data["UID"] = uid_str;
    data["LEVEL"] = 3;
    data.dump(4)
*/
