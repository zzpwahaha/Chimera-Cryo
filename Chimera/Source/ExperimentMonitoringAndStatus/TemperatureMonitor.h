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

class InfluxBroker
{
public:
	InfluxBroker(std::string identifier, std::string syntax);
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

	const std::string dbAddr = "http://admin:Twizzler@6.1.1.77:8086?db=home";
	const std::string syntax;
	const std::string identifier;

private:
	void queryDataPoint();

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
	TemperatureMonitorCore(IChimeraQtWindow* parent);
	void loadExpSettings(ConfigStream& stream) override;
	void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override {};
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override {};
	void programVariation(unsigned variation, std::vector<parameterType>& params,
		ExpThreadWorker* threadworker) override {};
	void normalFinish() override;
	void errorFinish() override;
	std::string getDelim() override { return "TempMon"; };
	

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
	TemperatureMonitor(IChimeraQtWindow* parent_in);
	void initialize(IChimeraQtWindow* parent);


private:
	std::array<QLabel*, TEMPMON_NUMBER>  name;
	std::array<QLabel*, TEMPMON_NUMBER>  reading;
	TemperatureMonitorCore core;

};