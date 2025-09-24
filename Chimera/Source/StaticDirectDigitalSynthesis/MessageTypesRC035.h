#pragma once
#include <map>
#include <string>
#include <cmath>
#include <vector>

struct RC035MessageType 
{

    static uint32_t getFTW(double freqMHz);

    static uint32_t getATW(double ampPercent);

    static uint32_t getPTW(double phaseDeg);

    static uint32_t getRampTW(double freqMHz, double tSec);

    static std::vector<uint8_t> getWriteMsg(const std::string& device, uint16_t addr, uint32_t data);

    inline static int const DDS_CLOCK_MHZ = 2400; 

    inline static const std::map<std::string, int> wb_dev = {
        {"System", 0x01}, {"DDS1", 0x02}, {"DDS2", 0x03},
        {"DDS1_mod", 0x05}, {"DDS2_mod", 0x06}
    };

    inline static const std::map<std::string, int> System_addr = {
        {"DDS_reset",0x0000}, 
        {"SW_trigger_enable",0x0001},
        {"Sync_good",0x0002}, 
        {"DDS_mod_enable",0x0003},
        {"SW_trigger_reset",0x0004}, 
        {"SW_trigger_step",0x0005},
        {"DDS_sync_enable",0x0006}, 
        {"DDS_IOU_request",0x0007},
        {"HW_trigger_enable",0x0008}
    };

    inline static const std::map<std::string, int> DDS_addr = {
        {"CFR1",0x0000}, 
        {"CFR2",0x0001}, 
        {"CFR3",0x0002},
        {"CFR4",0x0003}, 
        {"DRLL",0x0004}, 
        {"DRUL",0x0005},
        {"RDRSS",0x0006}, 
        {"FDRSS",0x0007}, 
        {"DRRR",0x0008},
        {"LFJR",0x0009}, 
        {"UFJR",0x000A}, 
        {"P0_FTW",0x000B},
        {"P0_PA",0x000C}, 
        {"P1_FTW",0x000D}, 
        {"P1_PA",0x000E},
        {"P2_FTW",0x000F}, 
        {"P2_PA",0x0010}, 
        {"P3_FTW",0x0011},
        {"P3_PA",0x0012}, 
        {"P4_FTW",0x0013}, 
        {"P4_PA",0x0014},
        {"P5_FTW",0x0015}, 
        {"P5_PA",0x0016}, 
        {"P6_FTW",0x0017},
        {"P6_PA",0x0018}, 
        {"P7_FTW",0x0019}, 
        {"P7_PA",0x001A},
        {"USR0",0x001B}
    };

    inline static const std::map<std::string, int> DDS1_mod_addr = {
        {"Reset_FTW",0x0000}, 
        {"Reset_ATW",0x0001}, 
        {"Reset_PTW",0x0002},
        {"Timestamp",0x0100}, 
        {"TS_start_FTW",0x0200}, 
        {"TS_stop_FTW",0x0201},
        {"TS_step",0x0202}, 
        {"TS_ATW",0x0203}, 
        {"TS_PTW",0x0204},
        {"TS_out_disable",0x0205}
    };

    inline static std::map<std::string, int> DDS2_mod_addr = DDS1_mod_addr; // same layout
};