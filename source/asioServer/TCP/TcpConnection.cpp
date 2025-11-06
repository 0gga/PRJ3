#include "TcpConnection.h"
#include "TcpServer.h"
#include <iostream>

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket, uint32_t id, TcpServer* owner)
: socket_(std::move(socket)),
  strand_(socket_.get_executor()),
  owner_(owner),
  id_(id) {}

TcpConnection::~TcpConnection() {
	close();
}

void TcpConnection::close() {
	boost::system::error_code ec;
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	if (ec && ec != boost::asio::error::not_connected)
		std::cout << "Shutdown Error: " << ec.message() << std::endl;

	socket_.close(ec);
	if (ec)
		std::cout << "Close Error: " << ec.message() << std::endl;
	if (owner_) owner_->removeConnection(id_);
}
