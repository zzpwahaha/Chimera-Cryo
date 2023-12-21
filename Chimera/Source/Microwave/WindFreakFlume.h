#pragma once
#include <string>
#include <GeneralFlumes/BoostAsyncSerial.h>
#include <Microwave/microwaveSettings.h>

class WindFreakFlume{
	public:
		WindFreakFlume (std::string portAddress, bool safemode);
		std::string query(std::string msg);
		void write(std::string msg);
		std::string read();
		std::string queryIdentity ();
		void resetConnection();
		void setPmSettings ();
		void setFmSettings ();
		void programSingleSetting (microwaveListEntry setting, unsigned varNumber);
		void programList (std::vector<microwaveListEntry> list, unsigned varNum, double triggerTime);
		std::string getListString ();
		const bool SAFEMODE;
	private:
		void readCallback(int byte);
		void errorCallback(std::string error);
		BoostAsyncSerial boostFlume;
		std::atomic<bool> readComplete;
		std::vector<unsigned char> readRegister;
		std::string errorMsg;

};
