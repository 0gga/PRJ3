#pragma once

#include <iostream>
#include <boost/asio.hpp>

#include "json.hpp"

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
	explicit TcpConnection(boost::asio::ip::tcp::socket socket);
	~TcpConnection();

	void close();

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
	const auto self   = shared_from_this(); // Keep to avoid termination before callback. Keeps a reference alive until callback returns.
	auto buffer = std::make_shared<boost::asio::streambuf>();

	boost::asio::async_read_until(socket, *buffer, '\n',
								  [this, self, buffer, handler](const boost::system::error_code& ec, size_t bytesTransferred) {
									  if (ec) {
										  std::cerr << "Connection Read Failed: " << ec.message() << std::endl;
										  return;
									  }

									  std::istream is(buffer.get());
									  std::string data;
									  std::getline(is, data);

									  if constexpr (std::is_same_v<Rx, std::string>)
										  handler(data);
									  else if constexpr (std::is_same_v<Rx, nlohmann::json>)
										  handler(nlohmann::json::parse(data));

									  read<Rx>(handler);
								  });
}

template<typename Tx>
void TcpConnection::write(const Tx& data) {
	const auto self = shared_from_this(); // Keep to avoid termination before callback. Keeps a reference alive until callback returns.
	std::shared_ptr<std::string> bytes = std::make_shared<std::string>();

	if constexpr (std::is_same_v<Tx, std::string>)
		*bytes = data + '\n';
	else if constexpr (std::is_same_v<Tx, nlohmann::json>)
		*bytes = data.dump() + '\n';
	else
		static_assert(std::is_same_v<Tx, std::string> || std::is_same_v<Tx, nlohmann::json>,
					  "TcpServer::write() exclusively supports std::string or nlohmann::json");

	boost::asio::async_write(socket, boost::asio::buffer(*bytes),
							 [this, self](const boost::system::error_code& ec, std::size_t) {
								 if (ec) {
									 std::cerr << "Connection Write Failed: " << ec.message() << std::endl;
								 }
							 });
}
