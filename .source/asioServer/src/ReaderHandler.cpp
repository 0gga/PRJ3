#include "ReaderHandler.h"

#include <iostream>
#include <regex>

ReaderHandler::ReaderHandler(const int& clientPort, const int& cliPort)
: clientServer(clientPort), cliServer(cliPort) {
	//////////////////////////////// Init Servers ////////////////////////////////
	clientServer.onClientConnect([this](std::shared_ptr<TcpConnection> connection) {
		std::cout << "Client Connected\n";
		handleClient(connection);
	});

	cliServer.onClientConnect([this](std::shared_ptr<TcpConnection> connection) {
		std::cout << "CLI Connected\n";
		handleCli(connection);
	});

	TcpServer::setThreadCount(4);

	clientServer.start();
	cliServer.start();

	running = true;
	std::cout << "Servers started and awaiting clients" << std::endl;
	//////////////////////////////// Init Servers ////////////////////////////////

	////////////////////////////// Read config JSON //////////////////////////////
	std::ifstream file("config.json");
	nlohmann::json configJson;

	if (!file.is_open() || file.peek() == std::ifstream::traits_type::eof()) {
		std::cerr << "config.json doesn't exist" << std::endl;
		std::ofstream("config.json") << R"({"users":[],"doors":[]})";
		std::cout << "Created empty config.json" << std::endl;
	} else {
		try {
			file >> configJson;
		} catch (const nlohmann::json::parse_error& e) {
			std::cerr << "Invalid JSON in config.json" << e.what() << std::endl;
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};

			std::ofstream("config.json") << R"({"users":[],"doors":[]})";
			std::cout << "Reset invalid config.json\n";
		}
	}

	//////////////////////////// Write onto hash maps ////////////////////////////
	if (configJson.contains("doors"))
		for (const auto& door : configJson["doors"]) {
			if (door.contains("name") && door.contains("accessLevel") &&
				door["name"].is_string() && door["accessLevel"].is_number_integer())
				doors[door["name"]] = door["accessLevel"];
			else
				std::cerr << "Invalid door entry in config.json - skipping one.\n";
		}

	if (configJson.contains("users"))
		for (const auto& user : configJson["doors"]) {
			if (user.contains("name") && user.contains("uid") && user.contains("accessLevel") &&
				user["name"].is_string() && user["uid"].is_string() && user["accessLevel"].is_number_integer())
				users[user["name"]] = {user["uid"], user["accessLevel"]};
			else
				std::cerr << "Invalid user entry in config.json - skipping one.\n";
		}
	//////////////////////////// Write onto hash maps ////////////////////////////
	////////////////////////////// Read config JSON //////////////////////////////
	runLoop();
}

ReaderHandler::~ReaderHandler() {
	stop();
}

void ReaderHandler::stop() {
	state = ReaderState::Idle;
	TcpServer::stopAll();
	std::cout << "Servers Shutting Down" << std::endl;
}

ReaderState ReaderHandler::getState() const {
	return state;
}

void ReaderHandler::runLoop() {
	while (running)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

void ReaderHandler::handleClient(const std::shared_ptr<TcpConnection>& connection) {
	state = ReaderState::Active;
	connection->read<std::string>([this, connection](const std::string& pkg) {
		const size_t seperator = pkg.find(':');
		std::string name       = pkg.substr(0, seperator);
		std::string uid        = pkg.substr(seperator + 1);

		auto door = doors.find(name);
		if (door == doors.end()) {
			connection->write<std::string>("Unknown Door");
			handleClient(connection);
			return;
		}

		const auto user       = users.find(uid);
		const bool authorized = (user != users.end() && user->second.second >= door->second);

		connection->write<std::string>(authorized ? "Approved" : "Denied");
		state = ReaderState::Idle;
		handleClient(connection);
	});
}

void ReaderHandler::handleCli(const std::shared_ptr<TcpConnection>& connection) {
	state = ReaderState::Active;
	connection->read<std::string>([this, connection](const std::string& pkg) {
		if (pkg.rfind("newDoor", 0) == 0) {
			newDoor(connection, pkg);
		} else if (pkg.rfind("newUser", 0) == 0) {
			newUser(connection, pkg);
		} else if (pkg.rfind("rmUser", 0) == 0) {
			rmUser(connection, pkg);
		} else if (pkg == "getLog") {
			connection->write<nlohmann::json>(getLog());
		} else if (pkg == "shutdown") {
			connection->write<std::string>("Shutting Down...");
			running = false;
			TcpServer::stopAll();
			return;
		} else {
			connection->write<std::string>("Unknown Command");
		}
		state = ReaderState::Idle;
		handleCli(connection);
	});
}

void ReaderHandler::newDoor(const std::shared_ptr<TcpConnection>& connection, const std::string& doorData) {
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

	nlohmann::json configJson;
	try {
		std::ifstream in("config.json");
		if (in && in.peek() != std::ifstream::traits_type::eof())
			in >> configJson;
		else
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
	} catch (const nlohmann::json::parse_error& e) {
		std::cerr << "Invalid JSON @ config.json" << e.what() << std::endl;
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

void ReaderHandler::newUser(const std::shared_ptr<TcpConnection>& connection, const std::string& userData) {
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
	connection->read<std::string>([this, name, accessLevel, connection](const std::string& uid) {
		nlohmann::json configJson;
		try {
			std::ifstream in("config.json");
			if (in && in.peek() != std::ifstream::traits_type::eof())
				in >> configJson;
			else
				configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
		} catch (const nlohmann::json::parse_error& e) {
			std::cerr << "Invalid JSON @ config.json" << e.what() << std::endl;
			configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
		}
		if (!configJson.contains("users"))
			configJson["users"] = nlohmann::json::array();

		users[name] = {uid, accessLevel};
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

void ReaderHandler::rmDoor(const std::shared_ptr<TcpConnection>& connection, const std::string& doorData) {
	// Parse CLI command for correct syntax
	static const std::regex cliSyntax(R"(^rmDoor\s+([A-Za-z_]+)$)");
	std::smatch match;
	if (!std::regex_match(doorData, match, cliSyntax)) {
		connection->write<std::string>("Failed to remove door - Incorrect CLI syntax");
		return;
	}

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
		std::cerr << "Invalid JSON @ config.json" << e.what() << std::endl;
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

void ReaderHandler::rmUser(const std::shared_ptr<TcpConnection>& connection, const std::string& userdata) {
	// Parse CLI command for correct syntax
	static const std::regex cliSyntax(R"(^rmUser\s+([A-Za-z_]+)$)");
	std::smatch match;
	if (!std::regex_match(userdata, match, cliSyntax)) {
		connection->write<std::string>("Failed to remove user - Incorrect CLI syntax");
		return;
	}

	std::string name = match[1].str();
	to_snake_case(name);

	if (users.erase(name) == 0) {
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
		std::cerr << "Invalid JSON @ config.json" << e.what() << std::endl;
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

void ReaderHandler::to_snake_case(std::string& input) {
	std::string result;

	for (size_t i = 0; i < input.size(); ++i) {
		char c = static_cast<unsigned char>(input[i]);
		if (std::isupper(c)) {
			if (i > 0 && std::islower(static_cast<unsigned char>(input[i - 1])))
				result += '_';
			result += std::tolower(c);
		} else
			result += c;
	}
	input = result;
}

nlohmann::json ReaderHandler::getLog() const {
	return log;
}
