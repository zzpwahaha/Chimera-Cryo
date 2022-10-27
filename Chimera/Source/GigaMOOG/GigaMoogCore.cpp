#include "stdafx.h"
#include "GigaMoogCore.h"
#include <ExperimentThread/ExpThreadWorker.h>
#include <ConfigurationSystems/ConfigSystem.h>
#include <DataLogging/DataLogger.h>
#include "MessagePrinter.h" 
#include "MessageSender.h" 

GigaMoogCore::GigaMoogCore(std::string portID, int baudrate)
	: fpga(portID, baudrate)
{

}

std::string GigaMoogCore::getSettingsFromConfig(ConfigStream& configStream)
{
	configStream >> experimentActive;
	std::string addr;
	configStream >> addr;
	return addr;
}

void GigaMoogCore::loadExpSettings(ConfigStream& stream)
{
	ConfigSystem::stdGetFromConfig(stream, *this, fileAddress);
}

void GigaMoogCore::logSettings(DataLogger& logger, ExpThreadWorker* threadworker)
{
	try {
		H5::Group GMoogGroup;
		try {
			GMoogGroup = logger.file.createGroup("/GIGAMOOG");
		}
		catch (H5::Exception&) {
			GMoogGroup = logger.file.openGroup("/GIGAMOOG");
		}

		H5::Group GMOOGScipt(GMoogGroup.createGroup("GMOOGScipt"));
		logger.writeDataSet(fileAddress, "Script-Address", GMOOGScipt);
		ScriptStream stream;
		try {
			ExpThreadWorker::loadGMoogScript(fileAddress, stream);
			logger.writeDataSet(stream.str(), "GigaMoog-Script", GMOOGScipt);
		}
		catch (ChimeraError&) {
			// failed to open, that's probably fine, 
			logger.writeDataSet("Script Failed to load.", "GigaMoog-Script", GMOOGScipt);
		}

	}
	catch (H5::Exception err) {
		logger.logError(err);
		throwNested("ERROR: Failed to log GIGAMOOG parameters in HDF5 file: " + err.getDetailMsg());
	}
}

void GigaMoogCore::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	unsigned variations = params.size() == 0 ? 1 : params.front().keyValues.size();
	if (variations == 0) {
		variations = 1;
	}
	/// imporantly, this sizes the relevant structures.
	gigaMoogCommandList.clear();
	//gigaMoogCommandList.resize(variations);
	for (auto variationInc : range(variations)) {
		analyzeMoogScript(fileAddress, params, variationInc);
	}

}

void GigaMoogCore::programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	send(gigaMoogCommandList[variation]);
}

void GigaMoogCore::programGMoogNow(std::string fileAddr, std::vector<parameterType> constants, DoCore& doCore, DOStatus dostatus)
{
	gigaMoogCommandList.clear();
	//gigaMoogCommandList.resize(1);
	analyzeMoogScript(fileAddr, constants, 0);
	send(gigaMoogCommandList[0]);
	Sleep(100);
	doCore.FPGAForcePulse(dostatus, std::vector<std::pair<unsigned, unsigned>>{GM_TRIGGER_LINE[0]}, GM_TRIGGER_TIME);
}

void GigaMoogCore::disconnectPort()
{
	fpga.disconnect();
}

void GigaMoogCore::reconnectPort()
{
	fpga.reconnect();
}

void GigaMoogCore::analyzeMoogScript(std::string fileAddr, std::vector<parameterType>& variables, UINT variation)
{
	ScriptStream currentMoogScript;
	ExpThreadWorker::loadGMoogScript(fileAddr, currentMoogScript);

	MessageSender ms;

	writeOff(ms);

	if (currentMoogScript.str() == "") {
		thrower("ERROR: Moog script is empty!\r\n");
	}
	std::string word;
	currentMoogScript >> word;
	std::vector<UINT> totalRepeatNum, currentRepeatNum;
	std::vector<std::streamoff> repeatPos;
	// the analysis loop. 

	while (!(currentMoogScript.peek() == EOF) || word != "__end__")
	{
		if (word == "set") {
			std::string DAC;
			Expression channel, amplitude, frequency, phase;
			currentMoogScript >> DAC;
			currentMoogScript >> channel;
			currentMoogScript >> amplitude;
			currentMoogScript >> frequency;
			currentMoogScript >> phase;

			MessageDAC dacset;
			if (DAC == "dac0") {
				dacset = MessageDAC::DAC0;
			}
			else if (DAC == "dac1") {
				dacset = MessageDAC::DAC1;
			}
			else if (DAC == "dac2") {
				dacset = MessageDAC::DAC2;
			}
			else if (DAC == "dac3") {
				dacset = MessageDAC::DAC3;
			}
			else {
				thrower("ERROR: unrecognized moog DAC selection: \"" + DAC + "\"");
			}

			Message m = Message::make().destination(MessageDestination::KA007)
				.DAC(dacset).channel(channel.evaluate(variables, variation))
				.setting(MessageSetting::LOADFREQUENCY)
				.frequencyMHz(frequency.evaluate(variables, variation)).amplitudePercent(amplitude.evaluate(variables, variation)).phaseDegrees(phase.evaluate(variables, variation));;
			ms.enqueue(m);
		}
		else
		{
			thrower("ERROR: unrecognized moog script command: \"" + word + "\"");
		}
		word = "";
		currentMoogScript >> word;
	}

	{
		Message m = Message::make().destination(MessageDestination::KA007)
			.setting(MessageSetting::TERMINATE_SEQ);
		ms.enqueue(m);
	}

	//send(ms);
	gigaMoogCommandList.push_back(ms);

}


void GigaMoogCore::writeOff(MessageSender& ms) {

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC0).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC1).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC2).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	for (int channel = 0; channel < 64; channel++) {
		Message m = Message::make().destination(MessageDestination::KA007)
			.DAC(MessageDAC::DAC3).channel(channel)
			.setting(MessageSetting::LOADFREQUENCY)
			.frequencyMHz(0).amplitudePercent(0).phaseDegrees(0.0);;
		ms.enqueue(m);
	}

	//{ 
	//	Message m = Message::make().destination(MessageDestination::KA007) 
	//		.setting(MessageSetting::TERMINATE_SEQ); 
	//	ms.enqueue(m); 
	//} 

	//ms.getQueueElementCount(); 
	//MessagePrinter rec; 
	//fpga.setReadCallback(boost::bind(&MessagePrinter::callback, rec, _1)); 
	//fpga.write(ms.getMessageBytes()); 
}

void GigaMoogCore::send(MessageSender& ms)
{
	ms.getQueueElementCount();
	//MessagePrinter rec; 
	//fpga.setReadCallback(boost::bind(&MessagePrinter::callback, rec, _1)); 
	fpga.write(ms.getMessageBytes());
}
