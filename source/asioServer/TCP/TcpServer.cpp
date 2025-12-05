#include <iostream>

#include "TcpServer.hpp"

TcpServer::TcpServer(int port) : acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
								 work_guard_(make_work_guard(io_context_)) {}

TcpServer::~TcpServer() {
	stop();
}

void TcpServer::start() {
	if (running_)
		return;

	running_ = true;

	io_context_.restart();
	acceptConnection();

	for (int i = 0; i < threadCount_; ++i)
		asyncThreadsT_.emplace_back([this] {
			try {
				io_context_.run();
			}
			catch (const std::exception& e) {
				DEBUG_OUT("io_context thread exception: " + std::string(e.what()));
			}
		});
}

void TcpServer::stop() {
	if (!running_)
		return;

	running_ = false;

	boost::system::error_code ec;
	acceptor_.cancel(ec);
	io_context_.stop();

	for (auto& t : asyncThreadsT_)
		if (t.joinable())
			t.join();

	asyncThreadsT_.clear();
	connections_.clear();
	acceptor_.close(ec);
}

void TcpServer::onClientDisconnect(std::function<void(CONNECTION_T)> callback) {
	disconnectHandler_ = std::move(callback);
}

void TcpServer::onClientConnect(std::function<void(CONNECTION_T)> callback) {
	connectHandler_ = std::move(callback);
}

void TcpServer::setThreadCount(const uint8_t count) {
	if (count <= threadLimit_) {
		threadCount_ = count;
	} else
		DEBUG_OUT("Thread count cannot be greater than " + std::to_string(threadLimit_));
}

void TcpServer::removeConnection(const uint32_t id) {
	const auto it = connections_.find(id);
	if (it != connections_.end()) {
		const CONNECTION_T connection = it->second.get();
		if (disconnectHandler_)
			disconnectHandler_(connection);
		connections_.erase(id);
	}
}

void TcpServer::acceptConnection() {
	if (!running_ || !acceptor_.is_open())
		return;

	auto up    = std::make_unique<boost::asio::ip::tcp::socket>(io_context_);
	auto* sock = up.get();

	acceptor_.async_accept(*sock, [this, up = std::move(up)](const boost::system::error_code& ec) {
		if (!running_)
			return;

		if (!ec) {
			uint32_t id = nextId_++;

			auto connection  = std::make_unique<TcpConnection>(std::move(*up), id, this);
			CONNECTION_T raw = connection.get();
			connections_.emplace(id, std::move(connection));

			if (connectHandler_)
				connectHandler_(raw); //should just be connection
			DEBUG_OUT("Accept Succeeded");
		} else {
			DEBUG_OUT("Accept Failed: " + std::string(ec.message()));
		}
		if (running_ && acceptor_.is_open())
			acceptConnection();
		else {
			DEBUG_OUT("Acceptor is closed - Recursion stopped");
		}
	});
}
