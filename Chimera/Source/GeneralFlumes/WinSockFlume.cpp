#include "stdafx.h"
#include "WinSockFlume.h"
#include <qdebug.h>


WinSockFlume::WinSockFlume(bool safemode_option, std::string hostAddress_, std::string hostPort_)
	: safemode(safemode_option)
	, hostAddress(hostAddress_)
	, hostPort(hostPort_)
	, Winsocket(INVALID_SOCKET)
{
	//open();
	//Sleep(1);
	//close();
}

WinSockFlume::~WinSockFlume()
{
	close();
}


int WinSockFlume::open()
{
	if (safemode) {
		return 0;
	}

	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		thrower("WSAStartup failed");
		return 1;
	}

	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(hostAddress.c_str(), hostPort.c_str(), &hints, &result);
	if (iResult != 0) {
		thrower("getaddrinfo failed");
		WSACleanup();
		return 1;
	}

	Winsocket = INVALID_SOCKET;
	// Attempt to connect to the first address returned by the call to getaddrinfo
	ptr = result;
	// Create a SOCKET for connecting to server
	Winsocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (Winsocket == INVALID_SOCKET) {
		thrower("Error at socket()");
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Connect to server.
	iResult = ::connect(Winsocket, ptr->ai_addr, (int)ptr->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		closesocket(Winsocket);
		Winsocket = INVALID_SOCKET;
	}
	freeaddrinfo(result);
	if (Winsocket == INVALID_SOCKET) {
		thrower("Unable to connect to server!");
		WSACleanup();
		return 1;
	}
	else
	{
		return 0;
	}

}

void WinSockFlume::close()
{
	if (safemode) {
		return;
	}
	closesocket(Winsocket);
	WSACleanup();
}

void WinSockFlume::resetConnection()
{
	close();
	open();
}

int WinSockFlume::sendWithCatch(const SOCKET& socket, const char* byte_buf, int buffLen, int flags)
{
	if (safemode) {
		return 0;
	}
	int BytesSent;
	BytesSent = send(socket, byte_buf, buffLen, flags);
	if (BytesSent == SOCKET_ERROR)
	{
		thrower("Unable to send message:" + str(byte_buf) + ", with prescribed length:" + str(buffLen) + " to server!");
		return 1; /*bad*/
	}
	return 0; /*good*/
}

void WinSockFlume::write(const char* msg, unsigned len, QByteArray terminator)
{
	if (safemode) {
		return;
	}
	try {
		QByteArray buff = QByteArray(msg, len);
		buff.append(terminator);
		int err = sendWithCatch(Winsocket, (const char*)buff, len + terminator.size());
		qDebug() << "Send WinSockFlume message: " << buff;
	}
	catch (ChimeraError& e) {
		throwNested(str("Error in winSockFlume write:\n"));
	}
}

void WinSockFlume::write(QByteArray ba, QByteArray terminator)
{
	write(ba, ba.size(), terminator);
}

int WinSockFlume::recvTimeOutTCP(SOCKET socket, unsigned sec, unsigned usec)

{
	// Setup timeval variable
	struct timeval timeout;
	struct fd_set fds;
	// assign the second and microsecond variables
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;

	// Setup fd_set structure
	FD_ZERO(&fds);
	FD_SET(socket, &fds);
	// Possible return values:
	// -1: error occurred
	// 0: timed out
	// > 0: data ready to be read
	return select(0, &fds, 0, 0, &timeout);

}

QByteArray WinSockFlume::readTillFull(unsigned size)
{
	QByteArray rc;
	//unsigned bytesRemaining = size;
	for (size_t i = 0; i < 2000; i++)
	{
		QByteArray tmp(size, Qt::Initialization::Uninitialized);

		int SelectTiming = recvTimeOutTCP(Winsocket, 5, 10);
		switch (SelectTiming)
		{
		case 0:
			close();
			thrower("Timeout in winsock TCP reading after 5ms");
			break;
		case 1:
			break;
		default:
			break;
		}
		// set timeout and see if socket is valid after recv, it will be invalid if revc is timed out
		//int timeout_ms = 20;
		//setsockopt(Winsocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
		int recvLen = recv(Winsocket, tmp.data(), size, 0);
		//int err = WSAGetLastError();
		//if (err != 0) {
		//	close();
		//	thrower("Error in TCP receiving data with error code" + str(err));
		//}
		rc.append((const char*)tmp, recvLen);
		if (rc.size() >= size) {
			qDebug() << "Finish WinSockFlume::readTillFull in " << i + 1 << "round";
			break;
		}
		Sleep(1);
	}
	if (rc.size() != size) {
		thrower("Data size " + str(rc.size()) + " is not expected as " + str(size) 
			+ " in socket " +  hostAddress + "port, " + hostPort+ " after 2000 ms.");
	}
	return rc;
}

QByteArray WinSockFlume::read(unsigned size)
{
	if (safemode) {
		return "Error! SafeMode!!";
	}
	QByteArray tmp(size, Qt::Initialization::Uninitialized);
	int timeout_ms = 500;
	setsockopt(Winsocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout_ms, sizeof(timeout_ms));
	int recvLen = recv(Winsocket, tmp.data(), size, 0);
	int test = sizeof(timeout_ms);
	if (SOCKET_ERROR == getsockopt(Winsocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout_ms, &test)) {
		int errCode = WSAGetLastError();
		close();
		thrower("Error in TCP receiving data with error code " + str(errCode));
	}
	tmp.truncate(recvLen);
	qDebug() << "Read WinSockFlume message: " << tmp;
	return tmp;
}
