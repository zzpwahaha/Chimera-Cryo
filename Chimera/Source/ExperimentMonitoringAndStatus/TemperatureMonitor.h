#pragma once
#include "GeneralObjects/commonTypes.h"
#include <GeneralObjects/IDeviceCore.h>
#include <GeneralObjects/IChimeraSystem.h>
#include <qlabel.h>
#include <qmutex.h>
#include <InfluxDB.h>
#include <InfluxDBFactory.h>
#include <atomic>

class IChimeraQtWindow;
class InfluxBroker;
class TemperatureMonitorCore;

struct InfluxDataType
{
	enum class mode {
		Temperature,
		Pressure
	};
	static const std::array<mode, 2> allModes;
};

class InfluxBroker
{
public:
	InfluxBroker(std::string identifier, std::string syntax, InfluxDataType::mode mode, bool safemode);
	InfluxBroker(const InfluxBroker&) = delete;
	InfluxBroker& operator=(const InfluxBroker&) = delete;
	//InfluxBroker(InfluxBroker&&) noexcept = default;
	//InfluxBroker& operator=(InfluxBroker&&) noexcept = default;
	//~InfluxBroker() noexcept {}

	std::pair<std::vector<long long>, std::vector<double>> getData();
	std::pair<std::vector<long long>, std::vector<double>> getDataExp();
	void experimentPrep();
	void experimentStart() { experimentOngoing=true; };
	void experimentEnd() { experimentOngoing = false; };;
	std::pair<long long,double> queryDataPoint();
	void clearNonExpData();


	const std::string dbAddr = "http://admin:Twizzler@6.1.1.93:8086?db=monitoring";
	const std::string syntax;
	const std::string identifier;
	const InfluxDataType::mode dataMode;

private:
	const bool safemode;
	std::unique_ptr<influxdb::InfluxDB> influxPtr;
	std::atomic<bool> experimentOngoing; 
	QMutex lock; // not movable!

	std::vector<long long> timeStamp;
	std::vector<double> data;
	// for storeing data during experiment, avoiding overnight dump and cleannig of std::vector<long long>timeStamp and std::vector<double>data
	std::vector<long long> timeStampExp;
	std::vector<double> dataExp;

};

class TemperatureMonitorCore : public IDeviceCore
{
	Q_OBJECT
public:
	TemperatureMonitorCore(IChimeraQtWindow* parent, bool safemode);
	void loadExpSettings(ConfigStream& stream) override;
	void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override {};
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override {};
	void programVariation(unsigned variation, std::vector<parameterType>& params,
		ExpThreadWorker* threadworker) override {};
	void normalFinish() override;
	void errorFinish() override;
	std::string getDelim() override { return "TEMPMON"; };
	

	void createDataFolder();
	void dumpDataToFile();
	
	std::array<InfluxBroker, TEMPMON_NUMBER> dataBroker;

private:
	//std::vector<InfluxBroker> dataBroker;
	std::string todayFoler;
	std::string fullPath;

};

class TemperatureMonitor : public IChimeraSystem
{
public:
	// THIS CLASS IS NOT COPYABLE.
	TemperatureMonitor& operator=(const TemperatureMonitor&) = delete;
	TemperatureMonitor(const TemperatureMonitor&) = delete;
	TemperatureMonitor(IChimeraQtWindow* parent_in, bool safemode);
	void initialize(IChimeraQtWindow* parent);
	TemperatureMonitorCore& getCore() { return core; };

private:
	std::array<QLabel*, TEMPMON_NUMBER>  name;
	std::array<QLabel*, TEMPMON_NUMBER>  reading;
	TemperatureMonitorCore core;

};