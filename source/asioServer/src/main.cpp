#include "ReaderHandler.h"

int main() {
    ReaderHandler::myIP();

    ReaderHandler reader(9000, 9001, "adminReader");

    ReaderHandler::runLoop();

    return 0;
}
