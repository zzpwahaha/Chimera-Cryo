#include "stdafx.h"
#include <boost/lexical_cast.hpp>
#include "WindFreakFlume.h"
#include <algorithm>

WindFreakFlume::WindFreakFlume(std::string portAddress, bool safemode)
	: boostFlume(safemode, portAddress, 9600)
	, SAFEMODE(safemode)
	, readComplete(true)
	// From WindFreak programming API: 
	// All the legacy COM port settings such as Baud Rate, Data bits, Stop bits, etc. are “don’t cares”. 
	// All communication will go at full USB speed no matter what these settings are.
{
	boostFlume.setReadCallback(boost::bind(&WindFreakFlume::readCallback, this, _1));
	boostFlume.setErrorCallback(boost::bind(&WindFreakFlume::errorCallback, this, _1));
}

std::string WindFreakFlume::query(std::string msg)
{
	if (SAFEMODE) {
		return std::string("WINDFREAK is in safemode.");
	}
	std::string recv;
	write(msg);
	/*read register after write*/
	for (auto idx : range(200)) {
		if (readComplete) {
			recv = std::string(readRegister.cbegin(), readRegister.cend());
			break;
		}
		Sleep(1);
	}
	/*check reading error and reading result and if reading is complete*/
	if (recv.empty() || !readComplete) {
		thrower("Reading is empty and timed out for 200ms in writing to windfreak serial port " + str(boostFlume.portID) +
			". The wrote message is: " + msg);
	}
	if (!errorMsg.empty()) {
		thrower("Nothing feeded back from Windfreak after writing: " + msg + ", something might be wrong with it." + recv + "\r\nError message: " + errorMsg);
	}
	recv.erase(std::remove(recv.begin(), recv.end(), '\n'), recv.end());
	return recv;
}

void WindFreakFlume::write(std::string msg)
{
	readRegister.clear();
	errorMsg.clear();
	readComplete = false;
	/*write data to serial port*/
	std::vector<unsigned char> byteMsg(msg.cbegin(), msg.cend());
	boostFlume.write(byteMsg);
	/*check exception after write*/
	if (auto e = boostFlume.lastException()) {
		try {
			boost::rethrow_exception(e);
		}
		catch (boost::system::system_error& e) {
			throwNested("Error seen in writing to serial port " + str(boostFlume.portID) + ". Error: " + e.what());
		}
	}
}

// This should be called with a expectation of reading something, e.g. after writing and that is why the reading register etc is not initialized. 
// Otherwise it will throw
std::string WindFreakFlume::read()
{
	if (SAFEMODE) {
		return std::string("WINDFREAK is in safemode.");
	}
	std::string recv;
	/*read register after write*/
	for (auto idx : range(200)) {
		if (readComplete) {
			recv = std::string(readRegister.cbegin(), readRegister.cend());
			break;
		}
		Sleep(1);
	}
	/*check reading error and reading result and if reading is complete*/
	if (recv.empty() || !readComplete) {
		thrower("Reading is empty and timed out for 200ms in reading from windfreak serial port " + str(boostFlume.portID) + ".");
	}
	if (!errorMsg.empty()) {
		thrower("Nothing feeded back from Windfreak, something might be wrong with it." + recv + "\r\nError message: " + errorMsg);
	}
	recv.erase(std::remove(recv.begin(), recv.end(), '\n'), recv.end());
	return recv;
}

void WindFreakFlume::readCallback(int byte)
{
	if (byte < 0 || byte >255) {
		thrower("Byte value readed needs to be in range 0-255.");
	}
	readRegister.push_back(byte);
	if (byte == '\n') {
		readComplete = true;
	}
}

void WindFreakFlume::errorCallback(std::string error)
{
	errorMsg = error;
}


std::string WindFreakFlume::queryIdentity (){
	auto model = query ("+");
	return "WindFreak Model/SN: " + model;
}

void WindFreakFlume::resetConnection()
{
	boostFlume.disconnect();
	Sleep(10);
	boostFlume.reconnect();
}

void WindFreakFlume::setPmSettings (){
	//write ("C0");
	// todo. I think modulation like this is possible though.
}

void WindFreakFlume::setFmSettings (){
	//write ("C0");
	// todo
}

void WindFreakFlume::programSingleSetting (microwaveListEntry setting, unsigned varNumber){
	std::string answer;
	write ("C0");
	write ("E1");
	write ("Ld");	// delete prev list
	write ("w0");
	write("f" + str(setting.frequency.getValue(varNumber), 7, false, false, true));
	write("W" + str(setting.power.getValue(varNumber), 3, false, false, true));
}

void WindFreakFlume::programList (std::vector<microwaveListEntry> list, unsigned varNum, double triggerTime){
	//wfFlume.reconnect();
	// Settings for RfoutA
	std::string answer;
	write ("C0");
	unsigned count = 0;
	// delete prev list
	write ("Ld");
	for (auto entry : list)	{
		auto ln = "L" + str (count);
		write(ln + "f" + str(entry.frequency.getValue(varNum), 7, false, false, true));
		write(ln + "a" + str(entry.power.getValue(varNum), 3, false, false, true));
		count++;
	}
	// lock the pll (probably already locked but ok)
	write ("E1");
	// set trigger mode
	write ("w2");
	// set sweep mode to "list"
	write ("X1");
	// set trigger time
	write ("t" + str(triggerTime, 3));
	// sweep single sweep 
	write ("c0");
}

std::string WindFreakFlume::getListString () {
	std::string answer = "";
	unsigned count = 0;
	//auto misc_msg2 = read ();
	while (true) {
		auto res = query ("L" + str (count) + "f?");
		try {
			res.erase (std::remove (res.begin (), res.end (), '\n'), res.end ());
			auto freq = boost::lexical_cast<double>(res);
			if (freq == 0) {
				break;
			}
		}
		catch (boost::bad_lexical_cast&) {
			throwNested ("Bad Frequency Value From Windfreak??? String was: \"" + res + "\"");
		}
		answer += str (count + 1) + ". ";
		answer += res + ", ";
		res = query ("L" + str (count) + "a?");
		res.erase (std::remove (res.begin (), res.end (), '\n'), res.end ());
		answer += res + "; ";
		count++;
		if (count > 100) {
			break;
		}
	}
	return answer;
}

