#pragma once
#include "TcpServer.h"

#include <unordered_map>
#include <fstream>

#include "json.hpp"

enum class ReaderState {
	Idle,
	Active
};

class ReaderHandler {
public:
	ReaderHandler(const int& clientPort, const int& cliPort);
	~ReaderHandler();

	void stop();

	ReaderState getState() const;

	static void runLoop();

private: // Member Functions
	void handleClient(std::shared_ptr<TcpConnection> connection);
	void handleCli(std::shared_ptr<TcpConnection> connection);

	void newDoor(std::shared_ptr<TcpConnection> connection, const std::string&);
	void newUser(std::shared_ptr<TcpConnection> connection, const std::string&);
	void rmDoor(std::shared_ptr<TcpConnection> connection, const std::string&);
	void rmUser(std::shared_ptr<TcpConnection> connection, const std::string&);
	static void to_snake_case(std::string&);

	nlohmann::json getLog() const;

private: // Member Variables
	struct ClientInfo {
		std::string doorName;
		uint8_t accessLevel;
	};
	ReaderState state = ReaderState::Idle;
	static inline bool running = true;

	TcpServer clientServer;
	TcpServer cliServer;

	int readerAccessLevel{3};

	nlohmann::json log;
	std::unordered_map<std::string, int> doors;
	std::unordered_map<std::string, std::pair<std::string, int>> users;
	std::unordered_map<std::unique_ptr<TcpConnection>,ClientInfo> connectedClients;
};
