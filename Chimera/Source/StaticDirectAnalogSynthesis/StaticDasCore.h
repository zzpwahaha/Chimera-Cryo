#pragma once
#include <GeneralObjects/IDeviceCore.h>
#include <StaticDirectAnalogSynthesis/StaticDasStructures.h>
#include <StaticDirectAnalogSynthesis/StaticDASFlume.h>
#include <ParameterSystem/ParameterSystemStructures.h>

class ConfigStream;
class ExpThreadWorker;
class DataLogger;
class StaticDasCore : public IDeviceCore
{
public:
    // THIS CLASS IS NOT COPYABLE.
    StaticDasCore(const StaticDasCore&) = delete;
    StaticDasCore& operator=(const StaticDasCore&) = delete;

    StaticDasCore(const std::array<bool, size_t(StaticDASGrid::numOFunit)> safemodes,
        const std::array<std::string, size_t(StaticDASGrid::numOFunit)> ports, 
        const std::array<unsigned, size_t(StaticDASGrid::numOFunit)> baudrates);

    virtual void loadExpSettings(ConfigStream& stream) override;
    virtual void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
    virtual void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
    virtual void programVariation(unsigned variation, std::vector<parameterType>& params,
        ExpThreadWorker* threadworker) override;
    virtual void normalFinish() override {};
    virtual void errorFinish() override {};
    virtual std::string getDelim() override { return configDelim; };

    StaticDASSettings getSettingsFromConfig(ConfigStream& file);

    void resetConnection();
    void directWrite(std::string cmd, int deviceId);
    std::string directRead(int deviceId);
    std::string getDeviceInfo();
    void setStaticDASExpSetting(StaticDASSettings tmpSetting); // used only for ProgramNow in StaticAOSystem

    const std::string configDelim = "STATIC_DAS";
    const std::array<bool, size_t(StaticDASGrid::numOFunit)> safemodes;
private:
    std::string getDASCommand(double dasfreqVal, int channel);
    void writeDASs(std::array<double, size_t(StaticDASGrid::total)> outputs);
    bool checkBound(double dasfreqVal);

public:
    static constexpr double dasResolutionInst = 1.25e-3; // 1.25 kHz
    const int numFreqDigits = static_cast<int>(abs(round(log10(dasResolutionInst) - 0.49)));
    const double minVal = 25;
    const double maxVal = 1000;

    const std::vector<std::string> dasSetupCommands = {
        "SOURCE1", /*set control controlling channel to Source 0*/
        "MODE CW", /*set to CW mode*/
        "REF 20 MHz", /*set reference frequency to 20MHz*/
        "REFS 0", /*Set reference to internal*/
        "REFDB 1", /*turn on reference doubler*/
        "REFDIV 0", /*turn off reference doubler*/
        "PFD 40 MHz", /*set PFD to 40 MHz*/
        "SDN 0", /*set Spur mitigation modes to low-noise mode*/
        "INTFRAC 2", /*set Fractional mode and switched to integer mode automatically if frequency is a multiple of the PFD frequency*/
        "CP 3", /*set charge pump current*/
        "PDN 1", /*PDN 1 turns the source on*/
        "OEN 1", /*Enables RF output buffer amplifiers while leaving the synthesizer PLL locked*/
        "PLEV 3", /*set relative power to largest*/
        "ATT 8.5", /*set output attenuation: 0 dB attenuation = 15 dBm output power*/

        "SOURCE2", /*set control controlling channel to Source 1*/
        "MODE CW", /*set to CW mode*/
        "REF 20 MHz", /*set reference frequency to 20MHz*/
        "REFS 0", /*Set reference to internal*/
        "REFDB 1", /*turn on reference doubler*/
        "REFDIV 0", /*turn off reference doubler*/
        "PFD 40 MHz", /*set PFD to 40 MHz*/
        "SDN 0", /*set Spur mitigation modes to low-noise mode*/
        "INTFRAC 2", /*set Fractional mode and switched to integer mode automatically if frequency is a multiple of the PFD frequency*/
        "CP 3", /*set charge pump current*/
        "PDN 1", /*PDN 1 turns the source on*/
        "OEN 1", /*Enables RF output buffer amplifiers while leaving the synthesizer PLL locked*/
        "PLEV 3", /*set relative power to largest*/
        "ATT 17", /*set output attenuation: 0 dB attenuation = 15 dBm output power*/

        "SAVE",
    };


private:
    std::array<StaticDASFlume, size_t(StaticDASGrid::numOFunit)> sdasFlumes;
    StaticDASSettings expSettings;    
};

