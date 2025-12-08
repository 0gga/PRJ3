#include "cli.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <cstring>
#include <limits>
#include <thread>

cli::cli(int portno, const char *server_ip) : portno(portno), server_ip(server_ip) {
    while ((sockfd = connect_to_server()) < 0) {
        std::cerr << "trying to connect... 2 seconds wait...";
        sleep(2); // sleeps 2 seconds -> blocking call
    }
}

cli::~cli() {
    if (sockfd >= 0)
        close(sockfd);
}

// TCP connect
int cli::connect_to_server() {
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("Error converting server ip");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        return -1;
    }

    std::cerr << "Connection established" << std::endl;

    connection = true;

    return sockfd;
}

void cli::send_data(const std::string &msg) {
    std::string cmd = msg + "\n";

    ssize_t n = write(sockfd, cmd.c_str(), cmd.size());

    if (n < 0) {
        perror("write");
        return;
    }
}

bool cli::recieve_data() {
    auto read_line = [&](char *buf, size_t bufsize) -> bool {
        size_t total = 0;
        ssize_t n = 0;

        memset(buf, 0, bufsize);

        while (total < bufsize - 1) {
            n = read(sockfd, buf + total, 1);

            if (n < 0) {
                perror("ERROR reading from socket");
                return false;
            }

            if (n == 0) {
                std::cerr << "Server closed connection\n";
                return false;
            }

            if (buf[total] == '\n' || buf[total] == '\r')
                break;

            total += n;
        }

        buf[total] = '\0';
        return true;
    };

    // -----------------------
    // 1) Read first required line (blocking)
    // -----------------------
    if (!read_line(buffer_receive, sizeof(buffer_receive)))
        return false;

    // Process delimiter
    char *delimiter = strstr(buffer_receive, "%%%");
    if (delimiter) {
        char *payload = delimiter + 3;
        size_t payload_len = strlen(payload);
        memmove(buffer_receive, payload, payload_len + 1);
    }

    std::cout << buffer_receive << std::endl;

    // -----------------------
    // 2) Drain all queued bytes (non-blocking)
    // -----------------------
    while (true) {
        int available = 0;

        // Check how many bytes are currently queued in the socket buffer
        if (ioctl(sockfd, FIONREAD, &available) < 0)
            break;

        if (available <= 0)
            break; // nothing more queued

        // Read additional line
        if (!read_line(buffer_receive, sizeof(buffer_receive)))
            return false;

        // Process delimiter again
        char *delimiter2 = strstr(buffer_receive, "%%%");
        if (delimiter2) {
            char *payload = delimiter2 + 3;
            size_t payload_len = strlen(payload);
            memmove(buffer_receive, payload, payload_len + 1);
        }

        std::cout << buffer_receive << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return true;
}

bool cli::admin_identification() {
    std::string cli_identification;
    while (true) {
        if (!recieve_data())
            return false;

        if (strcmp(buffer_receive, "Another Admin is connected") == 0) {
            return false;
        }

        if (strcmp(buffer_receive, "Input CLI Identification") == 0 ||
            strcmp(buffer_receive, "Incorrect CLI Identification") == 0) {
            std::cout << "> ";
            std::cin >> cli_identification;

            send_data(cli_identification);

            // Vent på næste besked
            continue;
        }

        if (strcmp(buffer_receive, "CLI is ready") == 0) {
            std::cout << "Admin identification accepted." << std::endl;
            return true;
        }
    }
}

void cli::run() {
    printCommands();

    if (!admin_identification())
        return;


    while (connection) {
        std::cout << "Input command. Type 'help' to see overview\n\n";
        std::cout << "> ";

        std::string cmd;
        std::cin >> cmd;   // læser kommando uden newline-problemer

        std::string rest;
        std::getline(std::cin, rest); // resten af linjen inkl. arguments

        std::string input = cmd + rest; // samlet kommando

        if (input.rfind("newDoor", 0) == 0)
            handle_newDoor(input);

        else if (input.rfind("newUser", 0) == 0)
            handle_newUser(input);

        else if (input.rfind("rmDoor", 0) == 0)
            handle_rmDoor(input);

        else if (input.rfind("rmUser", 0) == 0)
            handle_rmUser(input);

        else if (input.rfind("getSystemLog", 0) == 0 ||
                 input.rfind("getUserLog", 0) == 0 ||
                 input.rfind("getDoorLog", 0) == 0)
            handle_log(input);
        
        else if (input.rfind("mvUser", 0) == 0)
            handle_mvUser(input);

        else if (input.rfind("mvDoor", 0) == 0)
            handle_mvDoor(input);

        else if (input == "exit")
            handle_exit(input);

        else if (input == "shutdown")
            handle_shutdown(input);

        else if (input == "help")
            printCommands();
    }

    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
        return;
    }
}

void cli::printCommands() const {
    std::cout
            << "  SmartLock commands\n"
            << "  Commands :\n"
            << "  newDoor <Door name> <accessLevel>   - Add door\n"
            << "  newUser <Username> <accessLevel>    - Add user (includes NFC-scan)\n"
            << "  rmDoor <Door name>                  - Delete door\n"
            << "  rmUser <Username>                   - Delete user\n"
            << "  mvDoor <Door name> <accessLevel>    - Edit existing door\n"
            << "  mvUser <Username> <accessLevel>     - Edit existing user\n"
            << "  exit                                - Exit and kill CLI connection \n"
            << "  shutdown                            - Shutdown the entire system\n"
            << "  getSystemLog <'date'>               - Get date-specific system log\n"
            << "  getUserLog <Username>               - Get user-specific log\n"
            << "  getDoorLog <Door name>              - Get door-specific log\n"
            << "  help                                - Print command overview\n"
            << "\n";

    return;
}

void cli::handle_newDoor(const std::string &cmd) {
    send_data(cmd);

    if (!recieve_data())
        return;

    while (strcmp(buffer_receive, "CLI is ready") == 0) {
        if (!recieve_data())
            return;
    }

    if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0) {
        return;
    }

    std::string confirmation;
    std::cout << "<approved/denied> ";
    std::cin >> confirmation;

    send_data(confirmation);

    if (!recieve_data())
        return;
}

void cli::handle_newUser(const std::string &cmd) {
    send_data(cmd);

    if (!recieve_data())
        return;

    while (strcmp(buffer_receive, "CLI is ready") == 0) {
        if (!recieve_data())
            return;
    }

    if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0) {
        return;
    }

    // Second setup RFID reader.
    rfid_reader = std::make_unique<PN532Reader>();

    if (!rfid_reader->init_pn532()) {
        std::cerr << "Failed to initialize PN532" << std::endl;
    }

    std::string uid; 
    bool scanned = false; 
    while(!scanned) { //blocking loop.
		if(rfid_reader->waitForScan()){
            scanned = true; 
            uid = rfid_reader->getStringUID();
        }
    }
    
    send_data(uid);

    if (!recieve_data())
        return;

    std::string confirmation;
    std::cout << "<approved/denied> ";
    std::cin >> confirmation;

    send_data(confirmation);

    if (!recieve_data())
        return;
}

void cli::handle_rmDoor(const std::string &cmd) {
    send_data(cmd);

    if (!recieve_data())
        return;

    while (strcmp(buffer_receive, "CLI is ready") == 0) {
        if (!recieve_data())
            return;
    }


    if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0 ||
        strcmp(buffer_receive, "Door could not be found") == 0) {
        return;
    }

    std::string confirmation;
    std::cout << "<approved/denied> ";
    std::cin >> confirmation;

    send_data(confirmation);

    if (!recieve_data())
        return;
}

void cli::handle_rmUser(const std::string &cmd) {
    send_data(cmd);

    if (!recieve_data())
        return;

    while (strcmp(buffer_receive, "CLI is ready") == 0) {
        if (!recieve_data())
            return;
    }

    if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0 ||
        strcmp(buffer_receive, "User could not be found") == 0) 
    {
        return;
    }

    std::string confirmation;
    std::cout << "<approved/denied> ";
    std::cin >> confirmation;

    send_data(confirmation);

    if (!recieve_data())
        return;
}

void cli::handle_exit(const std::string &cmd) {
    send_data(cmd);

    if (!recieve_data())
        return;


    close(sockfd);
    sockfd = -1;
    connection = false;
    return;
}

void cli::handle_shutdown(const std::string &cmd) {
    send_data(cmd);

    if (!recieve_data())
        return;


    close(sockfd);
    sockfd = -1;
    connection = false;

    return;
}

void cli::handle_mvUser(const std::string &cmd) {
    send_data(cmd);

    if (!recieve_data())
        return;

    while (strcmp(buffer_receive, "CLI is ready") == 0) {
        if (!recieve_data())
            return;
    }

    if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0 ||
        strcmp(buffer_receive, "User could not be found") == 0) {
        return;
    }

    std::string confirmation;
    std::cout << "<approved/denied> ";
    std::cin >> confirmation;

    send_data(confirmation);

    if (!recieve_data())
        return;

}

void cli::handle_mvDoor(const std::string &cmd) {
    send_data(cmd);

    if (!recieve_data())
        return;

    while (strcmp(buffer_receive, "CLI is ready") == 0) {
        if (!recieve_data())
            return;
    }

    if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0 ||
        strcmp(buffer_receive, "Door could not be found") == 0) {
        return;
    }

    std::string confirmation;
    std::cout << "<approved/denied> ";
    std::cin >> confirmation;

    send_data(confirmation);

    if (!recieve_data())
        return;
}

void cli::handle_log( const std::string &cmd)
{
    send_data(cmd);

    if (!recieve_data())
        return;
}
