#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include "json.hpp"

class TcpServer;

class TcpConnection {
public:
    TcpConnection(boost::asio::ip::tcp::socket socket, uint32_t id, TcpServer* owner);
    ~TcpConnection();

    void close();
    void onDisconnect();

    template<typename Rx>
    void read(std::function<void(const Rx&)> handler);

    template<typename Tx>
    void write(const Tx& data);

    bool connected = false;

private: // Member Variables
    boost::asio::ip::tcp::socket socket_;
    boost::asio::strand<boost::asio::any_io_executor> strand_;
    TcpServer* owner_;
    int32_t id_;
};

//clang-format off
template<typename Rx>
void TcpConnection::read(std::function<void(const Rx&)> handler) {
    auto buffer = std::make_shared<boost::asio::streambuf>();

    boost::asio::async_read_until(socket_, *buffer, '\n', boost::asio::bind_executor(strand_,
         [this, buffer, handler](const boost::system::error_code& ec, size_t bytesTransferred) {
             if (ec) {
                 std::cerr << "Read Failed: " << ec.message() << std::endl;
                 return;
             }
             std::istream is(buffer.get());
             std::string data;
             std::getline(is, data);
             if constexpr (std::is_same_v<Rx, std::string>)
                 handler(data);
             else
                 handler(nlohmann::json::parse(data));
             this->read<Rx>(handler);
    }));
}

//clang-format on
template<typename Tx>
void TcpConnection::write(const Tx& data) {
    std::shared_ptr<std::string> bytes = std::make_shared<std::string>();

    if constexpr (std::is_same_v<Tx, std::string>)
        *bytes = data + '\n';
    else if constexpr (std::is_same_v<Tx, nlohmann::json>)
        *bytes = data.dump() + '\n';
    else
        static_assert(std::is_same_v<Tx, std::string> || std::is_same_v<Tx, nlohmann::json>,
                      "TcpServer::write() exclusively supports std::string or nlohmann::json");

    boost::asio::async_write(socket_, boost::asio::buffer(*bytes),
                             boost::asio::bind_executor(strand_, [this](const boost::system::error_code& ec, std::size_t) {
                                 if (ec) {
                                     std::cerr << "Connection Write Failed: " << ec.message() << std::endl;
                                 }
                             }));
}
