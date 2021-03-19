#pragma once
#include <QTcpSocket>
#include <qbytearray.h>

class QtSocketFlume :  public QTcpSocket
{
public:
	// THIS CLASS IS NOT COPYABLE.
	QtSocketFlume& operator=(const QtSocketFlume&) = delete;
	QtSocketFlume(const QtSocketFlume&) = delete;

	QtSocketFlume(bool safemode_option, std::string hostAddress, unsigned short hostPort);
	void open();
	void close();
	void write(QByteArray ba, QByteArray terminator);
	void write(std::string msg, QByteArray terminator);
	void write(const char* msg, QByteArray terminator);
	void resetConnection();
	std::string read();
	QByteArray readRaw();
	QByteArray readTillFull(unsigned size);
	std::string query(std::string msg, QByteArray terminator);

	QTcpSocket& getPort() { return socket; }

private:
	const bool safemode;
	QTcpSocket socket;
	std::string hostAddress;
	unsigned short hostPort;
};

