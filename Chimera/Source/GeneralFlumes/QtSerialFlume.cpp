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
	}

}

void QtSerialFlume::close () 
{
	port.close ();
}

void QtSerialFlume::write(std::string msg) 
{
	if (!safemode) {
		auto ba = qstr(msg).toUtf8();
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
	auto ba = port.read (1064);
	QString qs = QString::fromUtf8 (ba);
	return str(qs);
}

std::string QtSerialFlume::query(std::string msg) 
{
	write(msg);
	return read();
}
