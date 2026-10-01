#pragma once
#include "GeneralObjects/IDeviceCore.h"
#include <GeneralFlumes/BoostUDP.h>
#include "DigitalOutput/DoCore.h"
#include <Scripts/ScriptStream.h>
#include "SynthesizerSequences.h"
#include "GigaMoog420Packets.h"

// Core for the 32-bit GigaMoog420 box (ZCU106 / YJ205 timestamped sequencer, 4 channels).
// Unlike GigaMoogCore, this box does not use the KA007 message protocol in Messagetypes.h.
class GigaMoog420Core : public IDeviceCore
{
public:
	// THIS CLASS IS NOT COPYABLE.
	GigaMoog420Core& operator=(const GigaMoog420Core&) = delete;
	GigaMoog420Core(const GigaMoog420Core&) = delete;

	GigaMoog420Core(bool safemode, std::string IPAddress, int port);

	std::string getSettingsFromConfig(ConfigStream& configStream); //used in GigaMoogSystem::handleOpenConfig420 and ConfigSystem::stdGetFromConfig
	void loadExpSettings(ConfigStream& stream) override; // update fileAddress
	void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
	void programVariation(unsigned variation, std::vector<parameterType>& params,
		ExpThreadWorker* threadworker) override;
	void normalFinish() override {};
	void errorFinish() override {};
	std::string getDelim() override { return configDelim; };

	// doCore/dostatus are unused for this box but kept so GigaMoogSystem's calls don't change.
	void programGMoogNow(std::string fileAddr, std::vector<parameterType> constants, DoCore& doCore, DOStatus dostatus);
	void resetMemory(DoCore& doCore, DOStatus dostatus);
	void softwareTrigger();
	// Hex dump of the datagrams sent by the last programGMoogNow / programVariation call,
	// one per line, for reporting in the GUI.
	std::string getLastSentHex(size_t maxLines = 64) const;
	void disconnectPort() {};
	void reconnectPort() {};

private:
	using SequenceMap = std::map<int, std::vector<std::shared_ptr<synth::RFBlock>>>;

	// Parse the script file for one variation and append its table packets to commandList.
	void analyzeMoogScript(std::string fileAddr, std::vector<parameterType>& variables, unsigned variation);
	bool analyzeMoogScript(std::string word, ScriptStream& script, SequenceMap& sequence,
		gm420::PacketList& rawPackets, std::vector<parameterType>& variables, unsigned variation);

	void sendReset(bool resetOutputs);
	void sendPackets(const gm420::PacketList& packets);
	// Parse one "bit" token (hex, optional 0x prefix, or binary with 0b prefix) into one datagram.
	static gm420::Packet parseBitString(const std::string& token);

public:
	const std::string configDelim = "GMOOG420";

private:
	BoostUDP fpga;
	std::string fileAddress;
	std::vector<gm420::PacketList> commandList; // [variation] -> every datagram to send, in order
	gm420::PacketList lastSent;                 // what the last program call sent, for reporting

	friend class CruncherThreadWorker;
};