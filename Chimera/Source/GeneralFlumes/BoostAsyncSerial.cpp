#include "stdafx.h"
#include "BoostAsyncSerial.h"
#include <qdebug.h>

BoostAsyncSerial::BoostAsyncSerial(bool safemode, std::string portID, int baudrate)
	: BoostAsyncSerial(safemode, portID, baudrate, 8,
		boost::asio::serial_port_base::stop_bits::one,
		boost::asio::serial_port_base::parity::none,
		boost::asio::serial_port_base::flow_control::none) {}

BoostAsyncSerial::BoostAsyncSerial(
	bool safemode,
	std::string portID,
	int baudrate,
	int character_size,
	boost::asio::serial_port_base::stop_bits::type stop_bits,
	boost::asio::serial_port_base::parity::type parity,
	boost::asio::serial_port_base::flow_control::type flow_control) 
	: safemode(safemode)
	, portID(portID)
	, baudrate(baudrate)
	, continue_reading(true)
{
	if (!safemode) {
		port_ = std::make_unique<boost::asio::serial_port>(io_service_);

		port_->open(portID);
		port_->set_option(boost::asio::serial_port_base::baud_rate(baudrate));
		port_->set_option(boost::asio::serial_port_base::character_size(character_size));
		port_->set_option(boost::asio::serial_port_base::stop_bits(stop_bits));
		port_->set_option(boost::asio::serial_port_base::parity(parity));
		port_->set_option(boost::asio::serial_port_base::flow_control(flow_control));

		io_thread = boost::thread(boost::bind(&BoostAsyncSerial::run, this));
		Sleep(10); // give some time for the io_thread to run
		read();
	}
}

BoostAsyncSerial::~BoostAsyncSerial()
{
	// this io_service_.stop() will make the readhandler stop, but will also make the port not released after 'cancel()' and 'close()'
	// so need to avoid using io_service_.stop() when disconnect
	io_service_.stop();

	if (port_) {
		port_->cancel();
		port_->close();
	}
	
	io_thread.join();
}

void BoostAsyncSerial::setReadCallback(const boost::function<void(int)>& read_callback)
{
	read_callback_ = read_callback;
}

void BoostAsyncSerial::readhandler(const boost::system::error_code & error, std::size_t bytes_transferred)
{
	boost::mutex::scoped_lock look(mutex_);

	if (error) {
		if (!continue_reading) {
			// probably due to the port closing, so just return
			std::string s = error.message();
			qDebug() << "BoostAsyncSerial::readhandler: continue_reading false: " << qstr(s);
		}
		else {
			std::string s = error.message();
			throw("Error reading from serial: " + s);
		}
	}
	
	if (continue_reading) {
		port_->async_read_some(boost::asio::buffer(readbuffer), boost::bind(&BoostAsyncSerial::readhandler, this,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		));
	}
	int c;
	for (int idx = 0; idx < bytes_transferred; idx++) {
		c = static_cast<int>(readbuffer[idx]) & 0xFF;
		if (!read_callback_.empty()) {
			read_callback_(c);
		}
	}
		
}

void BoostAsyncSerial::write(std::vector<unsigned char> data)
{
	if (safemode) {
		return;
	}
	if (!port_->is_open()) {
		thrower("Serial port has not been opened");
	}
	boost::asio::write(*port_, boost::asio::buffer(data));
}

void BoostAsyncSerial::write(std::vector<int> data)
{
	if (safemode) {
		return;
	}
	std::vector<unsigned char> converted(data.size());
	for (int idx = 0; idx < data.size(); idx++) {
		if(data[idx] < 0 || data[idx] >255){
			thrower("Byte value needs to be in range 0-255");
		}
		converted[idx] = data[idx];
	}

	std::cout << "Serial is writing: ";
	for (auto& byte : data)
		std::cout << (int)byte << " ";
	std::cout << "\n";

	boost::asio::write(*port_, boost::asio::buffer(converted));
}

void BoostAsyncSerial::disconnect()
{
	if (safemode) {
		return;
	}
	if (!port_) {
		thrower("port_ is already closed. Can not disconnect again");
	}
	try {
		//io_service_.stop(); // will make the readhandler stop, but will also make the port not released after 'cancel()' and 'close()'
		if (port_) {
			continue_reading = false;
			boost::system::error_code ec;
			port_->cancel(ec);
			auto s1 = ec.message();
			port_->close(ec);
			auto s2 = ec.message();
		}
		if (port_->is_open()) {
			thrower("After port->close(), the port is still open??");
		}
	}
	catch (boost::system::system_error& ex) {
		throwNested("Error in disconnecting BoostAsyncSerial.");
	}
	work->reset();
	work.reset();
	io_thread.join();
	port_.reset();
}

void BoostAsyncSerial::reconnect()
{
	if (safemode) {
		return;
	}
	if (port_ && port_->is_open()/*io_service_.stopped()*/) {
		thrower("port_ is already open. Can not connect again");
	}
	continue_reading = true;
	io_service_.restart();

	port_ = std::make_unique<boost::asio::serial_port>(io_service_);
	try {
		port_->open(portID);
	}
	catch (boost::system::system_error& ex) {
		throwNested("Error in reconnecting BoostAsyncSerial.");
	}
	port_->set_option(boost::asio::serial_port_base::baud_rate(baudrate));
	port_->set_option(boost::asio::serial_port_base::character_size(8));
	port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
	port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
	port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

	io_thread = boost::thread(boost::bind(&BoostAsyncSerial::run, this));
	read();
}

void BoostAsyncSerial::read()
{
	if (safemode) {
		return;
	}
	if (!port_->is_open()) {
		thrower("Serial port has not been opened");
	}
	port_->async_read_some(boost::asio::buffer(readbuffer), boost::bind(&BoostAsyncSerial::readhandler,
		this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
	));
}

void BoostAsyncSerial::run()
{
	//work = std::make_unique<boost::asio::io_service::work>(io_service_);
	//work = boost::asio::make_work_guard(io_service_); // assigment operator is private, but not for copy. 
	work = std::make_unique<work_guard>(boost::asio::make_work_guard(io_service_));
	io_service_.run();
}
