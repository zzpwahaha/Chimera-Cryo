#pragma once
#include <winsock2.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <queue>
#include <iostream>

class BoostAsyncSerial
{
public:
	// THIS CLASS IS NOT COPYABLE.
	BoostAsyncSerial& operator=(const BoostAsyncSerial&) = delete;
	BoostAsyncSerial(const BoostAsyncSerial&) = delete;

	//this is just a simple constructor with default options
	BoostAsyncSerial(bool safemode, std::string portID, int baudrate);
	//constructor with all options available
	BoostAsyncSerial(
		bool safemode,
		std::string portID,
		int baudrate,
		int character_size,
		boost::asio::serial_port_base::stop_bits::type stop_bits,
		boost::asio::serial_port_base::parity::type parity,
		boost::asio::serial_port_base::flow_control::type flow_control
	);
	~BoostAsyncSerial();

	void setReadCallback(const boost::function<void(int)> &read_callback);
	void setErrorCallback(const boost::function<void(std::string)>& error_callback);

	void write(std::vector<unsigned char>);
	void write(std::vector<int>);

	void disconnect();
	void reconnect();

	boost::exception_ptr lastException();

private:
	void read();
	void run();
	void readhandler(const boost::system::error_code& error, std::size_t bytes_transferred);
public:
	const bool safemode;
	const std::string portID;
	const int baudrate;
private:
	std::atomic<bool> continue_reading;
	boost::asio::io_service io_service_;
	std::unique_ptr<boost::asio::serial_port> port_;
	boost::thread io_thread;
	std::array<unsigned char, 1024> readbuffer;
	boost::mutex mutex_;
	boost::function<void(uint8_t)> read_callback_;
	boost::function<void(std::string)> error_callback_;
	std::queue<boost::exception_ptr> exceptionQueue;

	//std::unique_ptr<boost::asio::io_service::work> work;
	typedef boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard;
	std::unique_ptr<work_guard> work;
};

