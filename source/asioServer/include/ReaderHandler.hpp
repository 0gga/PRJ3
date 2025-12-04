#pragma once
#include "TcpServer.hpp"

#include <unordered_map>
#include <shared_mutex>

#include "json.hpp"

enum class ReaderState {
	Idle, Active
};

void myIP();

class ReaderHandler {
public:
	explicit ReaderHandler(const int& clientPort, const int& cliPort, const std::string& cliName);
	~ReaderHandler();

	void stop();

	static void runLoop();

	static void myIp();

	void onDeadConnection(CONNECTION_T dead);

private: // Member Functions
	/// Do not pass by const reference since pointer copy is trivial.<br>Additional benefit: Avoids any unintended interference with the TcpConnection objects.
	void handleClient(CONNECTION_T connection);
	void handleCli(CONNECTION_T connection);

	enum command {
		newUser_,
		newDoor_,
		rmUser_,
		rmDoor_,
		mvUser_,
		mvDoor_
	};

	struct cmdArgs {
		std::string oldName_{"-1"};
		std::string newName_{"-1"};
		uint8_t accessLevel_{};
	};


	void newUser(CONNECTION_T connection, const std::string&, uint8_t);
	void newDoor(CONNECTION_T connection, const std::string&, uint8_t);
	void rmUser(CONNECTION_T connection, const std::string&);
	void rmDoor(CONNECTION_T connection, const std::string&);
	void mvUser(CONNECTION_T connection, const std::string&, const std::string&, uint8_t);
	void mvDoor(CONNECTION_T connection, const std::string&, const std::string&, uint8_t);

	//std::string getLog();

	bool addToConfig(const std::string&, const std::string&, uint8_t, const std::string& = "");
	bool removeFromConfig(const std::string&, const std::string&);
	void assertConfig(nlohmann::json&);
	cmdArgs parseSyntax(const std::string& pkg, command type);

	template<typename... Args>
	void to_snake_case(Args&... args);


	ReaderState getState() const;

private: // Member Variables
	ReaderState state          = ReaderState::Idle;
	static inline bool running = true;

	TcpServer clientServer;
	TcpServer cliServer;

	std::pair<std::string, CONNECTION_T> cliReader; //brug weak_ptr her perchance
	std::unordered_map<std::string, int> doors;
	std::unordered_map<std::string, std::pair<std::string, int>> usersByName;
	std::unordered_map<std::string, std::pair<std::string, int>> usersByUid;

	std::mutex cli_mtx;       // for cli admin control
	std::shared_mutex rw_mtx; // Use shared_lock for json reads and unique_lock for json writes.
};
