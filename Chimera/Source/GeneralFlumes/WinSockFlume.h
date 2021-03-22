#pragma once
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <qbytearray.h>

class WinSockFlume
{
public:
	WinSockFlume(bool safemode_option, std::string hostAddress_, std::string hostPort_);
	~WinSockFlume();

	int open();
	void close();
	void resetConnection();
	void write(const char* msg, unsigned len, QByteArray terminator);
	void write(QByteArray ba, QByteArray terminator);
	QByteArray readTillFull(unsigned size);
	QByteArray read(unsigned size = READBUFF); // used for casual read that does simple, short communication adn not care about exact size

private:
	int sendWithCatch(const SOCKET& socket, const char* byte_buf, int buffLen, int flags = 0);

public:
	const std::string hostAddress;
	const std::string hostPort;
	const bool safemode;
	static const unsigned READBUFF = 100; // used for casual read that does simple, short communication adn not care about exact size
private:
	SOCKET Winsocket;

};

