#include <iostream>

#include "TcpServer.hpp"

TcpServer::TcpServer(int port) : acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
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
			}
			catch (const std::exception& e) {
				DEBUG_OUT("io_context thread exception: " + std::string(e.what()));
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

void TcpServer::onClientDisconnect(std::function<void(CONNECTION_T)> callback) {
	disconnectHandler = std::move(callback);
}

void TcpServer::onClientConnect(std::function<void(CONNECTION_T)> callback) {
	connectHandler = std::move(callback);
}

void TcpServer::setThreadCount(const uint8_t count) {
	if (count <= threadLimit) {
		threadCount = count;
	} else
		DEBUG_OUT("Thread count cannot be greater than " + std::to_string(threadLimit));
}

void TcpServer::removeConnection(const uint32_t id) {
	const auto it = connections.find(id);
	if (it != connections.end()) {
		const CONNECTION_T connection = it->second.get();
		if (disconnectHandler)
			disconnectHandler(connection);
		connections.erase(id);
	}
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

			auto connection  = std::make_unique<TcpConnection>(std::move(*up), id, this); //make this shared aswell
			CONNECTION_T raw = connection.get();
			connections.emplace(id, std::move(connection));

			if (connectHandler)
				connectHandler(raw); //should just be connection
			DEBUG_OUT("Accept Succeeded");
		} else {
			DEBUG_OUT("Accept Failed: " + std::string(ec.message()));
		}
		if (running && acceptor.is_open())
			acceptConnection();
		else {
			DEBUG_OUT("Acceptor is closed - Recursion stopped");
		}
	});
}
