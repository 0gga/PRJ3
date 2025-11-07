#include "TcpConnection.h"
#include "TcpServer.h"
#include <iostream>

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket, uint32_t id, TcpServer* owner)
: socket_(std::move(socket)),
  strand_(socket_.get_executor()),
  owner_(owner),
  id_(id) {}

TcpConnection::~TcpConnection() {
    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

void TcpConnection::close() {
    boost::system::error_code ec;
    socket_.cancel(ec);
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);

    if (owner_) {
        auto owner = owner_;
        auto id    = id_;
        post(strand_, [owner, id] { owner->removeConnection(id); });
    }
}

void TcpConnection::onDisconnect() {
    if (owner_) owner_->removeConnection(id_);
}