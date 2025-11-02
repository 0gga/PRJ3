#pragma once
#include "TcpServer.h"

#include <unordered_map>
#include <fstream>
#include <shared_mutex>

#include "json.hpp"

enum class ReaderState {
    Idle,
    Active
};

void myIP();

using CONNECTION = std::shared_ptr<TcpConnection>;

class ReaderHandler {
public:
    explicit ReaderHandler(const int& clientPort, const int& cliPort, const std::string& cliReader);
    ~ReaderHandler();

    void stop();

    static void runLoop();

    static void myIP();

private: // Member Functions
    void handleClient(const CONNECTION& connection);
    void handleCli(const CONNECTION& connection);

    void newDoor(const CONNECTION& connection, const std::string&);
    void newUser(const CONNECTION& connection, const std::string&);
    void rmDoor(const CONNECTION& connection, const std::string&);
    void rmUser(const CONNECTION& connection, const std::string&);
    static void to_snake_case(std::string&);

    ReaderState getState() const;

    nlohmann::json getLog() const;

private: // Member Variables
    ReaderState state          = ReaderState::Idle;
    static inline bool running = true;

    TcpServer clientServer;
    TcpServer cliServer;

    nlohmann::json log;
    std::pair<std::string, CONNECTION> cliReader;
    std::unordered_map<std::string, int> doors;
    std::unordered_map<std::string, std::pair<std::string, int>> usersByName;
    std::unordered_map<std::string, std::pair<std::string, int>> usersByUid;

    std::shared_mutex rw_mtx; // Use shared_lock for reads and unique_lock for writes.
};
