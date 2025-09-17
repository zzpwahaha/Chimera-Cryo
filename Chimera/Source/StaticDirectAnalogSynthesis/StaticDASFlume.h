#pragma once
#include <string>
#include <GeneralFlumes/BoostAsyncSerial.h>

class StaticDASFlume {
public:
	StaticDASFlume(std::string portAddress, unsigned baudrate, bool safemode);
	std::string query(std::string msg);
	void write(std::string msg);
	std::string read();
	void resetConnection();
	const bool SAFEMODE;
	const std::string commandTerminator = "\r";

private:
	void readCallback(int byte);
	void errorCallback(std::string error);
	BoostAsyncSerial boostFlume;
	std::atomic<bool> readComplete;
	std::vector<unsigned char> readRegister;
	std::string errorMsg;

};