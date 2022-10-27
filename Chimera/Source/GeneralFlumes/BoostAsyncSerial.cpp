#include "stdafx.h"
#include "BoostAsyncSerial.h"

BoostAsyncSerial::BoostAsyncSerial(std::string portID, int baudrate)
	: portID(portID)
	, baudrate(baudrate)
{
	if (!GIGAMOOG_SAFEMODE) {
		//io_service_ = std::make_unique<boost::asio::io_service>(boost::asio::io_service());
		port_ = std::make_unique<boost::asio::serial_port>(io_service_);

		port_->open(portID);
		port_->set_option(boost::asio::serial_port_base::baud_rate(baudrate));
		port_->set_option(boost::asio::serial_port_base::character_size(8));
		port_->set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
		port_->set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
		port_->set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

		io_thread = boost::thread(boost::bind(&BoostAsyncSerial::run, this));
		read();
	}

}

BoostAsyncSerial::BoostAsyncSerial(
	std::string portID,
	int baudrate,
	int character_size,
	boost::asio::serial_port_base::stop_bits::type stop_bits,
	boost::asio::serial_port_base::parity::type parity,
	boost::asio::serial_port_base::flow_control::type flow_control) 
	: portID(portID)
	, baudrate(baudrate)
{
	port_ = std::make_unique<boost::asio::serial_port>(io_service_);

	port_->open(portID);
	port_->set_option(boost::asio::serial_port_base::baud_rate(baudrate));
	port_->set_option(boost::asio::serial_port_base::character_size(character_size));
	port_->set_option(boost::asio::serial_port_base::stop_bits(stop_bits));
	port_->set_option(boost::asio::serial_port_base::parity(parity));
	port_->set_option(boost::asio::serial_port_base::flow_control(flow_control));

	io_thread = boost::thread(boost::bind(&BoostAsyncSerial::run, this));
	read();
}

BoostAsyncSerial::~BoostAsyncSerial()
{
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
		thrower("Error reading from serial");
	}
	
	port_->async_read_some(boost::asio::buffer(readbuffer), boost::bind(&BoostAsyncSerial::readhandler, this,
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
	));

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
	if (!port_->is_open()) {
		thrower("Serial port has not been opened");
	}
	boost::asio::write(*port_, boost::asio::buffer(data));
}

void BoostAsyncSerial::write(std::vector<int> data)
{
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
	if (GIGAMOOG_SAFEMODE) {
		return;
	}
	if (!port_) {
		thrower("port_ is already closed. Can not disconnect again");
	}
	try {
		io_service_.stop();
		if (port_) {
			port_->cancel();
			boost::system::error_code ec;
			port_->close(ec);
			auto s = ec.message();
		}
		if (port_->is_open()) {
			thrower("After port->close(), the port is still open??");
		}
	}
	catch (boost::system::system_error& ex) {
		throwNested(ex.what());
	}
	//io_service_.post([this]() {
	//	io_service_.stop();
	//	port_->close();
	//	work->reset(); });
	//port_.reset();
	//work.reset();
	//io_service_.restart();
	//io_thread.detach();
	work->reset();
	//io_thread.interrupt();
	io_thread.join();
	io_service_.restart();
}

void BoostAsyncSerial::reconnect()
{
	if (GIGAMOOG_SAFEMODE) {
		return;
	}
	if (port_ && port_->is_open()/*io_service_.stopped()*/) {
		thrower("port_ is already open. Can not connect again");
	}
	io_service_.restart();

	port_ = std::make_unique<boost::asio::serial_port>(io_service_);
	try {
		port_->open(portID);
	}
	catch (boost::system::system_error& ex) {
		throwNested(ex.what());
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
