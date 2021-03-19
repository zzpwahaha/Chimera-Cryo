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

void QtSocketFlume::write(QByteArray ba, QByteArray terminator)
{
	if (!safemode) {
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

void QtSocketFlume::write(std::string msg, QByteArray terminator)
{
	QByteArray ba = QByteArray::fromStdString(msg);
	write(ba, terminator);

}

void QtSocketFlume::write(const char* msg, QByteArray terminator)
{
	write(QByteArray(msg), terminator);
}

void QtSocketFlume::resetConnection()
{
	close();
	open();
}

//this is here for the raw binary data which I thought for string would be truncated if there there is a zero in 
//the qbytearray, but it turns out that the zero in the array does not truncate the std::string, so can just read out as string
QByteArray QtSocketFlume::readRaw()
{
	/*have to use this funny way to get the feedback properly. May be don't have to have the feedback*/
	bool dataready = false;
	for (size_t i = 0; i < 100; i++) {
		if (socket.waitForReadyRead(10) || socket.bytesAvailable() != 0) {
			dataready = true;
			break;
		}
	}
	if (dataready)
	{
		QByteArray ba = socket.readAll();
		if (!ba.isEmpty()) {
			return ba;
		}
		thrower("No data to read in socket "
			+ str(socket.peerAddress().toString()) + "port, " + str(socket.peerPort()) + " after 1000 ms.");
		return QByteArray();
	}
	else {
		thrower("No data to read in socket "
			+ str(socket.peerAddress().toString()) + "port, " + str(socket.peerPort()) + " after 1000 ms.");
		return QByteArray();
	}

}

/*read untill the buffer is filled in with the expected number of data. Used when reading numerial data
  with deterministic size. Clear buffer before write and read */
QByteArray QtSocketFlume::readTillFull(unsigned size)
{
	unsigned bytesRemaining = size;
	QByteArray rc;
	int aa = socket.bytesAvailable();
	while (socket.bytesAvailable() < bytesRemaining) {
		bool dataready = false;
		for (size_t i = 0; i < 100; i++) {
			aa = socket.bytesAvailable();
			if (socket.waitForReadyRead(10) || aa != 0) {
				dataready = true;
				break;
			}
		}
		if (!dataready) {
			thrower("No data to read in socket "
				+ str(socket.peerAddress().toString()) + "port, " + str(socket.peerPort()) + " after 1000 ms.");
			return rc;
		}
		//aa = socket.bytesAvailable();
		// calling read() with the bytesRemaining argument will not guarantee
		// that you will receive all the data. It only means that you will 
		// receive AT MOST bytesRemaining bytes.
		rc.append(socket.read(bytesRemaining));
		//aa=socket.bytesAvailable();
		bytesRemaining = size - rc.size();
	}
	return rc;
}

std::string QtSocketFlume::read()
{
	return readRaw().toStdString();
}

std::string QtSocketFlume::query(std::string msg, QByteArray terminator)
{
	write(msg, terminator);
	return read();
}

