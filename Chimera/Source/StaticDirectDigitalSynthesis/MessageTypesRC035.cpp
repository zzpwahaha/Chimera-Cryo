#include "stdafx.h"
#include "MessageTypesRC035.h"

uint32_t RC035MessageType::getFTW(double freqMHz) 
{
    return static_cast<uint32_t>((freqMHz * (1ULL << 32)) / DDS_CLOCK_MHZ);
}

uint32_t RC035MessageType::getATW(double ampPercent)
{
    return static_cast<uint32_t>(ampPercent * ((1 << 12) - 1) / 100.0);
}

uint32_t RC035MessageType::getPTW(double phaseDeg)
{
    double wrappedPhaseDeg = std::fmod(phaseDeg, 360.0);
    if (wrappedPhaseDeg < 0.0) {
        wrappedPhaseDeg += 360.0;
    }
    return static_cast<uint32_t>(phaseDeg * (1 << 16) / 360.0);
}

uint32_t RC035MessageType::getRampTW(double freqMHz, double tSec)
{
    return static_cast<uint32_t>(std::llround(freqMHz * (1ULL << 32) / (3.0 * std::pow(5.0, 10)) / tSec));
}

std::vector<uint8_t> RC035MessageType::getWriteMsg(const std::string& device, uint16_t addr, uint32_t data)
{
    if (wb_dev.find(device) == wb_dev.end()) {
        thrower("invalid wb device in RC035MessageType::getWriteMsg. This is a low level bug.");
    }
    int devVal = wb_dev.at(device);

    uint8_t addr_hi = addr / 256;
    uint8_t addr_lo = addr % 256;

    uint8_t data3 = (data >> 24) & 0xFF;
    uint8_t data2 = (data >> 16) & 0xFF;
    uint8_t data1 = (data >> 8) & 0xFF;
    uint8_t data0 = (data >> 0) & 0xFF;

    std::vector<uint8_t> msg = { static_cast<uint8_t>(160 + devVal), addr_hi, addr_lo,
                                 data3, data2, data1, data0 };

    //std::cout << "genmsg: " << int(160 + devVal) << " "
    //    << int(addr_hi) << " " << int(addr_lo) << " "
    //    << int(data3) << " " << int(data2) << " "
    //    << int(data1) << " " << int(data0) << "\n";

    return msg;
}
