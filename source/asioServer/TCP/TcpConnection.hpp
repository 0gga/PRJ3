#pragma once

#include <fstream>
#include <iostream>
#include <json.hpp>

#include <boost/asio.hpp>

class TcpServer;

class TcpConnection {
public:
	TcpConnection(boost::asio::ip::tcp::socket socket, uint32_t id, TcpServer* owner);
	~TcpConnection();

	void close();
	bool isAlive();

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

	struct FilePayload {
		std::string type;
		std::string filePath;
	};
};

template<typename T>
struct typeName {
	static constexpr std::string_view type = "type:unknown";
};

template<>
struct typeName<std::string> {
	static constexpr std::string_view type = "type:string";
};

template<typename T>
std::shared_ptr<std::string> encodePayload(const T& data) {
	auto buffer = std::make_shared<std::string>();
	buffer->reserve(64 + sizeof(T));

	buffer->append(typeName<T>::type);
	buffer->append("%%%");
	buffer->append(data);

	return buffer;
}

//clang-format off
template<typename Rx>
void TcpConnection::read(std::function<void(const Rx&)> handler) {
	if (!alive_)
		return;

	auto buffer = std::make_shared<boost::asio::streambuf>();

    boost::asio::async_read_until(socket_, *buffer, '\n',
    	boost::asio::bind_executor(
    		strand_,
    		[this, buffer, handler = std::move(handler)](const boost::system::error_code& ec, size_t bytes) {
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
         		std::istream is(buffer.get());
				std::string data;
				std::getline(is, data);

         		if constexpr (std::is_same_v<Rx, std::string>)
			 		handler(data);

         		buffer->consume(bytes);
         	} catch (const std::exception& e) {
         		std::cerr << "Read Error " << e.what() << std::endl;
         	}
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
