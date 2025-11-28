#include "ReaderHandler.hpp"

#include <iostream>
#include <regex>

///
ReaderHandler::ReaderHandler(const int& clientPort, const int& cliPort, const std::string& cliName)
: clientServer(clientPort),
  cliServer(cliPort),
  cliReader{cliName, nullptr} {
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
            else std::cout << "Invalid door entry in config.json - skipping one.\n";
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
            } else std::cout << "Invalid user entry in config.json - skipping one.\n";
        }
    ////////////////////////////// Read config JSON //////////////////////////////

    //////////////////////////////// Init Servers ////////////////////////////////
    TcpServer::setThreadCount(4);

    clientServer.start();
    cliServer.start();

    clientServer.onClientConnect([this](const CONNECTION_T& connection) {
        std::cout << "Client Connected\n";
        handleClient(connection);
    });

    cliServer.onClientConnect([this](const CONNECTION_T& connection) {
        std::cout << "CLI Connected\n";
        handleCli(connection);
    });

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
    while (running) std::this_thread::sleep_for(std::chrono::milliseconds(500));
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

std::pair<std::string, uint8_t> ReaderHandler::checkSyntax(command type) {

    return std::make_pair("-1",0);
}

/// Handles Client IO.\n
/// Is automatically called via lambda callback in CTOR whenever a new TCP Connection is established on the clientServer.<br>Recalls itself after each pass.
/// @param connection ptr to the relative TcpConnection object. This is established and passed in the CTOR callback.
/// @returns void
void ReaderHandler::handleClient(CONNECTION_T connection) {
    state = ReaderState::Active;

    connection->read<std::string>([this, connection](const std::string& pkg) {
        const size_t seperator = pkg.find(':');
        if (seperator == std::string::npos || seperator == 0 || seperator == pkg.size() - 1) {
            std::cout << "Invalid Client Package Syntax" << std::endl;
            connection->write<std::string>("Invalid Client Package Syntax - Connection Closed");
            connection->close();
            return;
        }

        {
            const std::string name = pkg.substr(0, seperator);
            const std::string uid  = pkg.substr(seperator + 1);

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
/// @param connection ptr to the relative TcpConnection object. This is established and passed in the CTOR callback.
/// @returns void
void ReaderHandler::handleCli(CONNECTION_T connection) {
    state = ReaderState::Active;
    {
        // Phase 1: Check if admin is connected and if not facilitate new connection.
        std::scoped_lock{cli_mtx};

        if (cliReader.second == connection) {
            connection->write<std::string>("CLI is ready");
        } else if (!cliReader.second) {
            connection->write<std::string>("Input CLI identification");
        } else {
            connection->write<std::string>("Another Admin is connected");
            connection->close();
            return;
        }
    }

    connection->read<std::string>([this, connection](const std::string& pkg) {
        // Phase 2: Handle admin duplicates the connection registered as admin
        if (cliReader.second != connection && pkg.rfind(cliReader.first, 0) == 0) {
            cliReader.second = connection;
            handleCli(connection);
            return;
        } else if (cliReader.second != connection && pkg.rfind(cliReader.first, 0) != 0) {
            connection->write<std::string>("Incorrect CLI identification");
            handleCli(connection);
            return;
        }

        // Phase 3: Handle cmdlets
        if (pkg.rfind("newUser", 0) == 0) {
            if (stoi(checkSyntax(newUser_).first) != -1)
                newUser(connection, pkg);
            return;
        }
        if (pkg.rfind("newDoor", 0) == 0) {
            std::pair<std::string, uint8_t> args = checkSyntax(newDoor_);
            if (stoi(args.first) != -1)
                newDoor(connection, pkg);
            else
                handleCli(connection);
            return;
        }
        if (pkg.rfind("rmUser", 0) == 0) {
            if (stoi(checkSyntax(rmUser_).first) != -1)
                rmUser(connection, pkg);
            return;
        }
        if (pkg.rfind("rmDoor", 0) == 0) {
            if (stoi(checkSyntax(rmDoor_).first) != -1)
                rmDoor(connection, pkg);
            return;
        }
        if (pkg == "getLog") {
            //connection->write<csv>(getLog());
            return;
        }
        if (pkg == "exit") {
            cliReader.second = nullptr;
            connection->close();
            return;
        }
        if (pkg == "shutdown") {
            connection->write<std::string>("Shutting Down...");
            running = false;
            clientServer.stop();
            cliServer.stop();
        } else {
            connection->write<std::string>("Unknown Command");
            handleCli(connection);
        }
    });
    state = ReaderState::Idle;
}

/// Add new user function.
/// @param connection ptr to the relative TcpConnection object.
/// @param userData String representation of the user to be added i.e. "john_doe 1".
void ReaderHandler::newUser(CONNECTION_T connection, const std::string& userData) {
    // Parse CLI command for correct syntax
    static const std::regex cliSyntax(R"(^newUser\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
    std::smatch match;
    if (!std::regex_match(userData, match, cliSyntax)) {
        connection->write<std::string>("Failed to add new user - Incorrect CLI syntax");
        handleCli(connection);
        return;
    }

    std::string name = match[1].str();
    to_snake_case(name);

    uint8_t accessLevel = std::stoul(match[2].str());
    // CLI Parse stop

    connection->write<std::string>("Awaiting card read");
    connection->read<std::string>([this, name, accessLevel, connection](const std::string& uid) {
        const std::string confirmMsg("Are you sure you want to add user:\n"
                                     "UID: " + uid + "\n" +
                                     "Name: " + name + "\n" +
                                     "Access Level: " + std::to_string(accessLevel));
        connection->write<std::string>(confirmMsg);

        connection->read<std::string>([this, name, accessLevel, uid, connection](const std::string& status) {
            if (status == "denied" || status != "approved") {
                connection->write<std::string>("Did not add user");
                handleCli(connection);
                return;
            }

            std::unique_lock rw_lock(rw_mtx);
            nlohmann::json configJson;
            try {
                std::ifstream in("config.json");
                if (in && in.peek() != std::ifstream::traits_type::eof()) in >> configJson;
                else configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
            } catch (const nlohmann::json::parse_error& e) {
                std::cout << "Invalid JSON @ config.json" << e.what() << std::endl;
                configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
            }
            if (!configJson.contains("users")) configJson["users"] = nlohmann::json::array();

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
            handleCli(connection);
        });
    });
}

/// Add new door function.
/// /// @param connection ptr to the relative TcpConnection object.
/// @param doorData String representation of the door to be added i.e. "door1 1".
void ReaderHandler::newDoor(CONNECTION_T connection, const std::string& doorData) {
    // Parse CLI command for correct syntax
    static const std::regex cliSyntax(R"(^newDoor\s+([A-Za-z0-9_]+)\s+([0-9]+)$)");
    std::smatch match;
    if (!std::regex_match(doorData, match, cliSyntax)) {
        connection->write<std::string>("Failed to add new door - Incorrect CLI syntax");
        handleCli(connection);
        return;
    }

    std::string name = match[1].str();
    to_snake_case(name);

    uint8_t accessLevel = std::stoul(match[2].str());
    // CLI Parse stop

    const std::string confirmMsg("Are you sure you want to add door:\n"
                                 "Name: " + name + "\n" +
                                 "Access Level: " + std::to_string(accessLevel));
    connection->write<std::string>(confirmMsg);
    connection->read<std::string>([this, name, accessLevel, connection](const std::string& status) {
        if (status == "denied" || status != "approved") {
            connection->write<std::string>("Did not add door");
            handleCli(connection);
            return;
        }

        std::unique_lock rw_lock(rw_mtx);
        nlohmann::json configJson;
        try {
            std::ifstream in("config.json");
            if (in && in.peek() != std::ifstream::traits_type::eof()) in >> configJson;
            else configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
        } catch (const nlohmann::json::parse_error& e) {
            std::cout << "Invalid JSON @ config.json" << e.what() << std::endl;
            configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
        }
        if (!configJson.contains("doors")) configJson["doors"] = nlohmann::json::array();

        doors[name] = accessLevel;
        configJson["doors"].push_back({
                                          {"name", name},
                                          {"accessLevel", accessLevel}
                                      });

        std::ofstream out{"config_tmp.json"};
        out << configJson.dump(4);
        std::filesystem::rename("config_tmp.json", "config.json");
        connection->write<std::string>("Door Added Successfully");
        handleCli(connection);
    });
}

/// Remove user function.
/// @param connection ptr to the relative TcpConnection object.
/// @param userData String representation of the user to be removed i.e. "john_doe".
void ReaderHandler::rmUser(CONNECTION_T connection, const std::string& userData) {
    // Parse CLI command for correct syntax
    static const std::regex cliSyntax(R"(^rmUser\s+([A-Za-z_]+)$)");
    std::smatch match;
    if (!std::regex_match(userData, match, cliSyntax)) {
        connection->write<std::string>("Failed to remove user - Incorrect CLI syntax");
        handleCli(connection);
        return;
    }

    std::string name = match[1].str();
    to_snake_case(name);
    // CLI Parse stop

    auto user = usersByName.find(name);
    const std::string confirmMsg("Are you sure you want to remove user:\n"
                                 "UID: " + user->second.first + "\n"
                                 "Name: " + name + "\n"
                                 "Access Level: " + std::to_string(user->second.second));
    connection->write<std::string>(confirmMsg);
    connection->read<std::string>([this, user, name, connection](const std::string& status) {
        if (status == "denied" || status != "approved") {
            connection->write<std::string>("Did not remove user");
            handleCli(connection);
            return;
        }
        std::unique_lock rw_lock(rw_mtx);
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
            if (in && in.peek() != std::ifstream::traits_type::eof()) in >> configJson;
            else configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
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
        handleCli(connection);
    });
}

/// Remove door function.
/// @param connection ptr to the relative TcpConnection object.
/// @param doorData String representation of the door to be removed i.e. "door1".
void ReaderHandler::rmDoor(CONNECTION_T connection, const std::string& doorData) {
    // Parse CLI command for correct syntax
    static const std::regex cliSyntax(R"(^rmDoor\s+([A-Za-z_]+)$)");
    std::smatch match;
    if (!std::regex_match(doorData, match, cliSyntax)) {
        connection->write<std::string>("Failed to remove door - Incorrect CLI syntax");
        handleCli(connection);
        return;
    }

    std::string name = match[1].str();
    to_snake_case(name);

    if (doors.find(name) != doors.end()) {
        connection->write<std::string>("Door could not be found");
        handleCli(connection);
        return;
    }
    const auto door = doors.find(name);
    const std::string confirmMsg("Are you sure you want to remove door:\n"
                                 "Name: " + name + "\n" +
                                 "Access Level: " + std::to_string(door->second));
    connection->write<std::string>(confirmMsg);
    connection->read<std::string>([this, name, connection](const std::string& status) {
        if (status == "denied" || status != "approved") {
            connection->write<std::string>("Did not remove door");
            handleCli(connection);
            return;
        }
        // CLI Parse stop

        std::unique_lock rw_lock(rw_mtx);
        if (doors.erase(name) == 0) {
            std::cout << "Door not found in memory" << std::endl;
            return;
        }

        nlohmann::json configJson;
        try {
            std::ifstream in("config.json");
            if (in && in.peek() != std::ifstream::traits_type::eof()) in >> configJson;
            else configJson = {{"users", nlohmann::json::array()}, {"doors", nlohmann::json::array()}};
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
    });
}

void ReaderHandler::mvUser(CONNECTION_T connection, const std::string&) {}

void ReaderHandler::mvDoor(CONNECTION_T connection, const std::string&) {}

/// Helper function for converting std::string to snake_case.\n
/// Takes std::string by reference.
/// @param input std::string input
/// @returns void
void ReaderHandler::to_snake_case(std::string& input) {
    std::string result;
    result.reserve(input.size());
    /// Albeit reserving only input.size(), more usually get's allocated.
       /// If larger than input.size() it will just reallocate which is negligible regardless, considering the string sizes we'll be handling.

    bool prevLower{false};
    for (const unsigned char c : input) {
        if (std::isupper(c)) {
            if (prevLower) result += '_';
            result += static_cast<char>(std::tolower(c));
            prevLower = false;
        } else {
            result += static_cast<char>(c);
            prevLower = true;
        }
    }
    input = std::move(result);
}
