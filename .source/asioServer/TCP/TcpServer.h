#pragma once

#include <json.hpp>
#include <thread>
#include <boost/asio.hpp>

#include "TcpConnection.h"

class TcpServer {
public:
	explicit TcpServer(const int& port);
	~TcpServer();

	void start();
	void stop();
	static void stopAll();

	void onClientConnect(std::function<void(std::shared_ptr<TcpConnection>)> callback);

	static void setThreadCount(const uint8_t&);

private: // Member Functions
	void acceptConnection();

private: // Member Variables
	static boost::asio::io_context io_context;
	static std::vector<std::thread> asyncThreads_t;
	static boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;

	boost::asio::ip::tcp::acceptor acceptor;
	bool running = false;
	static inline uint8_t threadCount{1};
	static inline uint8_t threadLimit{4};
	std::function<void(std::shared_ptr<TcpConnection>)> connectHandler;
};
