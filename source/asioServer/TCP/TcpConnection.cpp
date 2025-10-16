#include "TcpConnection.h"
#include <iostream>

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket) : socket(std::move(socket)) {}

TcpConnection::~TcpConnection() {
	boost::system::error_code ec;
	socket.close(ec);
}
