#include "stdafx.h"
#include "GigaMoog420Core.h"
#include <ExperimentThread/ExpThreadWorker.h>
#include <ConfigurationSystems/ConfigSystem.h>
#include <DataLogging/DataLogger.h>
#include <set>
#include <cctype>

// Set to true to dump every datagram to the log while debugging.
static constexpr bool GM420_LOG_PACKETS = false;

GigaMoog420Core::GigaMoog420Core(bool safemode, std::string IPAddress, int port)
	: fpga(safemode, IPAddress, port)
{
}

std::string GigaMoog420Core::getSettingsFromConfig(ConfigStream& configStream)
{
	configStream >> experimentActive;
	std::string addr;
	configStream >> addr;
	return addr;
}

void GigaMoog420Core::loadExpSettings(ConfigStream& stream)
{
	ConfigSystem::stdGetFromConfig(stream, *this, fileAddress);
}

void GigaMoog420Core::logSettings(DataLogger& logger, ExpThreadWorker* threadworker)
{
	try {
		H5::Group GMoogGroup;
		try {
			GMoogGroup = logger.file.createGroup("/GIGAMOOG420");
		}
		catch (H5::Exception&) {
			GMoogGroup = logger.file.openGroup("/GIGAMOOG420");
		}

		H5::Group GMOOGScipt(GMoogGroup.createGroup("GMOOG420Scipt"));
		logger.writeDataSet(fileAddress, "Script-Address", GMOOGScipt);
		ScriptStream stream;
		try {
			ExpThreadWorker::loadGMoogScript(fileAddress, stream);
			logger.writeDataSet(stream.str(), "GigaMoog420-Script", GMOOGScipt);
		}
		catch (ChimeraError&) {
			logger.writeDataSet("Script Failed to load.", "GigaMoog420-Script", GMOOGScipt);
		}

		// The correction data itself is logged, not just its path, since the optimization loop overwrites
		// the files. These are the corrections variation 0 actually used (empty if feedforward was off).
		H5::Group ffGroup(GMoogGroup.createGroup("Corrections"));
		logger.writeDataSet(feedforwardEnabled, "Enabled", ffGroup);
		for (const auto& [label, corr] : lastAppliedCorrections) {
			if (!corr.isLoaded()) continue;
			H5::Group labelGroup(ffGroup.createGroup(label));
			logger.writeDataSet(corr.getPath(), "File-Path", labelGroup);
			logger.writeDataSet(corr.getTimes(), "Tau-s", labelGroup);
			logger.writeDataSet(corr.getGains(), "Gain", labelGroup);
		}
	}
	catch (H5::Exception err) {
		logger.logError(err);
		throwNested("ERROR: Failed to log GIGAMOOG420 parameters in HDF5 file: " + err.getDetailMsg());
	}
}

void GigaMoog420Core::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	unsigned variations = params.size() == 0 ? 1 : params.front().keyValues.size();
	if (variations == 0) {
		variations = 1;
	}
	commandList.clear();
	for (auto variationInc : range(variations)) {
		analyzeMoogScript(fileAddress, params, variationInc);
	}
}

void GigaMoog420Core::programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	sendPackets(commandList[variation]); // includes the reset in normal mode; the trigger does the rest
}

void GigaMoog420Core::programGMoogNow(std::string fileAddr, std::vector<parameterType> constants, DoCore& doCore, DOStatus dostatus)
{
	commandList.clear();
	analyzeMoogScript(fileAddr, constants, 0);
	sendPackets(commandList[0]);
}

void GigaMoog420Core::resetMemory(DoCore& doCore, DOStatus dostatus)
{
	sendReset(true);                     // A400: reset the sequencer and turn the outputs off
}

void GigaMoog420Core::softwareTrigger()
{
	fpga.write(gm420::triggerPacket());  // A200
}

// Script words are lowercased by ScriptStream, so labels are matched case-insensitively.
static std::string normalizeLabel(std::string label)
{
	for (char& c : label) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	if (label.empty()) thrower("ERROR: GigaMoog420 block label is empty.");
	return label;
}

// Table ramps and corrections live in Chimera-Cryo\Ramps. Scripts give just the file name; ".csv" is
// added if it's missing. Returns the full path; name is set to the file name without ".csv".
static const std::string GM420_RAMPS_FOLDER = str(CODE_ROOT) + "\\Ramps\\";

static std::string rampsFilePath(std::string fileName, std::string& name)
{
	if (fileName.find_first_of("/\\:") != std::string::npos) {
		thrower("ERROR: GigaMoog420: give just the file name (\"" + fileName + "\" contains a folder). "
			"Ramp and correction files must be in " + GM420_RAMPS_FOLDER);
	}
	const std::string ext = ".csv";
	if (fileName.size() < ext.size() || fileName.compare(fileName.size() - ext.size(), ext.size(), ext) != 0) {
		fileName += ext;
	}
	name = normalizeLabel(fileName.substr(0, fileName.size() - ext.size()));
	return GM420_RAMPS_FOLDER + fileName;
}

void GigaMoog420Core::loadCorrection(const std::string& label, const std::string& filePath)
{
	gm420::AmplitudeCorrection corr;
	corr.load(filePath);                 // throws before anything is replaced
	corrections[normalizeLabel(label)] = std::move(corr);
}

void GigaMoog420Core::clearCorrection(const std::string& label)
{
	corrections.erase(normalizeLabel(label));
}

void GigaMoog420Core::sendReset(bool resetOutputs)
{
	fpga.write(gm420::resetPacket(resetOutputs));
	if (!fpga.safemode) {
		Sleep(GM420_RESET_WAIT_MS);      // the working Python waits 0.5 s after reset
	}
}

void GigaMoog420Core::sendPackets(const gm420::PacketList& packets)
{
	flog << "GigaMoog420Core: sending " << packets.size() << " datagrams" << fendl;
	for (const auto& p : packets) {
		if (GM420_LOG_PACKETS) {
			flog << "  " << gm420::toHex(p) << fendl;
		}
		fpga.write(p);                   // < 1200 bytes, so BoostUDP sends exactly one datagram
		if (fpga.safemode) {
			continue;
		}
		bool isReset = p.size() == 2 && (p[0] == 0xA3 || p[0] == 0xA4);
		if (isReset) {
			Sleep(GM420_RESET_WAIT_MS);  // the working Python waits 0.5 s after a reset
		}
		else if (GM420_PACKET_DELAY_MS > 0) {
			Sleep(GM420_PACKET_DELAY_MS);
		}
	}
	lastSent = packets;
}

std::string GigaMoog420Core::getLastSentHex(size_t maxLines) const
{
	std::string out;
	for (size_t i = 0; i < lastSent.size() && i < maxLines; ++i) {
		out += "  [" + str(i) + "] " + gm420::toHex(lastSent[i]) + "\r\n";
	}
	if (lastSent.size() > maxLines) {
		out += "  ... " + str(lastSent.size() - maxLines) + " more datagrams\r\n";
	}
	if (fpga.safemode) {
		out += "  (GIGAMOOG420_SAFEMODE is on: nothing was actually sent)\r\n";
	}
	return out;
}

gm420::Packet GigaMoog420Core::parseBitString(const std::string& token)
{
	std::string s;
	for (char c : token) {
		if (c != '_') s += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	}
	gm420::Packet bytes;
	if (s.rfind("0b", 0) == 0) {                         // binary, e.g. 0b1010001100000000
		s = s.substr(2);
		if (s.empty() || s.size() % 8 != 0) {
			thrower("ERROR: GigaMoog420 bit string \"" + token + "\" must have a multiple of 8 binary digits");
		}
		for (size_t i = 0; i < s.size(); i += 8) {
			unsigned value = 0;
			for (size_t j = 0; j < 8; ++j) {
				char c = s[i + j];
				if (c != '0' && c != '1') thrower("ERROR: invalid binary digit in \"" + token + "\"");
				value = (value << 1) | static_cast<unsigned>(c - '0');
			}
			bytes.push_back(static_cast<unsigned char>(value));
		}
	}
	else {                                                // hex, e.g. A300 or 0xA100000000000000
		if (s.rfind("0x", 0) == 0) s = s.substr(2);
		if (s.empty() || s.size() % 2 != 0) {
			thrower("ERROR: GigaMoog420 bit string \"" + token + "\" must have an even number of hex digits");
		}
		auto hexVal = [&](char c) -> unsigned {
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			thrower("ERROR: invalid hex digit '" + std::string(1, c) + "' in \"" + token + "\"");
			return 0;
			};
		for (size_t i = 0; i < s.size(); i += 2) {
			bytes.push_back(static_cast<unsigned char>((hexVal(s[i]) << 4) | hexVal(s[i + 1])));
		}
	}
	if (bytes.size() > 1200) thrower("ERROR: GigaMoog420 bit string is longer than one UDP datagram (1200 bytes)");
	return bytes;
}

void GigaMoog420Core::analyzeMoogScript(std::string fileAddr, std::vector<parameterType>& variables, unsigned variation)
{
	ScriptStream script;
	ExpThreadWorker::loadGMoogScript(fileAddr, script);
	if (script.str() == "") {
		thrower("ERROR: GigaMoog420 script is empty!\r\n");
	}

	SequenceMap sequence;
	gm420::PacketList rawPackets;
	bool useFeedforward = feedforwardEnabled;   // the script may override this with "feedforward 0/1"
	CorrectionMap scriptCorrections;            // from "correction <label> <path>" lines
	std::string word;
	script >> word;
	if (!analyzeMoogScript(word, script, sequence, rawPackets, useFeedforward, scriptCorrections, variables, variation)) {
		thrower("ERROR: unrecognized GigaMoog420 script command: \"" + word + "\"");
	}

	// Raw mode: the script is only "bit" lines. They are sent exactly as written, in order, with no
	// automatic reset and no end entry, like the Python test scripts.
	if (!rawPackets.empty()) {
		if (!sequence.empty()) {
			thrower("ERROR: GigaMoog420 script mixes \"bit\" lines with set/hold/ramp commands. Use one or the other.");
		}
		commandList.push_back(std::move(rawPackets));
		return;
	}

	// Normal mode: reset, then the compiled table. The synth library and GM420Packets.h throw std::
	// exceptions; convert them so the GUI and experiment thread catch them as ChimeraErrors.
	gm420::PacketList packets{ gm420::resetPacket(false) };
	std::string feedforwardWarnings;
	// Script corrections override GUI/API ones with the same label.
	CorrectionMap active;
	if (useFeedforward) {
		active = corrections;
		for (auto& [label, corr] : scriptCorrections) active[label] = std::move(corr);
	}
	if (variation == 0) {
		lastAppliedCorrections = active;
	}
	try {
		synth::CompileResult compiled = synth::compile_sequence(sequence);
		// Corrections act on the compiled rows of labeled blocks, matched by time from each block's start.
		// This runs before tablePackets, so if the first row is corrected, the automatic idle/trigger
		// rows (copies of it) are too.
		feedforwardWarnings = gm420::applyCorrections(compiled, active);
		auto table = gm420::tablePackets(compiled);
		packets.insert(packets.end(), table.begin(), table.end());
	}
	catch (ChimeraError&) {
		throw;                              // applyCorrections reports its own errors
	}
	catch (std::exception& e) {
		thrower("ERROR: GigaMoog420 sequence failed to compile: " + std::string(e.what()));
	}
	if (!feedforwardWarnings.empty()) {
		flog << feedforwardWarnings << fendl;
	}
	//packets.push_back(gm420::triggerPacket());
	commandList.push_back(std::move(packets));
}

// GigaMoog420 script commands. Units: amplitude in %, frequency in MHz, phase in degrees, time in ms.
//   set      <ch> <amp> <freq> <phase> <absPhase>   change the output now
//   settrig  <ch> <amp> <freq> <phase> <absPhase>   wait for a trigger, then change the output
//   trigger  <ch>                                    wait for a trigger, output unchanged
//   hold     <ch> <duration>                         keep the current output for a duration (all durations in milliseconds)
// When the table runs out, the automatic end entry turns the output OFF (amplitude 0, RF switch off).
// To keep a tone on, end with a long hold (max ~1.8e9 ms = 2^48 clock ticks).
//   freqramp <ch> <duration> <amp> <f0> <f1> <steps> (all durations in milliseconds)
//   freqfuncramp <ch> <duration> <amp> <f0> <f1> <shape> <maxPhaseErr>
//			shape: linear | minjerk | sin2 | tanh | tanh:<k>;  maxPhaseErr in rad (e.g. 0.001)
//   ampramp  <ch> <duration> <a0> <a1> <steps>
//   ampfuncramp  <ch> <duration> <a0> <a1> <shape> <maxAmpErr> // same shapes as freqfuncramp
// 
// absPhase = 1 loads the phase absolutely; 0 applies it as a relative (phase-continuous) update.
//
//   tableramp <ch> <name>   play Chimera-Cryo\Ramps\<name>.csv: rows of tau_s, frequency_MHz,
//                           amplitude_percent [, phase_deg], tau from the block start (see synth::TableRamp).
//                           Give only the file name (".csv" optional). The block is labeled <name> unless
//                           a "label" command comes before it.
//
// Block labels and amplitude corrections (see AomFeedforward.h):
//   label <name>                  name the block made by the next command, e.g. "label sweepa" followed by
//                                 "freqfuncramp ...". Labels are case-insensitive and must be unique.
//                                 (Labels can't be written as "@name": the script editor deletes lines that
//                                 start with "@" when it loads a file.)
//   correction <label> <name>     apply Chimera-Cryo\Ramps\<name>.csv (tau_s, gain) to the block labeled
//                                 <label>, tau from the block's start. Give only the file name.
//   feedforward <0|1>             turn all corrections off/on for this script, overriding the GUI setting.
//                                 Use "feedforward 0" while measuring the uncorrected output.
//
// Raw test mode (cannot be mixed with the commands above):
//   bit <datagram>   send these exact bytes as one UDP datagram. Hex (A300, 0xA100000010000000,
//                    underscores allowed: A1_00_0000_00000000) or binary with a 0b prefix.
bool GigaMoog420Core::analyzeMoogScript(std::string word, ScriptStream& script, SequenceMap& sequence,
	gm420::PacketList& rawPackets, bool& useFeedforward, CorrectionMap& scriptCorrections,
	std::vector<parameterType>& variables, unsigned variation)
{
	static const std::set<std::string> commands =
	{ "var", "bit", "set", "settrig", "trigger", "hold", "freqramp", "freqfuncramp", "ampramp", "ampfuncramp",
	  "tableramp", "feedforward", "correction", "label" };
	if (commands.count(word) == 0) {
		return false;
	}
	std::string pendingLabel;                   // set by "label", attached to the next command's block
	std::set<std::string> usedLabels;
	std::string scope = GIGAMOOG_PARAMETER_SCOPE;
	std::string warnings;

	// Read the next token as an expression and evaluate it for this variation.
	auto nextValue = [&]() {
		Expression expr;
		script >> expr;
		expr.assertValid(variables, scope);
		return expr.evaluate(variables, variation);
		};
	auto nextChannel = [&]() {
		int ch = static_cast<int>(std::lround(nextValue()));
		if (ch < 0 || ch >= synth::N_CHANNELS) {
			thrower("ERROR: GigaMoog420 channel " + str(ch) + " out of range 0-" + str(synth::N_CHANNELS - 1));
		}
		return ch;
		};
	constexpr double DEG = PI / 180.0;

	while (!(script.peek() == EOF) || word != "__end__")
	{
		if (word == "label") {
			std::string name;
			script >> name;
			if (!pendingLabel.empty()) {
				thrower("ERROR: GigaMoog420 labels \"" + pendingLabel + "\" and \"" + name + "\" are on the same command.");
			}
			pendingLabel = normalizeLabel(name);
			if (!usedLabels.insert(pendingLabel).second) {
				thrower("ERROR: GigaMoog420 label \"" + pendingLabel + "\" is used more than once.");
			}
			word = "";
			script >> word;
			continue;
		}
		// Snapshot the block counts so a label can be attached to whatever block this command adds.
		std::map<int, size_t> sizesBefore;
		for (const auto& [ch, blocks] : sequence) sizesBefore[ch] = blocks.size();

		if (ExpThreadWorker::handleVariableDeclaration(word, script, variables, scope, warnings)) {}
		else if (word == "bit") {
			std::string token;
			script >> token;
			rawPackets.push_back(parseBitString(token));
		}
		else if (word == "set" || word == "settrig") {
			int ch = nextChannel();
			double amp = nextValue();
			double freq = nextValue();
			double phase = nextValue();
			bool absPhase = nextValue() > 0.5;
			// A plain "set" lasts one clock tick instead of zero. compile_sequence drops zero-duration
			// entries (merging their values into the next one), which would lose the absolute-phase
			// flag. One tick (6.5 ns) keeps it as its own table entry.
			double duration = (word == "set") ? synth::MIN_DURATION : 0.0;
			sequence[ch].push_back(std::make_shared<synth::Timestamp>(
				duration, amp / 100.0, phase * DEG, freq * 1e6,
				/*wait_for_trigger=*/word == "settrig", std::map<int, bool>{}, absPhase));
		}
		else if (word == "trigger") {
			int ch = nextChannel();
			sequence[ch].push_back(std::make_shared<synth::Wait>(0.0, /*wait_for_trigger=*/true));
		}
		else if (word == "hold") {
			int ch = nextChannel();
			double durationMs = nextValue();
			sequence[ch].push_back(std::make_shared<synth::Wait>(durationMs * 1e-3));
		}
		else if (word == "freqramp") {
			int ch = nextChannel();
			double durationMs = nextValue();
			double amp = nextValue();
			double f0 = nextValue();
			double f1 = nextValue();
			int steps = static_cast<int>(std::lround(nextValue()));
			sequence[ch].push_back(std::make_shared<synth::FrequencyRamp>(
				durationMs * 1e-3, amp / 100.0, std::nullopt, f0 * 1e6, f1 * 1e6, steps));
		}
		else if (word == "freqfuncramp") {
			int ch = nextChannel();
			double durationMs = nextValue();
			double amp = nextValue();
			double f0 = nextValue();
			double f1 = nextValue();
			std::string shapeSpec;
			script >> shapeSpec;                     // a name, not an Expression
			double maxErr = nextValue();
			try {
				sequence[ch].push_back(std::make_shared<synth::FrequencyFunctionRamp>(
					durationMs * 1e-3, synth::makeRampShape(shapeSpec), amp / 100.0,
					std::nullopt, f0 * 1e6, f1 * 1e6, maxErr));
			}
			catch (std::exception& e) {
				thrower("ERROR: GigaMoog420 freqfuncramp: " + std::string(e.what()));
			}
		}
		else if (word == "ampramp") {
			int ch = nextChannel();
			double durationMs = nextValue();
			double a0 = nextValue();
			double a1 = nextValue();
			int steps = static_cast<int>(std::lround(nextValue()));
			sequence[ch].push_back(std::make_shared<synth::AmplitudeRamp>(
				durationMs * 1e-3, a0 / 100.0, a1 / 100.0, std::nullopt, std::nullopt, steps));
		}
		else if (word == "ampfuncramp") {
			int ch = nextChannel();
			double durationMs = nextValue();
			double a0 = nextValue();
			double a1 = nextValue();
			std::string shapeSpec;
			script >> shapeSpec;                     // a name, not an Expression
			double maxErrPct = nextValue();
			try {
				sequence[ch].push_back(std::make_shared<synth::AmplitudeFunctionRamp>(
					durationMs * 1e-3, synth::makeRampShape(shapeSpec), a0 / 100.0, a1 / 100.0,
					std::nullopt, std::nullopt, maxErrPct / 100.0));
			}
			catch (std::exception& e) {
				thrower("ERROR: GigaMoog420 ampfuncramp: " + std::string(e.what()));
			}
		}
		else if (word == "tableramp") {
			int ch = nextChannel();
			std::string fileName, name;
			script >> fileName;
			std::string path = rampsFilePath(fileName, name);
			std::shared_ptr<synth::TableRamp> table;
			try {
				table = synth::TableRamp::fromFile(path);
			}
			catch (std::exception& e) {
				thrower("ERROR: GigaMoog420 tableramp: " + std::string(e.what()));
			}
			table->label = name;                    // a preceding "label" command overrides this below
			sequence[ch].push_back(table);
		}
		else if (word == "feedforward") {
			useFeedforward = nextValue() > 0.5;
		}
		else if (word == "correction") {
			std::string label, fileName, name;
			script >> label >> fileName;
			gm420::AmplitudeCorrection corr;
			corr.load(rampsFilePath(fileName, name));
			scriptCorrections[normalizeLabel(label)] = std::move(corr);
		}
		else {
			thrower("ERROR: unrecognized GigaMoog420 command: \"" + word + "\"");
		}

		if (!pendingLabel.empty()) {
			std::shared_ptr<synth::RFBlock> added;
			for (auto& [ch, blocks] : sequence) {
				size_t before = sizesBefore.count(ch) ? sizesBefore[ch] : 0;
				if (blocks.size() == before + 1) added = blocks.back();
			}
			if (!added) {
				thrower("ERROR: GigaMoog420 label \"" + pendingLabel + "\" must be followed by a command that "
					"adds a block (set, settrig, trigger, hold or a ramp), not \"" + word + "\".");
			}
			added->label = pendingLabel;
			pendingLabel.clear();
		}
		word = "";
		script >> word;
	}
	if (!pendingLabel.empty()) {
		thrower("ERROR: GigaMoog420 label \"" + pendingLabel + "\" at the end of the script has no command.");
	}
	if (!warnings.empty()) {
		emit errBox(qstr(warnings));
	}
	return true;
}