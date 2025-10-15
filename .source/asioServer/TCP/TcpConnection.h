#pragma once

#include <iostream>
#include <boost/asio.hpp>

#include "json.hpp"

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
	explicit TcpConnection(boost::asio::ip::tcp::socket socket);
	~TcpConnection();

	template<typename Rx>
	void read(std::function<void(const Rx&)> handler);

	template<typename Tx>
	void write(const Tx& data);

	bool connected = false;

private: // Member Variables
	boost::asio::ip::tcp::socket socket;
};

template<typename Rx>
void TcpConnection::read(std::function<void(const Rx&)> handler) {
	auto self    = shared_from_this();
	auto* buffer = new Rx();

	boost::asio::async_read(socket, boost::asio::buffer(buffer, sizeof(Rx)),
							[this, self, buffer, handler](boost::system::error_code ec, std::size_t) {
								if (ec) {
									std::cerr << "Connection Read Failed: " << ec.message() << std::endl;
									delete buffer;
									return;
								}
								handler(*buffer);
								delete buffer;
								read<Rx>(handler);
							});
}

template<typename Tx>
void TcpConnection::write(const Tx& data) {
	std::string bytes;
	if constexpr (std::is_same_v<Tx, std::string>)
		bytes = data;
	else if constexpr (std::is_same_v<Tx, nlohmann::json>)
		bytes = data.dump();
	else
		static_assert(std::is_same_v<Tx, std::string> || std::is_same_v<Tx, nlohmann::json>,
					  "TcpServer::write() exclusively supports std::string or nlohmann::json");

	auto self = shared_from_this();
	boost::asio::async_write(socket, boost::asio::buffer(bytes),
							 [this, self](boost::system::error_code ec, std::size_t) {
								 if (ec) {
									 std::cerr << "Connection Write Failed: " << ec.message() << std::endl;
								 }
							 });
}
