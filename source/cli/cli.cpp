#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <algorithm>
#include "cli.h"

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



int cli::connect_to_server()()
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

bool cli::send_line(const std::string& line)
{
   std::string cmd = line + "\n";

   ssize_t n = write(sockfd, cmd.c_str(), cmd.size());

   if(n < 0)
   {
    perror("write");
    return false;
   }

   return true;
}

bool cli::recieve_line(std::string& out)
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

    // std::cerr << "Recieved data: " << buffer_receive << std::endl;

    return true;
}

void cli::trim(std::string& s) const
{
    auto notSpace = [](int ch) { return !std::isspace(ch); };

    s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
    s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
}


void cli::format_newDoor(const std::string& doorData) const
{
    static const std::regex cliSyntax(R"(^newDoor\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
	std::smatch match;

	if (!std::regex_match(doorData, match, cliSyntax)) {
    std::cerr << "Failed to add new door - Incorrect CLI syntax\n";
    return;
	}

	std::string name = match[1].str();
	to_snake_case(name);
}

void cli::format_newUser(const std::string& userData) const
{
    static const std::regex cliSyntax(R"(^newUser\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
	std::smatch match;
	
    if (!std::regex_match(userData, match, cliSyntax)) {
        std::cerr << "Failed to add new user - Incorrect CLI syntax\n";
		return;
	}

	std::string name = match[1].str();
	to_snake_case(name);
}


void cli::format_rmDoor(const std::string& doorData)
{
    static const std::regex cliSyntax(R"(^rmDoor\s+([A-Za-z_]+)$)");
	std::smatch match;
	
    if (!std::regex_match(doorData, match, cliSyntax)) {
        std::cerr << "Failed to remove door - Incorrect CLI syntax\n";
		return;
	}

	std::string name = match[1].str();
    to_snake_case(name);
}

void format_rmUser(const std::string& userData)
{
    static const std::regex cliSyntax(R"(^rmUser\s+([A-Za-z_]+)$)");
	std::smatch match;
	if (!std::regex_match(userData, match, cliSyntax)) {
        std::cerr << "Failed to remove user - Incorrect CLI syntax\n";
		return;
	}

	std::string name = match[1].str();
	to_snake_case(name);
}


/// Helper function for converting std::string to snake_case.\n
/// Takes std::string by reference.
/// @param input std::string input
/// @returns void
void cli::to_snake_case(std::string& input) {
    std::string result;
    result.reserve(input.size());
    /// Albeit reserving only input.size(), more usually get's allocated.
	   /// If larger than input.size() it will just reallocate which is negligible regardless, considering the string sizes we'll be handling.
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

void cli::run() 
{
    printCommands();

    while(true)
    {
        std::cout << "<";
        std::string line;
        std::getline(std::cin, line)
        
        // Funktion til at trimme <line> så der ikke er trailing spaces og argumenter er sepereret med et enkelt mellemrum
        trim(line);

        if(line.empty())
            continue;

        if(line == "help")
        {
            printCommands();
            continue;
        }

        std::string formatted;

        /*
        formater line til korrekt syntaks
        */

        // Send to server
        if(!send_line(formatted))
        {
            std::cerr << "Failed to send command\n";
            break;
        }

        std::string response;
        if(!recieve_line(response));
        {
            std::cerr << "Connection closed by server\n";
            break;
        }
        
        if(line == "shutdown")
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
        << "SmartLock kommandoer samt syntaks\n"
        << "Kommandoer:\n"
        << "  help                       - Printer denne oversigt over kommandoer\n"
        << "  newDoor <Door name> <accessLevel>   - Tilføj dør\n"
        << "  newUser <Username> <accessLevel>   - Tilføj bruger (kræver NFC-scan)\n"
        << "  rmDoor <Door name>                  - Slet dør\n"
        << "  rmUser <Username>                  - Slet bruger\n"
        << "  getLog                         - Hent log \n"
        << "  shutdown                       - Luk serveren ned\n"
        << "\n";
}