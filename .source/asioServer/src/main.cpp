#include "ReaderHandler.h"

int main() {
	ReaderHandler reader(9000, 9001);

	ReaderHandler::runLoop();

	return 0;
}
