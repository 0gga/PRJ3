#include "ReaderHandler.h"

//  int main(int argc, char* argv[]) {
//  	ReaderHandler reader(argv[1], argv[2], std::to_string(argv[3])); // Use arguments for client- & cliPort parameters
//
//  	ReaderHandler::runLoop();
//
//  	return 0;
//  }

int main() {
    [[maybe_unused]] ReaderHandler reader(9000, 9001, "adminReader");

    ReaderHandler::runLoop();

    return 0;
}
