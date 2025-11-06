#pragma once

#include <json.hpp>
#include <thread>
#include <boost/asio.hpp>

#include "TcpConnection.h"

/// @param [in] CONNECTION shared_ptr to the TcpConnection object which holds a unique client connection.
using CONNECTION = TcpConnection*;

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    void start();
    void stop();

    void onClientConnect(std::function<void(CONNECTION)> callback);
    void removeConnection(uint32_t id);

    static void setThreadCount(uint8_t);

private: /// Member Functions
    void acceptConnection();

private: /// Member Variables
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;

    std::unordered_map<uint32_t, std::unique_ptr<TcpConnection>> connections;
    std::function<void(CONNECTION)> connectHandler;

    bool running = false;
    uint32_t nextId{};
    std::vector<std::thread> asyncThreads_t; /// Using strands for ACID compliance in read/write.
    static inline uint8_t threadCount{1};
    static inline uint8_t threadLimit{4};
};
