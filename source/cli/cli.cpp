#include "cli.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <algorithm>

cli::cli(int portno, const char *server_ip) 
{
    while((sockfd = connect_to_server()) < 0)
    {
        std::cerr << "trying to connect... 2 seconds wait...";
        sleep(2); // sleeps 2 seconds -> blocking call
    }
}

cli::~cli()
{
    if(sockfd >= 0)
        close(sockfd);
}


// TCP connect
int cli::connect_to_server()
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

    std::cerr << "Connection established" << std::endl;

    return sockfd;
}

void cli::send_data(const std::string& msg)
{
   std::string cmd = msg + "\n";

   ssize_t n = write(sockfd, cmd.c_str(), cmd.size());

   if(n < 0)
   {
        perror("write");
        return;
   }
}

bool cli::recieve_data()
{
    size_t total = 0; // antallet af bytes modtaget
    ssize_t n = 0;    // antallet af bytes læst

    // læser indtil \n eller \r, og kan blive stuck her hvis, det ikke er en del af beskeden
    // overvej eventuelt, bare at læse indtil EOF via read

    while (total < sizeof(buffer_receive) - 1)
    {
        n = read(sockfd, buffer_receive + total, 1); // send 1 byte at a time
        if (n <= 0)
        {
            perror("ERROR reading from socket");
            return false;
        }

        // end of line check
        if (buffer_receive[total] == '\n' || 
            buffer_receive[total] == '\r')
        {
            break;
        }

        total += n;
    }

    buffer_receive[total] = '\0'; // null termination
    return true;
}

// Helper function for converting std::string to snake_case.\n
// Takes std::string by reference.
/// @param input std::string input
/// @returns void
void cli::to_snake_case(std::string& input) {
    std::string result;
    result.reserve(input.size());

    // Albeit reserving only input.size(), more usually get's allocated.
	// If larger than input.size() it will just reallocate which is negligible regardless, considering the string sizes we'll be handling.
    bool prevLower{false};
    for (const unsigned char c : input) {
        if (std::isupper(c)) {
            if (prevLower)
                result += '_';
            result += static_cast<char>(std::tolower(c));
            prevLower = false;
        } else {
            result += static_cast<char>(c);
            prevLower = true;
        }
    }
    input = std::move(result);
}

bool cli::format(std::string& outMessage, const std::string& input)
{
    static std::regex newUserRx(R"(newUser\s+([A-Za-z0-9_]+)\s+(\d+))");
    static std::regex newDoorRx(R"(newDoor\s+([A-Za-z0-9_]+)\s+(\d+))");
    static std::regex rmUserRx(R"(rmUser\s+([A-Za-z0-9_]+))");
    static std::regex rmDoorRx(R"(rmDoor\s+([A-Za-z0-9_]+))"); 

    std::smatch m;

    //newUser
    if(std::regex_match(input, m, newUserRx))
    {
        std::string name = m[1];
        to_snake_case(name);
        outMessage = "newUser:" + name + ":" + m[2];
        return true;
    }

    // newDoor
    if (std::regex_match(input, m, newDoorRx)) {
        std::string name = m[1];
        to_snake_case(name);
        outMessage = "newDoor:" + name + ":" + m[2];
        return true;
    }

    // rmUser
    if (std::regex_match(input, m, rmUserRx)) {
        std::string name = m[1];
        to_snake_case(name);
        outMessage = "rmUser:" + name;
        return true;
    }

    // rmDoor
    if (std::regex_match(input, m, rmDoorRx)) {
        std::string name = m[1];
        to_snake_case(name);
        outMessage = "rmDoor:" + name;
        return true;
    }

    // shutdown
    if (input == "shutdown") {
        outMessage = "shutdown";
        return true;
    }

    return false;
}


void cli::run() 
{
    printCommands();
    std::cout << "CLI ready. Type commands:\n";
    std::cout << " newUser <User name> <accesslevel>\n";
    std::cout << " newDoor <Door name> <accesslevel>\n";
    std::cout << " rmUser <User name>\n";
    std::cout << " rmDoor <Door name>\n";
    std::cout << " shutdown\n";
    std::cout << " 'help' to print command overview\n\n";

    while(true)
    {
        std::cout << "<";
        std::string input;
        std::getline(std::cin, input)

        if(input.empty())
            continue;

        if(input == "help")
        {
            printCommands();
            continue;
        }

        std::string formatted;

        if(!format(formatted, input))
        {
            std::cout << "Invalid syntax. Type 'help' to see correct syntax\n";
            continue;
        }

        // Send to server
        send_data(formatted);

        // Wait for server response
        if(!recieve_data())
        {
            std::cerr << "Server lost.\n";
            break;
        }

        std::cout <<"[SERVER] " << buffer_receive << "\n";

        // If server needs confirmation for addUser??
        
        
        if(input == "shutdown")
        {
            std::cout << "Shutdown command send.  Closing admin terminal\n";
            break;
        }
    }

    if(sockfd >= 0) {
        ~cli();
        sockfd = -1;
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
        << "  getLog                              - Get log of entries\n"
        << "  shutdown                            - Close server\n"
        << "\n";
}