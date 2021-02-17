#pragma once

#include <QSerialPort>
#include <QSerialPortInfo>

class QtSerialFlume {
public:
	// THIS CLASS IS NOT COPYABLE.
	QtSerialFlume& operator=(const QtSerialFlume&) = delete;
	QtSerialFlume (const QtSerialFlume&) = delete;

	QtSerialFlume (bool safemode_option, std::string portAddress);
	void open (std::string fileAddr);
	void close ();
	void write (std::string msg);
	void resetConnection ();
	std::string read ();
	std::string query (std::string msg);

	QSerialPort& getPort() { return port; }

private:
	const bool safemode;
	QSerialPort port;
	QSerialPortInfo info;
	std::string portAddress;
};
