#pragma once

#include <fstream>
#include <iostream>
#include <json.hpp>

#include <boost/asio.hpp>

#include "PayloadEncoder.hpp"

class TcpServer;

class TcpConnection {
public:
	TcpConnection(boost::asio::ip::tcp::socket socket, uint32_t id, TcpServer* owner);
	~TcpConnection();

	void close();

	template<typename Rx>
	void read(std::function<void(const Rx&)> handler);

	template<typename Tx>
	void write(const Tx& data);

private: // Member Variables
	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	TcpServer* owner_;
	uint32_t id_;
	bool alive_{true};
	boost::asio::streambuf readBuffer_;
};

//clang-format off
template<typename Rx>
void TcpConnection::read(std::function<void(const Rx&)> handler) {
	if (!alive_)
		return;

    boost::asio::async_read_until(socket_, readBuffer_, '\n', boost::asio::bind_executor(strand_,
         [this, handler = std::move(handler)](const boost::system::error_code& ec, size_t bytesTransferred) {
         	if (!alive_)
				return;

         	if (ec == boost::asio::error::eof) {
         		close();
         		return;
         	}

             if (ec) {
             	std::cerr << "Read Failed: " << ec.message() << std::endl;
             	close();
             	return;
             }
         	try {
         		std::istream is(&readBuffer_);
				std::string data;
				std::getline(is, data);

         		if constexpr (std::is_same_v<Rx, std::string>)
			 		handler(data);
         	} catch (const std::exception& e) {
         		std::cerr << "Read Error" << e.what() << std::endl;
         	}

             if (alive_)
				read<Rx>(handler);
    }));
}

//clang-format on
template<typename Tx>
void TcpConnection::write(const Tx& data) {
	if (!alive_)
		return;

	auto bytes = encodePayload(data);

	boost::asio::async_write(socket_, boost::asio::buffer(*bytes),
							 boost::asio::bind_executor(strand_, [this, bytes](const boost::system::error_code& ec, std::size_t) {
								 if (ec)
									 close();
							 }));
}
