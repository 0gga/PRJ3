#include "ReaderHandler.hpp"
#include <ogga/print.hpp>
#include <ogga/scopetimer.hpp>
#include <ogga/csvlog.hpp>

/// CMake options: -DARGS=1 to use args
int main(int argc, char* argv[]) {
#ifdef ARGS
	[[maybe_unused]] ReaderHandler reader(*argv[1], *argv[2], argv[3]); // Use arguments for client- & cliPort parameters
#else
	[[maybe_unused]] ReaderHandler reader(9000, 9001, "adminReader");
#endif

	[[maybe_unused]] ogga::scopetimer runTimer("Server Uptime", "ns");
	ogga::print("Server Successfully Constructed");

	ReaderHandler::runLoop();
	return 0;
}
