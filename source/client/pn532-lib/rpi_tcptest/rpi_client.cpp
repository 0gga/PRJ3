#include "rpi_client.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

client::client(int portno, const char *server_ip, std::string doorname) : portno(portno), server_ip(server_ip), doorname(doorname)
{
    rfid_reader = std::make_unique<PN532Reader>(); 

    if(!rfid_reader->init_pn532()){
        std::cerr << "Failed to initialize PN532" << std::endl; 
    }

    while ((sockfd = connect_to_server()) < 0)
    {
        std::cerr << "trying to connect.. 2 seconds wait..";
        sleep(2); //should be 2s
    }
}

client::~client()
{
    close(sockfd);
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

void client::send_data(const std::string& uidstring)
{
    std::string totalstring(doorname);
    totalstring += ':';
    totalstring += uidstring;
    totalstring += '\n';
    std::cerr << totalstring << std::endl;

    ssize_t n = write(sockfd, totalstring.c_str(), strlen(totalstring.c_str()));
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

    std::string uid; 
    while (rfid_reader->isInitialized())
    {
        switch (curr_state)
        {
        case State::WAIT_INPUT:
            std::cerr << "In state: " << curr_state << std::endl;
            if(rfid_reader->waitForScan()){
                uid = rfid_reader->getStringUID();
                curr_state = State::SEND;
            }
            else{
                const long time = 200*1000*1000L;
                nanosleep((const struct timespec[]){{0, time}}, NULL); //200ms
            }
            break;

        case State::SEND:
            std::cerr << "In state: " << curr_state << std::endl;
            send_data(uid);
            curr_state = State::RESPONSE;
            break;

        case State::RESPONSE:
        {
            std::cerr << "In state: " << curr_state << std::endl;
            if (recieve_data())
            {
                curr_state = State::FEEDBACK;
                std::cerr << "Recieved data: " << buffer_receive << std::endl;
            }
            else {
                std::cerr << "No data recieved, set rfid status to false" << std::endl; 
                curr_state = State::WAIT_INPUT; 
            }
            break;
        }

        case State::FEEDBACK:
            std::cerr << "In state: " << curr_state << std::endl;
            io_feedback();
            curr_state = State::WAIT_INPUT;
            break;
        }
    }
}