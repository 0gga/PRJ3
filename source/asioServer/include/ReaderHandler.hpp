#pragma once
#include "TcpServer.hpp"

#include <unordered_map>
#include <fstream>
#include <shared_mutex>

#include "../logger/csv.hpp"
#include "json.hpp"

enum class ReaderState {
	Idle,
	Active
};

void myIP();

class ReaderHandler {
public:
	explicit ReaderHandler(const int& clientPort, const int& cliPort, const std::string& cliName);
	~ReaderHandler();

	void stop();

	static void runLoop();

	static void myIp();

private: // Member Functions
	/// Do not pass by const reference since pointer copy is trivial.<br>Additional benefit: Avoids any unintended interference with the TcpConnection objects.
	void handleClient(CONNECTION_T connection);
	void handleCli(CONNECTION_T connection);

	void newDoor(CONNECTION_T connection, const std::string&);
	void newUser(CONNECTION_T connection, const std::string&);
	void rmDoor(CONNECTION_T connection, const std::string&);
	void rmUser(CONNECTION_T connection, const std::string&);
	static void to_snake_case(std::string&);

	ReaderState getState() const;

private: // Member Variables
	ReaderState state          = ReaderState::Idle;
	static inline bool running = true;

	TcpServer clientServer;
	TcpServer cliServer;

	CsvLogger allLogger;

	nlohmann::json allLog;
	std::pair<std::string, CONNECTION_T> cliReader; //brug weak_ptr her perchance
	std::unordered_map<std::string, int> doors;
	std::unordered_map<std::string, std::pair<std::string, int>> usersByName;
	std::unordered_map<std::string, std::pair<std::string, int>> usersByUid;

	std::mutex cli_mutex; // for cli admin control
	std::mutex cli_command_mutex; // for command control
	std::shared_mutex rw_mtx; // Use shared_lock for reads and unique_lock for writes.
};
