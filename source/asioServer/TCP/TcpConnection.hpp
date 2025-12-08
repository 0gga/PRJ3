#pragma once

#include <fstream>
#include <iostream>
#include <json.hpp>

#include <boost/asio.hpp>

#ifdef DEBUG
#define DEBUG_OUT(msg) \
    do { \
        std::cout << (msg) << std::endl; \
    } while(0)
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

//clang-format on
template<typename Tx>
void TcpConnection::write(const Tx& data) {
	if (!alive_)
		return;
	DEBUG_OUT("Writing...\n");

	auto bytes = std::make_shared<std::string>();
	bytes->reserve(64 + sizeof(Tx));

	bytes->append("type:string");
	bytes->append("%%%");
	bytes->append(data);
	bytes->push_back('\n');

	boost::asio::async_write(socket_, boost::asio::buffer(*bytes),
							 boost::asio::bind_executor(
														strand_, [this, bytes](const boost::system::error_code& ec, std::size_t) {
															if (ec)
																close();
														}));
}

inline void TcpConnection::writeFile(const std::string& path)
{
	if (!alive_)
		return;

	namespace asio = boost::asio;

	// --- Read file into memory ---
	std::ifstream ifs(path, std::ios::binary | std::ios::ate);
	if (!ifs) {
		DEBUG_OUT("Failed to open file: " + path);
		return;
	}

	std::streamsize size = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	auto data = std::make_shared<std::string>(size, '\0');
	if (!ifs.read(data->data(), size)) {
		DEBUG_OUT("Failed to read file: " + path);
		return;
	}

	// Extract filename only
	std::string filename = std::filesystem::path(path).filename().string();

	// --- Build header: type:file%%%<filename>%%%<filesize>\n ---
	auto header = std::make_shared<std::string>();
	header->reserve(64 + filename.size());
	*header = "type:file%%%" + filename + "%%%" + std::to_string(size) + "\n";

	// --- Step 1: async header write ---
	asio::async_write(
		socket_,
		asio::buffer(*header),
		asio::bind_executor(
			strand_,
			[this, header, data](const boost::system::error_code& ec, std::size_t)
			{
				if (ec) {
					close();
					return;
				}

				// --- Step 2: async file payload write ---
				asio::async_write(
					socket_,
					asio::buffer(*data),
					asio::bind_executor(
						strand_,
						[this, data](const boost::system::error_code& ec, std::size_t)
						{
							if (ec)
								close();
						}
					)
				);
			}
		)
	);
}
