#pragma once

#include <json.hpp>
#include <thread>
#include <boost/asio.hpp>

#include "TcpConnection.hpp"

/// @param [in] CONNECTION_T unique_ptr to a TcpConnection object which holds a unique client connection.
using CONNECTION_T = TcpConnection*; //should be std::shared_ptr<TcpConnection>

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    void start();
    void stop();

    void onClientConnect(std::function<void(CONNECTION_T)> callback);
    void removeConnection(uint32_t id);

    static void setThreadCount(uint8_t count);

private: /// Member Functions
    void acceptConnection();

private: /// Member Variables
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;

    std::unordered_map<uint32_t, std::unique_ptr<TcpConnection>> connections; //this should also be changed then to shared_ptr
    std::function<void(CONNECTION_T)> connectHandler;

    bool running = false;
    uint32_t nextId{};
    std::vector<std::thread> asyncThreads_t; /// Using strands for ACID compliance in read/write.
    static inline uint8_t threadCount{1};
    static inline uint8_t threadLimit{4};
};
