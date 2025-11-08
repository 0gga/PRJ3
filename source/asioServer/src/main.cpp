#include "ReaderHandler.h"
#include <ogga/print.hpp>
#include <ogga/scopetimer.hpp>

#ifdef Args
int main(int argc, char* argv[]) {
	ReaderHandler reader(argv[1], argv[2], std::to_string(argv[3])); // Use arguments for client- & cliPort parameters

	ogga::scopetimer runTimer("Server ran for", "min");

	ogga::print("Server Successfully Constructed");

	ReaderHandler::runLoop();

	return 0;
}

#else
int main() {
	[[maybe_unused]] ReaderHandler reader(9000, 9001, "adminReader");

	ogga::scopetimer runTimer("Server ran for", "min");

	ogga::print("Server Successfully Constructed");

	ReaderHandler::runLoop();
	return 0;
}
#endif
