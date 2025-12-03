#include "ReaderHandler.hpp"
#include <ogga/print.hpp>
#include <ogga/scopetimer.hpp>

/// <b>CMake options:</b>\n
/// -DDEBUG\n
/// -DARGS
int main(int argc, char* argv[]) {
#ifdef ARGS
	[[maybe_unused]] ReaderHandler reader(*argv[1], *argv[2], argv[3]); // Use arguments for client- & cliPort parameters
#else
	[[maybe_unused]] ReaderHandler reader(9000, 9001, "admin");
#endif

#ifdef DEBUG
	[[maybe_unused]] ogga::scopetimer runTimer("Server Uptime", "min");
	ogga::print("Server Successfully Constructed");
#endif

	ReaderHandler::runLoop();
	return 0;
}
