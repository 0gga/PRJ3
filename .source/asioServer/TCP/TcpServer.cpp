#include "TcpServer.h"
#include <iostream>

boost::asio::io_context TcpServer::io_context;
std::vector<std::thread> TcpServer::asyncThreads_t;
boost::asio::executor_work_guard<boost::asio::io_context::executor_type> TcpServer::work_guard =
		boost::asio::make_work_guard(TcpServer::io_context);

TcpServer::TcpServer(const int& port)
: acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)) {}


TcpServer::~TcpServer() {
	stop();
}

void TcpServer::start() {
	if (running)
		return;

	running = true;

	io_context.restart();
	acceptConnection();

	asyncThreads_t.clear();
	for (int i = 0; i < threadCount; ++i)
		asyncThreads_t.emplace_back([] {
			try {
				io_context.run();
			} catch (const std::exception& e) {
				std::cerr << "io_context thread exception: " << e.what() << std::endl;
			}
		});
}

void TcpServer::stop() {
	if (!running)
		return;
	running = false;

	io_context.stop();
	for (auto& t : asyncThreads_t)
		if (t.joinable())
			t.join();

	boost::system::error_code ec;
	acceptor.close(ec);

	if (ec)
		std::cerr << "Stop error: " << ec.message() << std::endl;
}

void TcpServer::stopAll() {
	if (io_context.stopped())
		return;

	io_context.stop();
	for (auto& t : asyncThreads_t)
		if (t.joinable())
			t.join();
}

void TcpServer::onClientConnect(std::function<void(std::shared_ptr<TcpConnection>)> callback) {
	connectHandler = std::move(callback);
}

void TcpServer::setThreadCount(uint8_t count) {
	if (count <= threadLimit) {
		threadCount = count;
	} else
		std::cerr << "Thread count cannot be greater than " << threadLimit << std::endl;
}

void TcpServer::acceptConnection() {
	auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context);
	acceptor.async_accept(*socket, [this,socket](boost::system::error_code ec) {
		if (!ec) {
			const auto connection = std::make_shared<TcpConnection>(std::move(*socket));
			std::cout << "Client connected: " << socket->remote_endpoint() << std::endl;

			if (connectHandler)
				connectHandler(connection);
		} else {
			std::cerr << "Accept Failed: " << ec.message() << std::endl;
		}
		acceptConnection();
	});
}
