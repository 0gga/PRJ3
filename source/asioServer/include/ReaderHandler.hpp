#pragma once
#include "TcpServer.hpp"

#include <unordered_map>
#include <shared_mutex>

#include "json.hpp"

#include "csv.hpp"

class ReaderHandler {
public:
	explicit ReaderHandler(const int& clientPort, const int& cliPort, const std::string& cliName);
	~ReaderHandler();

	static void runLoop();

private: // Member Functions
	void stop();
	/// Do not pass by const reference since pointer copy is trivial.<br>Additional benefit: Avoids any unintended interference with the TcpConnection objects.
	void handleClient(CONNECTION_T connection) const;
	void handleCli(CONNECTION_T connection);

	static void myIp();

	void onDeadConnection(CONNECTION_T dead);

	void newUser(CONNECTION_T connection, const std::string&, uint8_t);
	void newDoor(CONNECTION_T connection, const std::string&, uint8_t);
	void rmUser(CONNECTION_T connection, const std::string&);
	void rmDoor(CONNECTION_T connection, const std::string&);
	void mvUser(CONNECTION_T connection, const std::string&, const std::string&, uint8_t);
	void mvDoor(CONNECTION_T connection, const std::string&, const std::string&, uint8_t);

	std::string getSystemLog(const std::string& date);
	std::string getUserLog(const std::string& name);
	std::string getDoorLog(const std::string& name);

	static void assertConfig(nlohmann::json&);
	bool addToConfig(const std::string&, const std::string&, uint8_t, const std::string& = "");
	bool removeFromConfig(const std::string&, const std::string&);

	struct CmdArgs;
	enum Command : int; // Standard enum type. Necessary for forward declaration
	CmdArgs parseSyntax(const std::string& pkg, Command type);
	static std::string getConfigPath();

	template<typename... Args>
	static void to_snake_case(Args&... args);

private: // Member Variables
	enum Command : int {
		newUser_,
		newDoor_,
		rmUser_,
		rmDoor_,
		mvUser_,
		mvDoor_,
		systemLog_,
		userLog_,
		doorLog_
	};

	struct CmdArgs {
		std::string oldName_{"-1"};
		std::string newName_{"-1"};
		uint8_t accessLevel_{};
	};

	static inline bool running_ = true;

	TcpServer clientServer_;
	TcpServer cliServer_;

	CsvLogger log_;
	std::pair<std::string, CONNECTION_T> cliReader_;
	std::unordered_map<std::string, int> doors_;
	std::unordered_map<std::string, std::pair<std::string, int>> usersByName_;
	std::unordered_map<std::string, std::pair<std::string, int>> usersByUid_;

	mutable std::mutex cli_mtx;
	mutable std::shared_mutex rw_mtx; // Use shared_lock for json reads and unique_lock for json writes.
};
