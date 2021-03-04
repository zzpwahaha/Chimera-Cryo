#include <stdafx.h>
#include <GeneralFlumes/QtSerialFlume.h>

QtSerialFlume::QtSerialFlume(bool safemode_option, std::string portAddress_)
	: safemode(safemode_option)
	, portAddress(portAddress_)
{
	info = QSerialPortInfo(port);
	/*Default settings from Qt:https://doc.qt.io/qt-5/qserialport.htm Property Documentation
	Baudrate: 9600; Databits: 8; FlowControl: None; Parity: None; Stopbits: One*/
	open(portAddress);
}

void QtSerialFlume::open(std::string fileAddr) {
	port.setPortName(qstr(portAddress));
	port.setBaudRate(QSerialPort::Baud9600);
	port.setDataBits(QSerialPort::Data8);
	port.setParity(QSerialPort::NoParity);
	port.setStopBits(QSerialPort::OneStop);
	port.setFlowControl(QSerialPort::NoFlowControl);
	if (!safemode) 
	{
		if (!port.open(QIODevice::ReadWrite)) 
		{
			QStringList ava_names;
			for (auto& p : info.availablePorts()) { ava_names.push_back(p.portName()); }
			thrower("Failed to open QT serial flume for "
				+ str(port.portName()) + ", error" + str(port.errorString()) +
				". Available ports are: " + str(ava_names.join(", ")));
		}
		port.clear();
		port.clearError();
	}

}

void QtSerialFlume::close () 
{
	port.close ();
}

void QtSerialFlume::write(std::string msg) 
{
	if (!safemode) {
		QByteArray ba = QString::fromStdString(msg).toUtf8();
		int err = port.write(ba);
		if (err == -1) {
			thrower("Failed to write to port "
				+ str(port.portName()) + ", error" + str(port.errorString()));
		}
		else if (err < ba.size()) {
			thrower("Failed to write full data to port "
				+ str(port.portName()) + ", error" + str(port.errorString()));
		}
	}
	
}

void QtSerialFlume::resetConnection() 
{
	close();
	open(portAddress);
}

std::string QtSerialFlume::read () 
{
	/*have to use this funny way to get the feedback properly. May be don't have to have the feedback*/
	bool dataready = false;
	for (size_t i = 0; i < 50; i++) {
		if (port.waitForReadyRead(10) || port.bytesAvailable() != 0) {
			dataready = true;
			break;
		}
	}

	//bool ready = port.waitForReadyRead(50);
	//bool ready2 = port.waitForReadyRead(30);
	//bool ready3 = port.waitForReadyRead(20);
	//bool ready4 = port.waitForReadyRead(150);
	//int numbyte = port.bytesAvailable();
	if (dataready)
	{
		QByteArray ba = port.readAll();
		QString qs = QString::fromUtf8(ba);
		if (!qs.isEmpty()) {
			port.clear();
			return str(qs); 
		}
		thrower("No data to read in serial COM: " + str(portAddress) + " after 45 ms.");
		return std::string("");
	}
	else {
		thrower("No data to read in serial COM: " + str(portAddress) + " after 45 ms.");
		return std::string("");
	}
}

std::string QtSerialFlume::query(std::string msg) 
{
	write(msg);
	return read();
}
