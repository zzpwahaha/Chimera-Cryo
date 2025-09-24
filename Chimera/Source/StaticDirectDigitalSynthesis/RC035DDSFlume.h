#pragma once
#include <string>
#include <GeneralFlumes/BoostAsyncSerial.h>
#include <StaticDirectDigitalSynthesis/MessageTypesRC035.h>

class RC035DDSFlume {
public:
	RC035DDSFlume(std::string portAddress, unsigned baudrate, bool safemode);
	std::string query(std::string msg);
	void write(std::string msg);
	void write(std::vector<unsigned char> msg);
	std::string read();
	void resetConnection();
	const bool SAFEMODE;

	void configureDDS();
	void setDDSFreq(double freqMHz, int channel);
	
private:
	void setResetDDS1(double freqMHz, double ampPercent, double phaseDeg);
	void setResetDDS2(double freqMHz, double ampPercent, double phaseDeg);
	void initializeDDS(bool sync_enable = true, bool lock_pll = false, bool profile_mode = false);
	void setModulationOn();
	void setModulationOff();
	void resetDDS();
	void setTimestamp(uint32_t timestamp,
		double DDS1_startfreq, double DDS1_stopfreq,
		double DDS1_ramptime, double DDS1_amp,
		double DDS1_phase, bool   DDS1_output_disable,
		double DDS2_startfreq, double DDS2_stopfreq,
		double DDS2_ramptime, double DDS2_amp,
		double DDS2_phase, bool   DDS2_output_disable);




private:
	void readCallback(int byte);
	void errorCallback(std::string error);
	BoostAsyncSerial boostFlume;
	std::atomic<bool> readComplete;
	std::vector<unsigned char> readRegister;
	std::string errorMsg;

};