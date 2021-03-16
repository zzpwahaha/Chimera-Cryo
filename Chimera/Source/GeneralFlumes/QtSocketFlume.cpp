#include "stdafx.h"
#include "QtSocketFlume.h"
#include "qhostaddress.h"
#include <Windows.h> 


QtSocketFlume::QtSocketFlume(bool safemode_option, std::string hostAddress_, unsigned short hostPort_)
	: safemode(safemode_option)
	, hostAddress(hostAddress_)
	, hostPort(hostPort_)
{
	open();
}

void QtSocketFlume::open() {
	if (!safemode)
	{	
		//QString hostAddress = "192.168.107.36";
		//unsigned short hostPort = 55913;
		socket.connectToHost(qstr(hostAddress), hostPort, QTcpSocket::ReadWrite);
		bool con = socket.waitForConnected(5000);
		if (!con || socket.state() != QTcpSocket::ConnectedState) {
			thrower("Error occurred in opening Tcp socket: " + hostAddress + ", port " + str(hostPort)
				+ ". Error code: " + str(socket.errorString()));
		}
	}

}

void QtSocketFlume::close()
{
	socket.disconnectFromHost(); /*same as socket.close()*/
}

void QtSocketFlume::write(std::string msg, QByteArray terminator)
{
	if (!safemode) {
		QByteArray ba = QString::fromStdString(msg).toUtf8();
		ba.append(terminator);
		int err = socket.write(ba);
		bool written = socket.waitForBytesWritten(100);

		if (err == -1) {
			thrower("Failed to write to socket "
				+ str(socket.peerAddress().toString()) + "port, " + str(socket.peerPort())
				+ ", error" + str(socket.errorString()));
		}
		else if (err < ba.size()) {
			thrower("Failed to write full data to socket "
				+ str(socket.peerAddress().toString()) + "port, " + str(socket.peerPort())
				+ ", error" + str(socket.errorString()));
		}
		else if (!written) {
			thrower("Failed to write full data due to time out to socket "
				+ str(socket.peerAddress().toString()) + "port, " + str(socket.peerPort())
				+ ", error" + str(socket.errorString()));
		}
	}

}

void QtSocketFlume::resetConnection()
{
	close();
	open();
}

std::string QtSocketFlume::read()
{
	/*have to use this funny way to get the feedback properly. May be don't have to have the feedback*/
	bool dataready = false;
	for (size_t i = 0; i < 50; i++) {
		if (socket.waitForReadyRead(10) || socket.bytesAvailable() != 0) {
			dataready = true;
			break;
		}
	}
	if (dataready)
	{
		QByteArray ba = socket.readAll();
		QString qs = QString::fromUtf8(ba);
		if (!qs.isEmpty()) {
			return str(qs);
		}
		thrower("No data to read in socket "
			+ str(socket.peerAddress().toString()) + "port, " + str(socket.peerPort()) + " after 500 ms.");
		return std::string("");
	}
	else {
		thrower("No data to read in socket "
			+ str(socket.peerAddress().toString()) + "port, " + str(socket.peerPort()) + " after 500 ms.");
		return std::string("");
	}
}

std::string QtSocketFlume::query(std::string msg, QByteArray terminator)
{
	write(msg, terminator);
	return read();
}

