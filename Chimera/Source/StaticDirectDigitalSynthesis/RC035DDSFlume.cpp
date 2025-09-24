#include "stdafx.h"
#include "RC035DDSFlume.h"

RC035DDSFlume::RC035DDSFlume(std::string portAddress, unsigned baudrate, bool safemode)
	: boostFlume(safemode, portAddress, baudrate)
	, SAFEMODE(safemode)
	, readComplete(true)
{
	boostFlume.setReadCallback(boost::bind(&RC035DDSFlume::readCallback, this, _1));
	boostFlume.setErrorCallback(boost::bind(&RC035DDSFlume::errorCallback, this, _1));
}

std::string RC035DDSFlume::query(std::string msg)
{
	write(msg);
    return read();
}

void RC035DDSFlume::write(std::string msg)
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

void RC035DDSFlume::write(std::vector<unsigned char> msg)
{
    readRegister.clear();
    errorMsg.clear();
    readComplete = false;
    /*write data to serial port*/
    boostFlume.write(msg);
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
std::string RC035DDSFlume::read()
{
	if (SAFEMODE) {
		return std::string("static DDS is in safemode.");
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
		thrower("Nothing feeded back from static DDS, something might be wrong with it." + recv + "\r\nError message: " + errorMsg);
	}
	recv.erase(std::remove(recv.begin(), recv.end(), '\n'), recv.end());
	return recv;
}

void RC035DDSFlume::resetConnection()
{
	boostFlume.disconnect();
	Sleep(10);
	boostFlume.reconnect();
}

void RC035DDSFlume::resetDDS()
{
	write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_reset"), 1));
	std::this_thread::sleep_for(std::chrono::seconds(2));
	write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_reset"), 0));
	std::this_thread::sleep_for(std::chrono::seconds(2));
}

//void RC035DDSFlume::configureDDS(bool use_sw_trig)
//{
//    // --- Disable all modulation / triggers / sync ---
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_mod_enable"), 0x00000000));
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_enable"), 0x00000000));
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_reset"), 0x00000000));
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_step"), 0x00000000));
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_sync_enable"), 0x00000000));
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("HW_trigger_enable"), 0x00000000));
//
//    // --- Run initialization routine (sync_enable = true, lock_pll = true, profile_mode = false) ---
//    initializeDDS(false, false, false);
//
//    // --- OSK enabled (needed for amplitude updates) ---
//    write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR1"), 0x00010108));
//    write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR1"), 0x00010108));
//
//    std::this_thread::sleep_for(std::chrono::milliseconds(100));
//
//    // --- Reset configuration (FTW, ATW, PTW) ---
//    setResetDDS1(366.0, 100, 0);
//    setResetDDS2(366.0, 100, 0);
//
//    // --- Setup timestamps ---
//    //for (uint32_t i = 0; i < 256; i++) {
//    //    setTimestamp(
//    //        i,                // Timestamp number 0–255
//    //        400.0, 300.0, 1, 100, 0, false,  // DDS1 params
//    //        400.0, 300.0, 1, 100, 180, false   // DDS2 params
//    //    );
//    //}
//
//    std::this_thread::sleep_for(std::chrono::milliseconds(10));
//
//    // --- Clear phase accumulator and prep modulation mode ---
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_mod_enable"), 0x00000000));
//
//    // Reset phase accumulator with OSK enabled
//    write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR1"), 0x00010908));
//    write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR1"), 0x00010908));
//
//    // Issue IOU (single synchronous update)
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_IOU_request"), 0x00000001));
//
//    // Reset phase accumulator (OSK enabled again)
//    write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR1"), 0x00010108));
//    write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR1"), 0x00010108));
//
//    // --- Turn modulation back on ---
//    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_mod_enable"), 0x00000001));
//
//    // --- Arm trigger inputs ---
//    if (use_sw_trig) {
//        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_enable"), 0x00000001));
//        std::this_thread::sleep_for(std::chrono::milliseconds(10));
//        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_reset"), 0x00000001));
//        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_reset"), 0x00000000));
//    }
//    else {
//        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("HW_trigger_enable"), 0x00000001));
//    }
//}

void RC035DDSFlume::initializeDDS(bool sync_enable, bool lock_pll, bool profile_mode)
{
    using namespace std::chrono_literals;
    // Reset communication interface state machine (optional)
    write(std::vector<uint8_t>(10, 0));
    std::this_thread::sleep_for(100ms);

    //std::cout << "Resetting DDS" << std::endl;
    // Reset DDS (pulse reset line)
    resetDDS();

    // Set matched latency (CFR2)
    if (profile_mode) {
        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR2"), 0x00808900));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR2"), 0x00808900));
    }
    else {
        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR2"), 0x00008900));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR2"), 0x00008900));
    }
    std::this_thread::sleep_for(100ms);

    if (lock_pll) {
        // Multiply input by 48 (50 MHz → 2400 MHz system clock)
        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR3"), 0x0004181C));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR3"), 0x0004181C));
        std::this_thread::sleep_for(100ms);

        // Run VCO calibration
        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR1"), 0x01010008));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR1"), 0x01010008));
        std::this_thread::sleep_for(100ms);

        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR1"), 0x00010008));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR1"), 0x00010008));
    }
    std::this_thread::sleep_for(100ms);

    // Run DAC calibration
    write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR4"), 0x01052120));
    write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR4"), 0x01052120));
    std::this_thread::sleep_for(100ms);

    write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR4"), 0x00052120));
    write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR4"), 0x00052120));
    std::this_thread::sleep_for(100ms);

    if (sync_enable) {
        // Enable sync signal
        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_sync_enable"), 1));
        std::this_thread::sleep_for(100ms);

        // Set delay for SYNC_IN and enable calibration with sync
        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("USR0"), 0x00000841));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("USR0"), 0x00000841));
        std::this_thread::sleep_for(100ms);

        // Run DAC calibration again
        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR4"), 0x01052120));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR4"), 0x01052120));
        std::this_thread::sleep_for(100ms);

        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR4"), 0x00052120));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR4"), 0x00052120));
        std::this_thread::sleep_for(100ms);

        // Disable sync signal again
        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_sync_enable"), 0));
    }
    else {
        // Sync not used -> disable explicitly
        std::this_thread::sleep_for(100ms);
        write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("USR0"), 0x00000800));
        write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("USR0"), 0x00000800));
    }
}

void RC035DDSFlume::configureDDS()
{
    // --- Disable all modulation / triggers / sync ---
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_mod_enable"), 0x00000000));
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_enable"), 0x00000000));
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_reset"), 0x00000000));
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_step"), 0x00000000));
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_sync_enable"), 0x00000000));
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("HW_trigger_enable"), 0x00000000));

    initializeDDS(true, false, false);

    // --- OSK enabled (needed for amplitude updates) ---
    write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR1"), 0x00010108));
    write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR1"), 0x00010108));

    // --- Reset configuration (FTW, ATW, PTW), default to 750 MHz ---
    setResetDDS1(750.00, 100, 0);
    setResetDDS2(750.00, 100, 0);
        
    // --- Clear phase accumulator and prep modulation mode ---
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_mod_enable"), 0x00000000));
    
    // Reset phase accumulator with OSK disenabled
    write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR1"), 0x00010908));
    write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR1"), 0x00010908));
    
    // Issue IOU (single synchronous update)
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_IOU_request"), 0x00000001));
    
    // Reset phase accumulator (OSK enabled again)
    write(RC035MessageType::getWriteMsg("DDS1", RC035MessageType::DDS_addr.at("CFR1"), 0x00010108));
    write(RC035MessageType::getWriteMsg("DDS2", RC035MessageType::DDS_addr.at("CFR1"), 0x00010108));
    
    // --- Turn modulation back on ---
    // should be ready to go
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_mod_enable"), 0x00000001));
    
    // --- Arm trigger inputs ---
    bool use_sw_trig = true;
    if (use_sw_trig) {
        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_enable"), 0x00000001));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_reset"), 0x00000001));
        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("SW_trigger_reset"), 0x00000000));
    }
    else {
        write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("HW_trigger_enable"), 0x00000001));
    }

}

void RC035DDSFlume::setDDSFreq(double freqMHz, int channel)
{
    switch (channel)
    {
    case 0:
        setResetDDS1(freqMHz, 100.0, 0.0);
        break;
    case 1:
        setResetDDS2(freqMHz, 100.0, 0.0);
        break;
    default:
        thrower("Channel number "+ str(channel) + " for RC035 DDS going out of range. This is a low level bug.");
    }
}

void RC035DDSFlume::setResetDDS1(double freqMHz, double ampPercent, double phaseDeg)
{
    setModulationOff();
    write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("Reset_FTW"), RC035MessageType::getFTW(freqMHz)));
    write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("Reset_ATW"), RC035MessageType::getATW(ampPercent)));
    write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("Reset_PTW"), RC035MessageType::getPTW(phaseDeg)));
    setModulationOn();
}

void RC035DDSFlume::setResetDDS2(double freqMHz, double ampPercent, double phaseDeg)
{
    setModulationOff();
    write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("Reset_FTW"), RC035MessageType::getFTW(freqMHz)));
    write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("Reset_ATW"), RC035MessageType::getATW(ampPercent)));
    write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("Reset_PTW"), RC035MessageType::getPTW(phaseDeg)));
    setModulationOn();
}

void RC035DDSFlume::setModulationOn()
{
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_mod_enable"), 0x00000001));

}

void RC035DDSFlume::setModulationOff()
{
    write(RC035MessageType::getWriteMsg("System", RC035MessageType::System_addr.at("DDS_mod_enable"), 0x00000000));
}

void RC035DDSFlume::setTimestamp(uint32_t timestamp, 
    double DDS1_startfreq, double DDS1_stopfreq, double DDS1_ramptime, double DDS1_amp, double DDS1_phase, bool DDS1_output_disable, 
    double DDS2_startfreq, double DDS2_stopfreq, double DDS2_ramptime, double DDS2_amp, double DDS2_phase, bool DDS2_output_disable)
{
    // ---------------- DDS1 ----------------
    //std::cout << "timestep " << timestamp << " to DDS1" << std::endl;
    write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("Timestamp"), timestamp));

    if (DDS1_output_disable) {
        write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("TS_out_disable"), 0x00000001));
    }
    else {
        write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("TS_start_FTW"), RC035MessageType::getFTW(DDS1_startfreq)));
        write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("TS_stop_FTW"), RC035MessageType::getFTW(DDS1_stopfreq)));
        write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("TS_step"), RC035MessageType::getRampTW(std::abs(DDS1_stopfreq - DDS1_startfreq), DDS1_ramptime)));
        write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("TS_ATW"), RC035MessageType::getATW(DDS1_amp)));
        write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("TS_PTW"), RC035MessageType::getPTW(DDS1_phase)));
        write(RC035MessageType::getWriteMsg("DDS1_mod", RC035MessageType::DDS1_mod_addr.at("TS_out_disable"), 0x00000000));
    }

    // ---------------- DDS2 ----------------
    //std::cout << "timestep " << timestamp << " to DDS2" << std::endl;
    write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("Timestamp"), timestamp));

    if (DDS2_output_disable) {
        write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("TS_out_disable"), 0x00000001));
    }
    else {
        write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("TS_start_FTW"), RC035MessageType::getFTW(DDS2_startfreq)));
        write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("TS_stop_FTW"), RC035MessageType::getFTW(DDS2_stopfreq)));
        write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("TS_step"), RC035MessageType::getRampTW(std::abs(DDS2_stopfreq - DDS2_startfreq), DDS2_ramptime)));
        write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("TS_ATW"), RC035MessageType::getATW(DDS2_amp)));
        write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("TS_PTW"), RC035MessageType::getPTW(DDS2_phase)));
        write(RC035MessageType::getWriteMsg("DDS2_mod", RC035MessageType::DDS2_mod_addr.at("TS_out_disable"), 0x00000000));
    }
}

void RC035DDSFlume::readCallback(int byte)
{
	if (byte < 0 || byte >255) {
		thrower("Byte value readed needs to be in range 0-255.");
	}
	readRegister.push_back(byte);
	if (byte == '\n') {
		readComplete = true;
	}
}

void RC035DDSFlume::errorCallback(std::string error)
{
	errorMsg = error;
}
