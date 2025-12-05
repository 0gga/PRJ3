#include "rpi_client.h"

int main(int argc, char *argv[])
{

    std::cerr << "Arguments: " << argc << std::endl;
    for (auto i = 1; i < argc; i++)
    {
        std::cerr << argv[i] << std::endl;
    }

    std::string doorname = (argc > 1) ? argv[1] : "maindoor";
    int portno = (argc > 2) ? atoi(argv[2]) : 9000;
    const char *server_ip = (argc > 3) ? argv[3] : "172.20.10.9";

    try
    {
        client rpi_client(portno, server_ip, doorname);
        rpi_client.run();
    }
    catch (std::exception &e)
    {
        std::cout << "Error:" << e.what() << std::endl;
        return -1;
    }

    return 0;
}
