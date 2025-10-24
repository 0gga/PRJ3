#include "ReaderHandler.h"

int main() {
    ReaderHandler reader(9000, 9001, "adminReader");

    ReaderHandler::runLoop();

    return 0;
}
