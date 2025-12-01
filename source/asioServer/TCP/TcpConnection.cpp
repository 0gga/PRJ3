#include <iostream>

#include "TcpConnection.hpp"
#include "TcpServer.hpp"

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket, const uint32_t id, TcpServer* owner) : socket_(std::move(socket)),
																										 strand_(socket_.get_executor()),
																										 owner_(owner),
																										 id_(id) {}

TcpConnection::~TcpConnection() {
	alive_ = false;

	boost::system::error_code ec;
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);
}

void TcpConnection::close() {
	write<std::string>("Closing Connection...");
	if (!alive_)
		return;
	alive_ = false;

	boost::system::error_code ec;
	socket_.cancel(ec);
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	socket_.close(ec);

	if (owner_) {
		auto owner = owner_;
		auto id    = id_;
		boost::asio::post(strand_, [owner, id] {
			owner->removeConnection(id);
		});
	}
}

bool TcpConnection::isAlive() {
	return alive_;
}
