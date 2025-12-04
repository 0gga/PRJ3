#pragma once

#include <fstream>
#include <iostream>
#include <json.hpp>

#include <boost/asio.hpp>

#ifdef DEBUG
#define DEBUG_OUT(msg) do { std::cout << (msg) << std::endl;} while(0)
#else
#define DEBUG_OUT(msg) do {} while(0)
#endif

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
	void writeFile(const std::string&);

private: // Member Variables
	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	TcpServer* owner_;
	uint32_t id_;
	bool alive_{true};
};

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
             	DEBUG_OUT("Read Failed: " + std::string(ec.message()));
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
             	DEBUG_OUT("Read Error: " + std::string(e.what()));
         	}
    }));
}

#include "PayloadEncoder.hpp"

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

inline void TcpConnection::writeFile(const std::string& filepath) {
	if (!alive_)
		return;

	// Step 1: validate file
	std::ifstream file(filepath, std::ios::binary);
	if (!file) {
		DEBUG_OUT("File open failed: " + filepath);
		return;
	}
	DEBUG_OUT("Sending file: " + filepath);

	// Step 2: get filesize
	file.seekg(0, std::ios::end);
	const std::size_t filesize = file.tellg();
	file.seekg(0, std::ios::beg);

	// Step 3: extract filename only
	const std::string filename = filepath.substr(filepath.find_last_of("/\\") + 1);

	// Step 4: build header
	auto header = std::make_shared<std::string>();
	header->reserve(128);

	header->append("type:file%%%");
	header->append(filename);
	header->append("%%%");
	header->append(std::to_string(filesize));
	header->push_back('\n');

	// Step 5: send header
	auto file_ptr = std::make_shared<std::ifstream>(filepath, std::ios::binary);

	boost::asio::async_write(
							 socket_,
							 boost::asio::buffer(*header),
							 boost::asio::bind_executor(
														strand_,
														[this, header, file_ptr, filesize]
												(const boost::system::error_code& ec, std::size_t) {
															if (ec) {
																close();
																return;
															}

															auto buffer    = std::make_shared<std::vector<char>>(8192);
															auto sendChunk = std::make_shared<std::function<void()>>();

															std::weak_ptr weakSend = sendChunk;

															*sendChunk = [this, buffer, weakSend, file_ptr]() mutable {
																file_ptr->read(buffer->data(), buffer->size());
																std::streamsize bytesRead = file_ptr->gcount();

																if (bytesRead <= 0)
																	return;

																boost::asio::async_write(
																						 socket_,
																						 boost::asio::buffer(buffer->data(), bytesRead),
																						 boost::asio::bind_executor(
																							  strand_,
																							  [this, buffer, weakSend, file_ptr]
																					  (const boost::system::error_code& ec, std::size_t) {
																								  if (ec) {
																									  close();
																									  return;
																								  }

																								  if (auto fn = weakSend.lock())
																									  (*fn)();
																							  }
																							 )
																						);
															};

															(*sendChunk)();
														}
													   )
							);
}
