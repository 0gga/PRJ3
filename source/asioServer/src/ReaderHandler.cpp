#include "ReaderHandler.hpp"

#include <iostream>
#include <regex>

///
ReaderHandler::ReaderHandler(const int& clientPort, const int& cliPort,
							 const std::string& cliName) : clientServer_(clientPort),
														   cliServer_(cliPort),
														   cliReader_{cliName, nullptr} {
	myIp();
	////////////////////////////// Read config JSON //////////////////////////////
	std::ifstream file("config.json");
	nlohmann::json configJson;

	if (!file.is_open() || file.peek() == std::ifstream::traits_type::eof()) {
		DEBUG_OUT("config.json doesn't exist");
		std::ofstream("config.json") << R"({"users":[],"doors":[]})";
		DEBUG_OUT("Created empty config.json");
	} else {
		try {
			file >> configJson;
		}
		catch (const nlohmann::json::parse_error& e) {
			DEBUG_OUT("Invalid JSON in config.json" + std::string(e.what()));
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};

			std::ofstream("config.json") << R"({"users":[],"doors":[]})";
			DEBUG_OUT("Reset invalid config.json\n");
		}
	}

	if (configJson.contains("doors"))
		for (const auto& door : configJson["doors"]) {
			if (door.contains("name") && door.contains("lvl") &&
				door["name"].is_string() && door["lvl"].is_number_integer())
				doors_[door["name"]] = door["lvl"];
			else
				DEBUG_OUT("Invalid door entry in config.json - skipping one.\n");
		}

	if (configJson.contains("users"))
		for (const auto& user : configJson["users"]) {
			if (user.contains("name") && user.contains("uid") && user.contains("lvl") &&
				user["name"].is_string() && user["uid"].is_string() && user["lvl"].is_number_integer()) {
				const std::string name = user["name"];
				const std::string uid  = user["uid"];
				const int lvl          = user["lvl"];

				usersByName_[name] = {uid, lvl};
				usersByUid_[uid]   = {name, lvl};
			} else
				DEBUG_OUT("Invalid user entry in config.json - skipping one.\n");
		}
	file.close();
	////////////////////////////// Read config JSON //////////////////////////////

	//////////////////////////////// Init Servers ////////////////////////////////
	TcpServer::setThreadCount(1);

	clientServer_.start();
	cliServer_.start();

	clientServer_.onClientConnect([this](const CONNECTION_T& connection) {
		DEBUG_OUT("Client Connected\n");
		handleClient(connection);
	});

	cliServer_.onClientConnect([this](const CONNECTION_T& connection) {
		DEBUG_OUT("CLI Connected\n");
		handleCli(connection);
	});

	cliServer_.onClientDisconnect([this](const CONNECTION_T& connection) {
		std::scoped_lock{cli_mtx};
		if (cliReader_.second == connection) {
			cliReader_.second = nullptr;
			DEBUG_OUT("Dead CLI Cleared\n");
		}
	});

	running_ = true;
	DEBUG_OUT("Servers started and awaiting clients");
	//////////////////////////////// Init Servers ////////////////////////////////
#ifdef DEBUG
	addToConfig("doors", "door1", 1);
	addToConfig("doors", "door2", 2);
	addToConfig("doors", "door3", 3);
	addToConfig("doors", "door4", 4);
	addToConfig("doors", "door5", 5);

	addToConfig("users", "a", 1, "1a");
	addToConfig("users", "b", 2, "2b");
	addToConfig("users", "c", 3, "3c");
	addToConfig("users", "d", 4, "4d");
	addToConfig("users", "e", 5, "5e");
#endif
}

/// Destructor\n
/// Calls stop()
ReaderHandler::~ReaderHandler() {
	stop();
}

/// Stop all servers.\n
/// Called in DTOR when main goes out of scope.\n
/// Also called if cli calls "shutdown".
/// @returns void
void ReaderHandler::stop() {
	running_ = false;
	clientServer_.stop();
	cliServer_.stop();
	DEBUG_OUT("Servers Shutting Down");
}

/// Runtime loop.\n Necessary to avoid termination while callbacks await.
/// @returns void
void ReaderHandler::runLoop() {
	while (running_)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

/// Outputs server IPV4.
/// /// @returns void
void ReaderHandler::myIp() {
#ifdef _WIN32
	static bool wsa_initialized = false;
	if (!wsa_initialized) {
		WSADATA wsa{};
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
			throw std::runtime_error("WSAStartup failed");
		}
		wsa_initialized = true;
	}
#endif

	// 1) Create UDP socket
	int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		throw std::runtime_error("socket() failed");
	}

	// 2) "Connect" to some external IPv4 (no packets need to be sent)
	sockaddr_in remote{};
	remote.sin_family = AF_INET;
	remote.sin_port   = htons(53); // arbitrary (DNS)
	if (inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr) != 1) {
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		throw std::runtime_error("inet_pton failed");
	}

	if (connect(sock, reinterpret_cast<sockaddr*>(&remote), sizeof(remote)) < 0) {
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		throw std::runtime_error("connect() failed");
	}

	// 3) Ask the OS which local address was used
	sockaddr_in local{};
	socklen_t len = sizeof(local);
	if (getsockname(sock, reinterpret_cast<sockaddr*>(&local), &len) < 0) {
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		throw std::runtime_error("getsockname() failed");
	}

#ifdef _WIN32
	closesocket(sock);
#else
	close(sock);
#endif

	// 4) Convert to string
	char buf[INET_ADDRSTRLEN]{};
	if (!inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf))) {
		throw std::runtime_error("inet_ntop failed");
	}
	DEBUG_OUT(std::string_view(buf));
}

void ReaderHandler::onDeadConnection(CONNECTION_T dead) {
	std::scoped_lock{cli_mtx};
	if (cliReader_.second == dead)
		cliReader_.second = nullptr;
}

/// Handles Client IO.\n
/// Is automatically called via lambda callback in CTOR whenever a new TCP Connection is established on the clientServer_.<br>Recalls itself after each pass.
/// @param connection ptr to the relative TcpConnection object. This is established and passed in the CTOR callback.
/// @returns void
void ReaderHandler::handleClient(CONNECTION_T connection) const {
	connection->read<std::string>([this, connection](const std::string& pkg) {
		const size_t seperator = pkg.find(':');
		if (seperator == std::string::npos || seperator == 0 || seperator == pkg.size() - 1) {
			DEBUG_OUT("Invalid Client Package Syntax - Connection Closed");
			connection->write<std::string>("Invalid Client Package Syntax - Connection Closed");
			connection->close();
			return;
		}

		{
			const std::string name = pkg.substr(0, seperator);
			const std::string uid  = pkg.substr(seperator + 1);

			std::shared_lock rw_lock(rw_mtx); // Shared lock since it exclusively reads
			const auto door = doors_.find(name);
			if (door == doors_.end()) {
				DEBUG_OUT("Unknown Door");
				handleClient(connection);
				return;
			}

			const auto user       = usersByUid_.find(uid);
			const bool authorized = (user != usersByUid_.end() && user->second.second <= door->second);
			DEBUG_OUT(
					  (user == usersByUid_.end())
					  ? "Unknown UID"
					  : ((authorized ? "Approved access to " + user->second.first
							  : "Denied access to " + user->second.first) + '(' + std::to_string(user->second.second) + ')'
						  + " at " + door->first + '(' + std::to_string(door->second) + ')')
					 );
			connection->write<std::string>(authorized ? "approved" : "denied");
			try {
				if (user != usersByUid_.end())
					log_.addLog(door->first, user->second.first, user->first, authorized ? "approved" : "denied");
				else
					log_.addLog(door->first, "unknown", "unknown", authorized ? "approved" : "denied");
			}
			catch (std::exception& e) {
				DEBUG_OUT(e.what());
			}
		}
		handleClient(connection);
	});
}

/// Handles CLI IO.\n
/// Is automatically called via lambda callback in CTOR whenever a new TCP Connection is established on the cliServer_. Recalls itself after each pass.
/// @param connection ptr to the relative TcpConnection object. This is established and passed in the CTOR callback.
/// @returns void
void ReaderHandler::handleCli(CONNECTION_T connection) {
	{
		// Phase 1: Check if admin is connected and if not facilitate new connection.
		std::scoped_lock{cli_mtx};
		if (cliReader_.second == connection) {
			connection->write<std::string>("CLI is ready");
		} else if (!cliReader_.second) {
			connection->write<std::string>("Input CLI Identification");
		} else {
			connection->write<std::string>("Another Admin is connected");
			connection->close();
			return;
		}
	}

	connection->read<std::string>([this, connection](const std::string& pkg) {
		// Phase 2: Handle admin duplicates the connection registered as admin
		bool recursionFlag = false;
		{
			std::scoped_lock{cli_mtx};
			if (cliReader_.second != connection) {
				if (pkg.rfind(cliReader_.first, 0) == 0) {
					cliReader_.second = connection;
					DEBUG_OUT("Admin Verified");
				} else if (pkg.rfind(cliReader_.first, 0) != 0) {
					connection->write<std::string>("Incorrect CLI Identification");
				}
				recursionFlag = true;
			}
		}
		if (recursionFlag) {
			handleCli(connection);
			return;
		}

		// Added lambda for readability
		auto checkSyntax = [&](const std::string& name) {
			if (name == "-1") {
				connection->write<std::string>("Operation failed - Incorrect CLI syntax");
				handleCli(connection);
				return false;
			}
			return true;
		};

		// Phase 3: Handle cmdlets
		if (pkg.rfind("newUser", 0) == 0) {
			const auto [name, _, lvl] = parseSyntax(pkg, newUser_);
			if (checkSyntax(name))
				newUser(connection, name, lvl);
		} else if (pkg.rfind("newDoor", 0) == 0) {
			const auto [name, _, lvl] = parseSyntax(pkg, newDoor_);
			if (checkSyntax(name))
				newDoor(connection, name, lvl);
		} else if (pkg.rfind("rmUser", 0) == 0) {
			const auto [name, _, lvl] = parseSyntax(pkg, rmUser_);
			if (checkSyntax(name))
				rmUser(connection, name);
		} else if (pkg.rfind("rmDoor", 0) == 0) {
			const auto [name, _, lvl] = parseSyntax(pkg, rmDoor_);
			if (checkSyntax(name))
				rmDoor(connection, name);
		} else if (pkg.rfind("mvUser", 0) == 0) {
			const auto [oldName, newName, lvl] = parseSyntax(pkg, mvUser_);
			if (checkSyntax(oldName))
				mvUser(connection, oldName, newName, lvl);
		} else if (pkg.rfind("mvDoor", 0) == 0) {
			const auto [oldName, newName , lvl] = parseSyntax(pkg, mvDoor_);
			if (checkSyntax(oldName))
				mvDoor(connection, oldName, newName, lvl);
		} else if (pkg.rfind("getSystemLog", 0) == 0) {
			const auto [name, _, __] = parseSyntax(pkg, systemLog_);
			if (checkSyntax(name)) {
				connection->writeFile(getSystemLog(name));
				handleCli(connection);
			}
		} else if (pkg.rfind("getUserLog", 0) == 0) {
			const auto [name, _, __] = parseSyntax(pkg, userLog_);
			if (checkSyntax(name)) {
				connection->writeFile(getUserLog(name));
				handleCli(connection);
			}
		} else if (pkg.rfind("getDoorLog", 0) == 0) {
			const auto [name, _, __] = parseSyntax(pkg, doorLog_);
			if (checkSyntax(name)) {
				connection->writeFile(getDoorLog(name));
				handleCli(connection);
			}
		} else if (pkg == "exit") {
			connection->write<std::string>("Closing Connection...");
			if (connection == cliReader_.second)
				cliReader_.second = nullptr;
			connection->close();
		} else if (pkg == "shutdown") {
			connection->write<std::string>("Shutting Down...");
			stop();
		} else {
			connection->write<std::string>("Unknown Command");
			handleCli(connection);
		}
	});
}

/// Add new user function.
/// @param connection ptr to the relative TcpConnection object.
/// @param name string representation of the user to be added i.e. "john_doe".
/// @param lvl uint8_t representation of the access level for the user to be added i.e. "1".
void ReaderHandler::newUser(CONNECTION_T connection, const std::string& name, const uint8_t lvl) {
	connection->write<std::string>("Awaiting card read");
	connection->read<std::string>([this, name, lvl, connection](const std::string& uid) {
		const std::string confirmMsg("Are you sure you want to add user:\n"
									 "UID: " + uid + "\n" +
									 "Name: " + name + "\n" +
									 "Access Level: " + std::to_string(lvl));
		connection->write<std::string>(confirmMsg);

		connection->read<std::string>([this, name, lvl, uid, connection](const std::string& status) {
			if (status == "denied" || status != "approved") {
				connection->write<std::string>("Did not add user");
				handleCli(connection);
				return;
			}

			if (addToConfig("users", name, lvl, uid))
				connection->write<std::string>("User added successfully");
			else
				connection->write<std::string>("Failed to add user");
			handleCli(connection);
		});
	});
}

/// Add new door function.
/// /// @param connection ptr to the relative TcpConnection object.
/// @param name string representation of the door to be added i.e. "front_entrance".
/// @param lvl uint8_t representation of the access level for the door to be added i.e. "1".
void ReaderHandler::newDoor(CONNECTION_T connection, const std::string& name, const uint8_t lvl) {
	const std::string confirmMsg("Are you sure you want to add door:\n"
								 "Name: " + name + "\n" +
								 "Access Level: " + std::to_string(lvl));
	connection->write<std::string>(confirmMsg);
	connection->read<std::string>([this, name, lvl, connection](const std::string& status) {
		if (status == "denied" || status != "approved") {
			connection->write<std::string>("Did not add door");
			handleCli(connection);
			return;
		}

		if (addToConfig("doors", name, lvl))
			connection->write<std::string>("Door added successfully");
		else
			connection->write<std::string>("Failed to add door");
		handleCli(connection);
	});
}

/// Remove user function.
/// @param connection ptr to the relative TcpConnection object.
/// @param name string representation of the user to be removed i.e. "john_doe".
void ReaderHandler::rmUser(CONNECTION_T connection, const std::string& name) {
	auto user = usersByName_.find(name);
	if (user == usersByName_.end()) {
		connection->write<std::string>("User could not be found");
		handleCli(connection);
		return;
	}
	const std::string confirmMsg("Are you sure you want to remove user:\n"
								 "UID: " + user->second.first + "\n"
								 "Name: " + name + "\n"
								 "Access Level: " + std::to_string(user->second.second));
	connection->write<std::string>(confirmMsg);
	connection->read<std::string>([this, name, connection](const std::string& status) {
		if (status == "denied" || status != "approved") {
			connection->write<std::string>("Cancelled remove operation");
			handleCli(connection);
			return;
		}

		if (removeFromConfig("users", name))
			connection->write<std::string>("User removed successfully");
		else
			connection->write<std::string>("Failed to remove user");
		handleCli(connection);
	});
}

/// Remove door function.
/// @param connection ptr to the relative TcpConnection object.
/// @param name string representation of the door to be removed i.e. "front_entrance".
void ReaderHandler::rmDoor(CONNECTION_T connection, const std::string& name) {
	const auto door = doors_.find(name);
	if (door == doors_.end()) {
		connection->write<std::string>("Door could not be found");
		handleCli(connection);
		return;
	}
	const std::string confirmMsg("Are you sure you want to remove door:\n"
								 "Name: " + name + "\n" +
								 "Access Level: " + std::to_string(door->second));
	connection->write<std::string>(confirmMsg);
	connection->read<std::string>([this, name, connection](const std::string& status) {
		if (status == "denied" || status != "approved") {
			connection->write<std::string>("Cancelled remove operation");
			handleCli(connection);
			return;
		}

		if (removeFromConfig("doors", name))
			connection->write<std::string>("Door removed successfully");
		else
			connection->write<std::string>("Failed to remove door");
		handleCli(connection);
	});
}

void ReaderHandler::mvUser(CONNECTION_T connection, const std::string& oldName, const std::string& newName,
						   uint8_t lvl) {
	auto user = usersByName_.find(oldName);
	if (user == usersByName_.end()) {
		connection->write<std::string>("User could not be found");
		handleCli(connection);
		return;
	}
	if (lvl == 0)
		lvl = user->second.second;

	const std::string confirmMsg("Are you sure you want to edit user:\n"
								 "UID: " + user->second.first + "\n"
								 "Name: " + oldName + " -> " + newName + "\n"
								 "Access Level: " + std::to_string(user->second.second) + " -> " + std::to_string(lvl) +
								 "\n"
								);
	std::string uid = user->second.first;
	connection->write<std::string>(confirmMsg);
	connection->read<std::string>([this, uid, oldName, newName, lvl, connection](const std::string& status) {
		if (status == "denied" || status != "approved") {
			connection->write<std::string>("Cancelled edit operation");
			handleCli(connection);
			return;
		}

		if (removeFromConfig("users", oldName) && addToConfig("users", newName, lvl, uid))
			connection->write<std::string>("User edited successfully");
		else
			connection->write<std::string>("Failed to edit user, data may be corrupted");
		handleCli(connection);
	});
}


void ReaderHandler::mvDoor(CONNECTION_T connection, const std::string& oldName, const std::string& newName,
						   uint8_t lvl) {
	const auto door = doors_.find(oldName);
	if (door == doors_.end()) {
		connection->write<std::string>("Door could not be found");
		handleCli(connection);
		return;
	}

	if (lvl == 0)
		lvl = door->second;

	const std::string confirmMsg("Are you sure you want to edit door:\n"
								 "Name: " + oldName + " -> " + newName + "\n"
								 "Access Level: " + std::to_string(door->second) + " -> " + std::to_string(lvl) + "\n"
								);
	connection->write<std::string>(confirmMsg);
	connection->read<std::string>([this, oldName, newName, lvl, connection](const std::string& status) {
		if (status == "denied" || status != "approved") {
			connection->write<std::string>("Cancelled edit operation");
			handleCli(connection);
			return;
		}

		if (removeFromConfig("doors", oldName) && addToConfig("doors", newName, lvl))
			connection->write<std::string>("Door edited successfully");
		else
			connection->write<std::string>("Failed to edit door, data may be corrupted");
		handleCli(connection);
	});
}

std::string ReaderHandler::getSystemLog(const std::string& date) {
	return log_.getLogByDate(date);
}

std::string ReaderHandler::getUserLog(const std::string& name) {
	return log_.getLogByName(name);
}

std::string ReaderHandler::getDoorLog(const std::string& name) {
	return log_.getLogByDoor(name);
}


bool ReaderHandler::addToConfig(const std::string& type, const std::string& name, uint8_t lvl, const std::string& uid) {
	// Assert type is correct. Cannot use compile-time asserts on string comparisons, maybe use const char* instead in the future.
	if (type != "doors" && type != "users") {
		DEBUG_OUT("Type must be either 'doors' or 'users'");
		return false;
	}
	if (type == "users" && usersByUid_.contains(uid)) {
		DEBUG_OUT("UID already exists");
		return false;
	}
	if (type == "doors" && doors_.contains(name)) { // First condition is purely for readability
		DEBUG_OUT("Door already exists");
		return false;
	}

	// Lock before editing any runtime memory/config.json
	std::scoped_lock{rw_mtx};

	// config.json error syntax checks
	nlohmann::json configJson;
	assertConfig(configJson);

	if (!configJson.contains(type))
		configJson[type] = nlohmann::json::array();

	nlohmann::json addition;

	if (type == "users") {
		usersByName_[name] = {uid, lvl};
		usersByUid_[uid]   = {name, lvl};
		addition           = {
			{"name", name},
			{"uid", uid},
			{"lvl", lvl}
		};
	} else {
		doors_[name] = lvl;
		addition     = {
			{"name", name},
			{"lvl", lvl}
		};
	}

	configJson[type].push_back(addition);

	// Save changes to config.json. Using temporary file and rename for safer patch appliance.
	{
		std::ofstream out{"config_tmp.json"};
		out << configJson.dump(4);
	}
	std::filesystem::rename("config_tmp.json", "config.json");

	return true;
}

bool ReaderHandler::removeFromConfig(const std::string& type, const std::string& name) {
	// Assert type is correct. Cannot use compile-time asserts on string comparisons, maybe use const char* instead in the future.
	if (type != "doors" && type != "users") {
		DEBUG_OUT("Type must be either 'doors' or 'users'");
		return false;
	}

	// Lock before editing any runtime memory/config.json
	std::scoped_lock{rw_mtx};
	// Remove from memory
	if (type == "users") {
		const auto& user = usersByName_.find(name);
		usersByUid_.erase(user->second.first); // remove by uid
		usersByName_.erase(user);
	} else if (type == "doors") {
		const auto& door = doors_.find(name);
		doors_.erase(door);
	}

	// config.json error syntax checks
	nlohmann::json configJson;
	assertConfig(configJson);

	// Rewrite related config.json segment
	if (configJson.contains(type)) {
		auto& revisedJson = configJson[type];
		for (auto it = revisedJson.begin(); it != revisedJson.end(); ++it) {
			if (it->contains("name") && (*it)["name"] == name) {
				revisedJson.erase(it);
				break;
			}
		}
	}

	// Save changes to config.json. Using temporary file and rename for safer patch appliance.
	{
		std::ofstream out{"config_tmp.json"};
		out << configJson.dump(4);
	}
	std::filesystem::rename("config_tmp.json", "config.json");

	return true;
}

void ReaderHandler::assertConfig(nlohmann::json& configJson) {
	try {
		std::ifstream in("config.json");
		if (in && in.peek() != std::ifstream::traits_type::eof())
			in >> configJson;
		else
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	}
	catch (const nlohmann::json::parse_error& e) {
		DEBUG_OUT("Invalid JSON @ config.json, attempting clean overwrite - " + std::string(e.what()));
		configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	}
}

ReaderHandler::CmdArgs ReaderHandler::parseSyntax(const std::string& data, Command type) {
	std::smatch match;
	CmdArgs error;
	uint8_t lvl{};

	switch (type) {
		case newUser_:
			static const std::regex newUserSyntax(R"(^newUser\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
			if (!std::regex_match(data, match, newUserSyntax))
				return error;
			lvl = std::stoul(match[2].str());
			break;

		case newDoor_:
			static const std::regex newDoorSyntax(R"(^newDoor\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
			if (!std::regex_match(data, match, newDoorSyntax))
				return error;
			lvl = std::stoul(match[2].str());
			break;

		case rmUser_:
			static const std::regex rmUserSyntax(R"(^rmUser\s+([A-Za-z0-9_]+)$)");
			if (!std::regex_match(data, match, rmUserSyntax))
				return error;
			break;

		case rmDoor_:
			static const std::regex rmDoorSyntax(R"(^rmDoor\s+([A-Za-z0-9_]+)$)");
			if (!std::regex_match(data, match, rmDoorSyntax))
				return error;
			break;

		case mvUser_:
			static const std::regex mvUserSyntax1(R"(^mvUser\s+([A-Za-z0-9_]+)\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
			static const std::regex mvUserSyntax2(R"(^mvUser\s+([A-Za-z0-9_]+)\s+([A-Za-z0-9_]+)$)");
			if (std::regex_match(data, match, mvUserSyntax1))
				lvl = std::stoul(match[3].str());
			else if (!std::regex_match(data, match, mvUserSyntax2))
				return error;
			break;

		case mvDoor_:
			static const std::regex mvDoorSyntax1(R"(^mvDoor\s+([A-Za-z0-9_]+)\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
			static const std::regex mvDoorSyntax2(R"(^mvDoor\s+([A-Za-z0-9_]+)\s+([A-Za-z0-9_]+)$)");
			if (std::regex_match(data, match, mvDoorSyntax1))
				lvl = std::stoul(match[3].str());
			else if (!std::regex_match(data, match, mvDoorSyntax2))
				return error;
			break;
		case systemLog_:
			static const std::regex SystemLogSyntax(R"(^getSystemLog\s+([A-Za-z0-9_]+)\s+([A-Za-z0-9_]+)$)");
			if (!std::regex_match(data, match, SystemLogSyntax))
				return error;
			break;
		case userLog_:
			static const std::regex userLogSyntax(R"(^getUserLog\s+([A-Za-z0-9_]+)$)");
			if (!std::regex_match(data, match, userLogSyntax))
				return error;
			break;
		case doorLog_:
			static const std::regex doorLogSyntax(R"(^getDoorLog\s+([A-Za-z0-9_]+)$)");
			if (!std::regex_match(data, match, doorLogSyntax))
				return error;
			break;

		default:
			return error;
	}

	uint8_t count       = match.size() - 1;
	std::string oldName = count >= 1 ? match[1].str() : "-1";
	std::string newName = count >= 2 ? match[2].str() : "-1";

	if (match.size() > 2)
		to_snake_case(oldName, newName);
	else
		to_snake_case(oldName);

	return CmdArgs{oldName, newName, lvl};
}

/// Helper function for converting std::string to snake_case.\n
/// Takes std::string by reference.
/// @param args undefined parameter pack. Will assert type is std::string&
/// @returns void
template<typename... Args>
void ReaderHandler::to_snake_case(Args&... args) {
	static_assert((std::is_same_v<Args, std::string> && ...), "Arguments must be of type std::string&");
	auto convertOne = [](std::string& input) {
		std::string result;
		result.reserve(input.size());
		/// Albeit reserving only input.size(), more usually get's allocated.
        /// If larger than input.size() it will just reallocate which is negligible regardless, considering the string sizes we'll be handling.

		bool prevLower{false};
		for (const unsigned char c : input) {
			if (std::isupper(c)) {
				if (prevLower)
					result += '_';
				result += static_cast<char>(std::tolower(c));
				prevLower = false;
			} else {
				result += static_cast<char>(c);
				prevLower = true;
			}
		}
		input = std::move(result);
	};
	(convertOne(args), ...);
}
