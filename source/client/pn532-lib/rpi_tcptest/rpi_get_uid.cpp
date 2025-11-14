#include "rpi_client.h"

int main(int argc, char* argv[]) {
    const int portno      = (argc > 1) ? atoi(argv[1]) : 9000;
    const char* server_ip = (argc > 2) ? argv[2] : "172.16.15.3";

    try {
        client rpi_client(portno, server_ip);
        rpi_client.run();
    } catch (std::exception& e) {
        std::cout << "Error:" << e.what() << std::endl;
        return -1;
    }

    return 0;
}
