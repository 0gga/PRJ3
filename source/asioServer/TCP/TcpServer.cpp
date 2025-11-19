#include "TcpServer.h"
#include <iostream>

TcpServer::TcpServer(int port)
: acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
  work_guard(make_work_guard(io_context)) {}

TcpServer::~TcpServer() {
    stop();
}

void TcpServer::start() {
    if (running)
        return;
    running = true;

    io_context.restart();
    acceptConnection();

    asyncThreads_t.clear();
    for (int i = 0; i < threadCount; ++i)
        asyncThreads_t.emplace_back([this] {
            try {
                io_context.run();
            } catch (const std::exception& e) {
                std::cout << "io_context thread exception: " << e.what() << std::endl;
            }
        });
}

void TcpServer::stop() {
    if (!running)
        return;
    running = false;

    io_context.stop();
    for (auto& t : asyncThreads_t)
        if (t.joinable())
            t.join();

    boost::system::error_code ec;
    acceptor.close(ec);

    if (ec)
        std::cout << "Acceptor close error: " << ec.message() << std::endl;

    connections.clear();
}

void TcpServer::onClientConnect(std::function<void(CONNECTION_T)> callback) {
    connectHandler = std::move(callback);
}

void TcpServer::setThreadCount(uint8_t count) {
    if (count <= threadLimit) {
        threadCount = count;
    } else
        std::cout << "Thread count cannot be greater than " << threadLimit << std::endl;
}

void TcpServer::removeConnection(uint32_t id) {
    connections.erase(id);
}

void TcpServer::acceptConnection() {
    if (!running)
        return;

    auto up = std::make_unique<boost::asio::ip::tcp::socket>(io_context);
    auto& sock = *up;

    acceptor.async_accept(sock, [this, up = std::move(up)](boost::system::error_code ec) {
        if (!running)
            return;
        try {
            if (!ec) {
                uint32_t id = nextId++;

                auto sock = std::move(*up);

                boost::system::error_code ep_ec;
                auto ep = sock.remote_endpoint(ep_ec);

                auto connection = std::make_unique<TcpConnection>(std::move(sock), id, this);
                CONNECTION_T raw  = connection.get();
                connections.emplace(id, std::move(connection));

                if (!ep_ec)
                    std::cout << "Client connected: " << ep << std::endl;

                if (connectHandler)
                    connectHandler(raw);
            } else {
                std::cout << "Accept Failed: " << ec.message() << std::endl;
            }
        } catch (std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
        }
        if (running)
            acceptConnection();
    });
}
