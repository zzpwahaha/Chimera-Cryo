#pragma once
#include "GeneralObjects/IDeviceCore.h"
#include "GeneralFlumes/BoostAsyncSerial.h" 
#include "DigitalOutput/DoCore.h"
#include <GigaMOOG/MessageSender.h>


class GigaMoogCore :  public IDeviceCore
{
public:
	// THIS CLASS IS NOT COPYABLE.
	GigaMoogCore& operator=(const GigaMoogCore&) = delete;
	GigaMoogCore(const GigaMoogCore&) = delete;

	GigaMoogCore(std::string portID, int baudrate);

	std::string getSettingsFromConfig(ConfigStream& configStream); //used in GigaMoogSystem::handleOpenConfig and ConfigSystem::stdGetFromConfig
	void loadExpSettings(ConfigStream& stream) override; // update fileAddress
	void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
	void programVariation(unsigned variation, std::vector<parameterType>& params,
		ExpThreadWorker* threadworker) override;
	//void analyzeGMoogScript()
	void normalFinish() override {};
	void errorFinish() override {};
	std::string getDelim() override { return configDelim; };

	void programGMoogNow(std::string fileAddr, std::vector<parameterType> constants, DoCore& doCore, DOStatus dostatus);
	void disconnectPort();
	void reconnectPort();
private:
	//Attempt to parse moog script 
	void analyzeMoogScript(std::string fileAddr, std::vector<parameterType>& variables, UINT variation);
	void writeOff(MessageSender& ms);
	void send(MessageSender& ms);


public:
	const std::string configDelim = "GMOOG";
	BoostAsyncSerial fpga;
	std::string fileAddress;


private:
	std::vector<MessageSender> gigaMoogCommandList;




};

