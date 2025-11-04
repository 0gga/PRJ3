#include "ReaderHandler.h"

#include <iostream>
#include <regex>

ReaderHandler::ReaderHandler(const int& clientPort, const int& cliPort, const std::string& cliReader)
: clientServer(clientPort),
  cliServer(cliPort),
  cliReader{cliReader, nullptr} {
	myIp();
	////////////////////////////// Read config JSON //////////////////////////////
	std::ifstream file("config.json");
	nlohmann::json configJson;

	if (!file.is_open() || file.peek() == std::ifstream::traits_type::eof()) {
		std::cout << "config.json doesn't exist" << std::endl;
		std::ofstream("config.json") << R"({"users":[],"doors":[]})";
		std::cout << "Created empty config.json" << std::endl;
	} else {
		try {
			file >> configJson;
		} catch (const nlohmann::json::parse_error& e) {
			std::cout << "Invalid JSON in config.json" << e.what() << std::endl;
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};

			std::ofstream("config.json") << R"({"users":[],"doors":[]})";
			std::cout << "Reset invalid config.json\n";
		}
	}

	if (configJson.contains("doors"))
		for (const auto& door : configJson["doors"]) {
			if (door.contains("name") && door.contains("accessLevel") &&
				door["name"].is_string() && door["accessLevel"].is_number_integer())
				doors[door["name"]] = door["accessLevel"];
			else
				std::cout << "Invalid door entry in config.json - skipping one.\n";
		}

	if (configJson.contains("users"))
		for (const auto& user : configJson["users"]) {
			if (user.contains("name") && user.contains("uid") && user.contains("accessLevel") &&
				user["name"].is_string() && user["uid"].is_string() && user["accessLevel"].is_number_integer()) {
				const std::string name = user["name"];
				const std::string uid  = user["uid"];
				const int accessLevel  = user["accessLevel"];

				usersByName[name] = {uid, accessLevel};
				usersByUid[uid]   = {name, accessLevel};
			} else
				std::cout << "Invalid user entry in config.json - skipping one.\n";
		}
	////////////////////////////// Read config JSON //////////////////////////////

	//////////////////////////////// Init Servers ////////////////////////////////
	clientServer.onClientConnect([this](const CONNECTION& connection) {
		std::cout << "Client Connected\n";
		handleClient(connection);
	});

	cliServer.onClientConnect([this](const CONNECTION& connection) {
		std::cout << "CLI Connected\n";
		handleCli(connection);
	});

	TcpServer::setThreadCount(4);

	clientServer.start();
	cliServer.start();

	running = true;
	std::cout << "Servers started and awaiting clients" << std::endl;
	//////////////////////////////// Init Servers ////////////////////////////////
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
	state = ReaderState::Idle;
	clientServer.stop();
	cliServer.stop();
	std::cout << "Servers Shutting Down" << std::endl;
}

ReaderState ReaderHandler::getState() const {
	return state;
}

/// Runtime loop.\n Necessary to avoid termination while callbacks await.
/// @returns void
void ReaderHandler::runLoop() {
	while (running)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

/// Outputs server IPV4.
/// @returns void
void ReaderHandler::myIp() {
	boost::asio::io_context io;
	boost::asio::ip::tcp::resolver resolver(io);
	auto results = resolver.resolve(boost::asio::ip::host_name(), "");
	std::string ip_address;
	for (const auto& e : results) {
		auto addr = e.endpoint().address();
		if (addr.is_v4() && !addr.is_loopback()) {
			ip_address = addr.to_string();
			break;
		}
	}
	std::cout << ip_address << std::endl;
}

/// Handles Client IO.\n
/// Is automatically called via lambda callback in CTOR whenever a new TCP Connection is established on the clientServer. Recalls itself after each pass.
/// @param connection shared_ptr to the current TCP client connection. This is established and passed in the CTOR callback.
/// @returns void
void ReaderHandler::handleClient(const CONNECTION& connection) {
	state = ReaderState::Active;

	connection->read<std::string>([this, connection](const std::string& pkg) {
		const size_t seperator = pkg.find(':');
		if (seperator == std::string::npos || seperator == 0 || seperator == pkg.size() - 1) {
			std::cout << "Invalid Client Package Syntax" << std::endl;
			connection->write<std::string>("Invalid Client Package Syntax - Connection Closed");
			connection->close();
			return;
		}

		const std::string name = pkg.substr(0, seperator);
		const std::string uid  = pkg.substr(seperator + 1);

		{
			std::shared_lock rw_lock(rw_mtx);
			const auto door = doors.find(name);
			if (door == doors.end()) {
				connection->write<std::string>("Unknown Door");
				handleClient(connection);
				return;
			}

			const auto user       = usersByUid.find(uid);
			const bool authorized = (user != usersByUid.end() && user->second.second >= door->second);
			connection->write<std::string>(authorized ? "Approved" : "Denied");
		}

		handleClient(connection);
	});
	state = ReaderState::Idle;
}

/// Handles CLI IO.\n
/// Is automatically called via lambda callback in CTOR whenever a new TCP Connection is established on the cliServer. Recalls itself after each pass.
/// @param connection shared_ptr to the current TCP cli connection. This is established and passed in the CTOR callback.
/// @returns void
void ReaderHandler::handleCli(const CONNECTION& connection) {
	state = ReaderState::Active;
	connection->read<std::string>([this, connection](const std::string& pkg) {
		if (cliReader.first == pkg) {
			cliReader.second = connection;
			connection->write<std::string>("cliReader is ready");
		}
		if (!connection) {
			connection->write<std::string>("Awaiting cliReader connection...");
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		} else if (pkg.rfind("newDoor", 0) == 0) {
			newDoor(connection, pkg);
		} else if (pkg.rfind("newUser", 0) == 0) {
			newUser(connection, pkg);
		} else if (pkg.rfind("rmDoor", 0) == 0) {
			rmDoor(connection, pkg);
		} else if (pkg.rfind("rmUser", 0) == 0) {
			rmUser(connection, pkg);
		} else if (pkg == "getLog") {
			connection->write<nlohmann::json>(getLog());
		} else if (pkg == "shutdown") {
			connection->write<std::string>("Shutting Down...");
			running = false;
			clientServer.stop();
			cliServer.stop();
			return;
		} else {
			connection->write<std::string>("Unknown Command");
		}
		handleCli(connection);
	});
	state = ReaderState::Idle;
}

/// Add new door function.
/// @param connection shared_ptr to the current TCP client connection.
/// @param doorData String representation of the door to be added i.e. "door1 1".
void ReaderHandler::newDoor(const CONNECTION& connection, const std::string& doorData) {
	// Parse CLI command for correct syntax
	static const std::regex cliSyntax(R"(^newDoor\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
	std::smatch match;
	if (!std::regex_match(doorData, match, cliSyntax)) {
		connection->write<std::string>("Failed to add new door - Incorrect CLI syntax");
		return;
	}

	std::string name = match[1].str();
	to_snake_case(name);

	uint8_t accessLevel = std::stoul(match[2].str());

	std::unique_lock rw_lock(rw_mtx);
	nlohmann::json configJson;
	try {
		std::ifstream in("config.json");
		if (in && in.peek() != std::ifstream::traits_type::eof())
			in >> configJson;
		else
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	} catch (const nlohmann::json::parse_error& e) {
		std::cout << "Invalid JSON @ config.json" << e.what() << std::endl;
		configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	}
	if (!configJson.contains("doors"))
		configJson["doors"] = nlohmann::json::array();

	doors[name] = accessLevel;
	configJson["doors"].push_back({
									  {"name", name},
									  {"accessLevel", accessLevel}
								  });

	std::ofstream out{"config_tmp.json"};
	out << configJson.dump(4);
	std::filesystem::rename("config_tmp.json", "config.json");
	connection->write<std::string>("Door Added Successfully");
}

/// Add new user function.
/// @param connection shared_ptr to the current TCP client connection.
/// @param userData String representation of the user to be added i.e. "john_doe 1".
void ReaderHandler::newUser(const CONNECTION& connection, const std::string& userData) {
	// Parse CLI command for correct syntax
	static const std::regex cliSyntax(R"(^newUser\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
	std::smatch match;
	if (!std::regex_match(userData, match, cliSyntax)) {
		connection->write<std::string>("Failed to add new user - Incorrect CLI syntax");
		return;
	}

	std::string name = match[1].str();
	to_snake_case(name);

	uint8_t accessLevel = std::stoul(match[2].str());
	bool exitFunction{false};

	cliReader.second->read<std::string>([this, &exitFunction, name, accessLevel, connection](const std::string& uid) {
		std::string confirmMsg("Are you sure you want to add user:\n"
							   "UID: " + uid + "\n" +
							   "Name: " + name + "\n" +
							   "Access Level: " + std::to_string(accessLevel));
		connection->write<std::string>(confirmMsg);

		connection->read<std::string>([&exitFunction](const std::string& status) {
			if (status == "Approved")
				exitFunction = true;
		});
		if (exitFunction)
			return;

		std::unique_lock rw_lock(rw_mtx);
		nlohmann::json configJson;
		try {
			std::ifstream in("config.json");
			if (in && in.peek() != std::ifstream::traits_type::eof())
				in >> configJson;
			else
				configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
		} catch (const nlohmann::json::parse_error& e) {
			std::cout << "Invalid JSON @ config.json" << e.what() << std::endl;
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
		}
		if (!configJson.contains("users"))
			configJson["users"] = nlohmann::json::array();

		usersByName[name] = {uid, accessLevel};
		usersByUid[uid]   = {name, accessLevel};
		configJson["users"].push_back({
										  {"name", name},
										  {"uid", uid},
										  {"accessLevel", accessLevel}
									  });

		std::ofstream out{"config_tmp.json"};
		out << configJson.dump(4);
		std::filesystem::rename("config_tmp.json", "config.json");
		connection->write<std::string>("User Added Successfully");
	});
}

/// Remove door function.
/// @param connection shared_ptr to the current TCP client connection.
/// @param doorData String representation of the door to be removed i.e. "door1".
void ReaderHandler::rmDoor(const CONNECTION& connection, const std::string& doorData) {
	// Parse CLI command for correct syntax
	static const std::regex cliSyntax(R"(^rmDoor\s+([A-Za-z_]+)$)");
	std::smatch match;
	if (!std::regex_match(doorData, match, cliSyntax)) {
		connection->write<std::string>("Failed to remove door - Incorrect CLI syntax");
		return;
	}

	std::unique_lock rw_lock(rw_mtx);
	std::string name = match[1].str();
	to_snake_case(name);

	if (doors.erase(name) == 0) {
		std::cout << "Door not found in memory" << std::endl;
		return;
	}

	nlohmann::json configJson;
	try {
		std::ifstream in("config.json");
		if (in && in.peek() != std::ifstream::traits_type::eof())
			in >> configJson;
		else
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	} catch (const nlohmann::json::parse_error& e) {
		std::cout << "Invalid JSON @ config.json" << e.what() << std::endl;
		configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	}

	if (configJson.contains("doors")) {
		auto& doorsJson = configJson["doors"];
		for (auto it = doorsJson.begin(); it != doorsJson.end(); ++it) {
			if (it->contains("name") && (*it)["name"] == name) {
				doorsJson.erase(it);
				break;
			}
		}
	}

	std::ofstream out{"config_tmp.json"};
	out << configJson.dump(4);
	std::filesystem::rename("config_tmp.json", "config.json");
	connection->write<std::string>("Door Removed Successfully");
}

/// Remove user function.
/// @param connection shared_ptr to the current TCP client connection.
/// @param userData String representation of the user to be removed i.e. "john_doe".
void ReaderHandler::rmUser(const CONNECTION& connection, const std::string& userData) {
	// Parse CLI command for correct syntax
	static const std::regex cliSyntax(R"(^rmUser\s+([A-Za-z_]+)$)");
	std::smatch match;
	if (!std::regex_match(userData, match, cliSyntax)) {
		connection->write<std::string>("Failed to remove user - Incorrect CLI syntax");
		return;
	}

	std::string name = match[1].str();
	to_snake_case(name);

	std::unique_lock rw_lock(rw_mtx);
	auto user = usersByName.find(name);
	if (user != usersByName.end()) {
		usersByUid.erase(user->second.first); // remove by uid
		usersByName.erase(user);
	} else {
		std::cout << "User not found in memory" << std::endl;
		return;
	}

	nlohmann::json configJson;
	try {
		std::ifstream in("config.json");
		if (in && in.peek() != std::ifstream::traits_type::eof())
			in >> configJson;
		else
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	} catch (const nlohmann::json::parse_error& e) {
		std::cout << "Invalid JSON @ config.json" << e.what() << std::endl;
		configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	}

	if (configJson.contains("users")) {
		auto& usersJson = configJson["users"];
		for (auto it = usersJson.begin(); it != usersJson.end(); ++it) {
			if (it->contains("name") && (*it)["name"] == name) {
				usersJson.erase(it);
				break;
			}
		}
	}

	std::ofstream out{"config_tmp.json"};
	out << configJson.dump(4);
	std::filesystem::rename("config_tmp.json", "config.json");
	connection->write<std::string>("User Removed Successfully");
}

/// Helper function for converting std::string to snake_case.\n
/// Takes std::string by reference.
/// @param input std::string input
/// @returns void
void ReaderHandler::to_snake_case(std::string& input) {
	std::string result;
	result.reserve(input.size()); /// Albeit reserving only input.size(), more usually get's allocated.
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
}

/// Log getter function.
/// @returns nlohmann::json
nlohmann::json ReaderHandler::getLog() const {
	return log;
}
