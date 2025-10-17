#include "TcpConnection.h"
#include <iostream>

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket) : socket(std::move(socket)) {}

TcpConnection::~TcpConnection() {
	close();
}

void TcpConnection::close() {
	boost::system::error_code ec;
	socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	if (ec && ec != boost::asio::error::not_connected)
		std::cout << "Shutdown Error: " << ec.message() << std::endl;

	socket.close(ec);
	if (ec)
		std::cout << "Close Error: " << ec.message() << std::endl;
}
