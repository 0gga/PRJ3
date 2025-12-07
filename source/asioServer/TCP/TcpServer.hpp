#pragma once

#include <json.hpp>
#include <thread>
#include <boost/asio.hpp>

#include "TcpConnection.hpp"

/// @param [in] CONNECTION_T unique_ptr to a TcpConnection object which holds a unique client connection.
using CONNECTION_T = TcpConnection*;

class TcpServer {
public:
	explicit TcpServer(int port);
	~TcpServer();

	void start();
	void stop();

	void onClientConnect(std::function<void(CONNECTION_T)> callback);
	void onClientDisconnect(std::function<void(CONNECTION_T)> callback);

	static void setThreadCount(uint8_t count);
	void removeConnection(uint32_t id);

private: /// Member Functions
	void acceptConnection();

private: /// Member Variables
	boost::asio::io_context io_context_;
	boost::asio::ip::tcp::acceptor acceptor_;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;

	std::unordered_map<uint32_t, std::unique_ptr<TcpConnection>> connections_;
	std::function<void(CONNECTION_T)> disconnectHandler_;
	std::function<void(CONNECTION_T)> connectHandler_;

	bool running_ = false;
	uint32_t nextId_{};                      /// Can hold up to 4.294.967.296 connections. Should be plenty.
	std::vector<std::thread> asyncThreadsT_; /// Using strands for ACID compliance in read/write.
	static inline uint8_t threadCount_{1};
	static inline uint8_t threadLimit_{4};
};
