#include <iostream>

#include "TcpServer.hpp"

TcpServer::TcpServer(int port)
: acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
  work_guard(make_work_guard(io_context)) {}

TcpServer::~TcpServer() {
	stop();
}

void TcpServer::start() {
	if (running)
		return;

	running = true;

	io_context.restart();
	acceptConnection();

	for (int i = 0; i < threadCount; ++i)
		asyncThreads_t.emplace_back([this] {
			try {
				io_context.run();
			} catch (const std::exception& e) {
				std::cout << "io_context thread exception: " << e.what() << std::endl;
			}
		});
}

void TcpServer::stop() {
	if (!running)
		return;

	running = false;

	boost::system::error_code ec;
	acceptor.cancel(ec);
	io_context.stop();

	for (auto& t : asyncThreads_t)
		if (t.joinable())
			t.join();

	asyncThreads_t.clear();
	connections.clear();
	acceptor.close(ec);
}

void TcpServer::onClientConnect(std::function<void(CONNECTION_T)> callback) {
	connectHandler = std::move(callback);
}

void TcpServer::setThreadCount(const uint8_t count) {
	if (count <= threadLimit) {
		threadCount = count;
	} else
		std::cout << "Thread count cannot be greater than " << threadLimit << std::endl;
}

void TcpServer::removeConnection(const uint32_t id) {
	connections.erase(id);
}

void TcpServer::acceptConnection() {
	if (!running || !acceptor.is_open())
		return;

	auto up    = std::make_unique<boost::asio::ip::tcp::socket>(io_context);
	auto* sock = up.get();

	acceptor.async_accept(*sock, [this, up = std::move(up)](boost::system::error_code ec) {
		if (!running)
			return;

		if (!ec) {
			uint32_t id = nextId++;

			auto connection  = std::make_unique<TcpConnection>(std::move(*up), id, this);
			CONNECTION_T raw = connection.get();
			connections.emplace(id, std::move(connection));

			if (connectHandler)
				connectHandler(raw);
		} else {
			std::cout << "Accept failed: " << ec.message() << std::endl;
		}
		if (running && acceptor.is_open())
			acceptConnection();
		else {
			std::cout << "Acceptor is closed - Recursion stopped" << std::endl;
		}
	});
}
