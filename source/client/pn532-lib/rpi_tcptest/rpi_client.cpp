#include "rpi_client.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

client::client(int portno, const char *server_ip) : portno(portno), server_ip(server_ip)
{
    init_pn532();

    while ((sockfd = connect_to_server()) < 0)
    {
        std::cerr << "trying to connect.. 2 seconds wait..";
        sleep(2); // sleeps 2 seconds -> blocking call
    }
}

client::~client()
{
    close(sockfd);
}

void client::init_pn532()
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
        return;
    }
    PN532_SamConfiguration(&pn532);
    setup_complete = true;
}

void client::get_uid()
{
    while (!has_read)
    {
        // Check if a card is available to read
        uid_len = PN532_ReadPassiveTarget(&pn532, uid, PN532_MIFARE_ISO14443A, 1000);
        if (uid_len == PN532_STATUS_ERROR)
        {
            // std::cerr << "." << std::endl; //optimize this add sleep periode to reduce cpu usage (100ms-1s)
            // fflush(stdout);
            usleep(100000); // 100k microsec -> 100ms -> 0.1s (blocking call)
            continue;
        }
        else
        {
            for (uint8_t i = 0; i < uid_len; i++)
            {
                sprintf(uid_string + i * 2, "%02x", uid[i]); // stores contents of uid in uid_str in hex format (2-characters)
            }
            // std::cerr << "Scanned card with uid:" << uid_string << std::endl;
            has_read = true;
        }
    }
}

int client::connect_to_server()
{
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

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

void client::send_data()
{
    std::string msg_nullterm(uid_string);
    msg_nullterm.push_back('\n'); // null terminate, the uid aswell

    ssize_t n = write(sockfd, msg_nullterm.c_str(), strlen(msg_nullterm.c_str()));
    if (n < 0)
    {
        perror("ERROR writing to socket");
        return;
    }
}

bool client::recieve_data()
{
    size_t total = 0; // antallet af bytes modtaget
    ssize_t n = 0;    // antallet af bytes læst

    // læser indtil \n eller \r, og kan blive stuck her hvis, det ikke er en del af beskeden
    // overvej eventuelt, bare at læse indtil EOF via read

    while (total < sizeof(buffer_receive) - 1)
    {
        n = read(sockfd, buffer_receive + total, 1); // send 1 byte at a time
        if (n < 0)
        {
            perror("ERROR reading from socket");
            return false;
        }

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

void client::io_feedback()
{
    if (strcmp(buffer_receive, "Godkendt") == 0)
    {
        std::cerr << "GODKENDT!! velkommen :))" << std::endl;
    }
    else if (strcmp(buffer_receive, "Denied") == 0)
    {
        std::cerr << "AFVIST!! øv bøv, slem slem slem :((" << std::endl;
    }
    else
    {
        std::cerr << "Nothing detected :/" << std::endl;
        // håndtering af fejlstrenge ?
    }
}

void client::run()
{
    enum State
    {
        WAIT_INPUT,
        SEND,
        RESPONSE,
        FEEDBACK,
    };

    State curr_state = State::WAIT_INPUT;

    while (setup_complete)
    {
        switch (curr_state)
        {
        case State::WAIT_INPUT:
            std::cerr << "In state: " << curr_state << std::endl;
            if (!has_read)
                get_uid();
            curr_state = State::SEND;
            std::cerr << "State is now updated to: " << curr_state << std::endl;
            break;

        case State::SEND:
            std::cerr << "In state: " << curr_state << std::endl;
            if (has_read)
                send_data();
            curr_state = State::RESPONSE;
            std::cerr << "State is now updated to: " << curr_state << std::endl;
            break;

        case State::RESPONSE:
        {
            std::cerr << "In state: " << curr_state << std::endl;
            if (recieve_data())
            {
                curr_state = State::FEEDBACK;
                std::cerr << "Recieved data: " << buffer_receive << std::endl;
            }
            else
                has_read = false;
            std::cerr << "State is now updated to: " << curr_state << std::endl;
            break;
        }

        case State::FEEDBACK:
            std::cerr << "In state: " << curr_state << std::endl;
            io_feedback();
            has_read = false;
            curr_state = State::WAIT_INPUT;
            std::cerr << "State is now updated to: " << curr_state << std::endl;
            break;
        }
    }
}