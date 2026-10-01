#pragma once
// GM420Packets.h - UDP packet encoding for the 32-bit GigaMoog420 box (ZCU106 / YJ205 sequencer).
//
// The bit layout here mirrors zcu106_sweep_and_phase_sequence.py, which is verified on the hardware.
// Every packet is its own UDP datagram and must be sent individually and in the order produced.
//
//   Reset (2 bytes)    : A3 00  (sequencer only)   or   A4 00  (sequencer + outputs)
//   Trigger (2 bytes)  : A2 00  (software trigger)
//   Table write (8 B)  : A1 | bank|ch | addr_hi addr_lo | data (32-bit, big-endian)
//
//   Each sequence-table entry is four datagrams at the same address. They are identified by the different banks:
//     bank 0x00  time[31:0]                        (ticks of 1/153.6 MHz since the last trigger)
//     bank 0x10  bit 28 = D0 (RF switch), bit 16 = wait_for_trigger, bits 15:0 = time[47:32] (need 48 bits for timestamp encoding)
//     bank 0x20  32-bit FTW = round(f / 307.2 MHz * 2^32)
//     bank 0x30  bits 29:28 = phase-update mode (0 none, 1 absolute, 2 relative),
//                bits 27:16 = 12-bit phase word, bits 15:0 = 16-bit amplitude word

#include "SynthesizerSequences.h"   // adjust the path to where you put the synth library

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include "stdafx.h"
#include "LowLevel/constants.h"

namespace gm420 {

	using Packet = std::vector<unsigned char>;      // one UDP datagram
	using PacketList = std::vector<Packet>;

	constexpr double DDS_CLOCK_HZ = 307.2e6;        // FTW full scale
	constexpr double SEQ_CLOCK_HZ = 153.6e6;        // timestamp tick rate
	constexpr double TWO_PI = 2 * PI;

	// Memory banks (upper nibble of byte 1)
	constexpr unsigned char BANK_TIME_LOW = 0x00;
	constexpr unsigned char BANK_TIME_HIGH = 0x10;
	constexpr unsigned char BANK_FTW = 0x20;
	constexpr unsigned char BANK_PHASE_AMP = 0x30;

	// Flag bits in the TIME_HIGH word
	constexpr uint32_t WAIT_FOR_TRIGGER_BIT = 1u << 16;
	constexpr uint32_t RF_SWITCH_D0_BIT = 1u << 28;

	// --- Field conversions --------------------------------------------------------

	inline uint32_t freqToFTW(double hz) {
		return static_cast<uint32_t>(std::llround(hz / DDS_CLOCK_HZ * 4294967296.0) & 0xFFFFFFFFLL); // 2^32 = 4294967296.0
	}

	inline uint32_t ampToATW(double amplitude) { // amplitude gets divided by 100 while parsing moog script in GigaMoog420Core.cpp.
		return static_cast<uint32_t>(std::lround(amplitude * 65535.0)) & 0xFFFFu;
	}

	inline uint32_t phaseToPTW(double phaseRad) {
		double p = std::fmod(phaseRad, TWO_PI);
		if (p < 0) p += TWO_PI;
		return static_cast<uint32_t>(std::lround(p / TWO_PI * 4096.0)) & 0xFFFu;   // 2*pi wraps to 0
	}

	inline uint64_t timeToTicks(double seconds) {
		if (seconds < 0) thrower("GigaMoog420: negative timestamp");
		uint64_t ticks = static_cast<uint64_t>(std::llround(seconds * SEQ_CLOCK_HZ));
		if (ticks >= (1ULL << 48)) thrower("GigaMoog420: timestamp exceeds 48 bits");
		return ticks;
	}

	// --- Packets ------------------------------------------------------------------

	inline unsigned char bankByte(unsigned char bank, int channel) {
		if (channel < 0 || channel >= synth::N_CHANNELS) {
			thrower("GigaMoog420: channel " + std::to_string(channel) + " out of range 0-"
				+ std::to_string(synth::N_CHANNELS - 1));
		}
		return static_cast<unsigned char>(bank | (channel & 0x0F));
	}

	inline Packet writePacket(unsigned char bank, int channel, int address, uint32_t data) {
		if (address < 0 || address > 0xFFFF) thrower("GigaMoog420: table address out of range");
		return Packet{
			0xA1, bankByte(bank, channel),
			static_cast<unsigned char>((address >> 8) & 0xFF), static_cast<unsigned char>(address & 0xFF),
			static_cast<unsigned char>((data >> 24) & 0xFF), static_cast<unsigned char>((data >> 16) & 0xFF),
			static_cast<unsigned char>((data >> 8) & 0xFF),  static_cast<unsigned char>(data & 0xFF) };
	}

	inline Packet resetPacket(bool resetOutputs) { return Packet{ static_cast<unsigned char>(resetOutputs ? 0xA4 : 0xA3), 0x00 }; }
	inline Packet triggerPacket() { return Packet{ 0xA2, 0x00 }; }

	// The four write packets that make up one table entry. Each time step in a sweep will have a single table entry.
	inline PacketList entryPackets(int channel, int address, const synth::CompiledTimestamp& ts) {
		uint64_t ticks = timeToTicks(ts.timestamp);
		uint32_t timeLow = static_cast<uint32_t>(ticks & 0xFFFFFFFFULL);
		uint32_t timeHigh = static_cast<uint32_t>((ticks >> 32) & 0xFFFFULL);
		if (ts.wait_for_trigger) timeHigh |= WAIT_FOR_TRIGGER_BIT;

		uint32_t control = (static_cast<uint32_t>(ts.phase_update & 0x3) << 28)
			| (phaseToPTW(ts.phase) << 16)
			| ampToATW(ts.amplitude);

		return PacketList{
			writePacket(BANK_TIME_LOW,  channel, address, timeLow),
			writePacket(BANK_TIME_HIGH, channel, address, timeHigh),
			writePacket(BANK_FTW,       channel, address, freqToFTW(ts.frequency)),
			writePacket(BANK_PHASE_AMP, channel, address, control) };
	}

	inline bool sameOutput(const synth::CompiledTimestamp& a, const synth::CompiledTimestamp& b) {
		return freqToFTW(a.frequency) == freqToFTW(b.frequency)
			&& ampToATW(a.amplitude) == ampToATW(b.amplitude)
			&& phaseToPTW(a.phase) == phaseToPTW(b.phase)
			&& b.phase_update == 0 && !b.wait_for_trigger
			&& a.digital_out == b.digital_out;
	}

	// All table writes for a compiled sequence. Each channel's table starts at address 0.
	inline PacketList tablePackets(const synth::CompileResult& compiled) {
		PacketList out;
		for (const auto& [channel, compiledEntries] : compiled.channels) {
			std::vector<synth::CompiledTimestamp> entries = compiledEntries;
			// entries.back() is the stop marker; entries[n-2] may be the redundant hold row.
			if (entries.size() >= 3 && sameOutput(entries[entries.size() - 3], entries[entries.size() - 2])) {
				entries.erase(entries.end() - 2);
			}
			// Trigger start: the script's rows are sandwiched between two automatic header rows and the
			// stop row. The script itself is left unchanged.
			//   row 0      idle row: output off, no flags. Runs as soon as the sequencer starts (A200).
			//   row 1      trigger row: output still off, wait-for-trigger flag. Parks here until the TTL.
			//   row 2...   the script's rows, timed from the trigger (shifted by one tick so they come
			//              after the trigger row).
			//   last row   all-zero stop row.
			if (entries.size() >= 2) {
				synth::CompiledTimestamp idle = entries[0];
				idle.timestamp = 0.0;
				//idle.amplitude = 0.0; // make this have non-zero amplitude at start so that the tone is playing up until the sequence trigger.
				idle.phase_update = 0;
				idle.wait_for_trigger = false;
				idle.digital_out.fill(false);
				synth::CompiledTimestamp trig = idle;
				trig.wait_for_trigger = true;
				// Put the first two idle + trig enable entries first
				entries.insert(entries.begin(), { idle, trig });
			}
			for (size_t addr = 0; addr < entries.size(); ++addr) {
				auto p = entryPackets(channel, static_cast<int>(addr), entries[addr]);
				out.insert(out.end(), p.begin(), p.end());
			}
		}
		return out;
	}

	inline std::string toHex(const Packet& p) {
		std::string s;
		char buf[4];
		for (unsigned char b : p) { std::snprintf(buf, sizeof(buf), "%02x ", b); s += buf; }
		return s;
	}

}