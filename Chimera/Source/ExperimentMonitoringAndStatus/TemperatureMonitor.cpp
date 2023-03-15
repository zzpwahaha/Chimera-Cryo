#include "stdafx.h"
#include "TemperatureMonitor.h"
#include <InfluxDBException.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qdatetime.h>
#include <filesystem>
#include <algorithm>
#include <DataLogging/DataLogger.h>
#include <ParameterSystem/Expression.h>
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>


TemperatureMonitor::TemperatureMonitor(IChimeraQtWindow* parent_in, bool safemode)
	: IChimeraSystem(parent_in)
	, core(parent_in, safemode)
	, name{ new QLabel(this) ,new QLabel(this), new QLabel(this), new QLabel(this), new QLabel(this) }
	, reading{ new QLabel(this) ,new QLabel(this), new QLabel(this), new QLabel(this), new QLabel(this) }
{
}

void TemperatureMonitor::initialize(IChimeraQtWindow* parent)
{
	for (auto id : range(TEMPMON_NUMBER)) {
		std::string identifier(core.dataBroker[id].identifier);
		std::replace(identifier.begin(), identifier.end(), '_', ' ');
		name[id]->setText(qstr(identifier)+": ");
		name[id]->setStyleSheet("QLabel {font: bold 16pt;}");
		reading[id]->setStyleSheet("QLabel {font: bold 16pt;}");
	}
	QVBoxLayout* layout = new QVBoxLayout(this);
	std::array<QHBoxLayout*, 3> lay;
	for (auto id : range(3/*TEMPMON_NUMBER*/)) {
		lay[id] = new QHBoxLayout();
		lay[id]->setContentsMargins(0, 0, 0, 0);
		if (id == 0) {
			lay[0]->addWidget(name[0]);
			lay[0]->addWidget(reading[0], 0);
			lay[0]->addWidget(name[1]);
			lay[0]->addWidget(reading[1], 0);
			lay[0]->addWidget(name[2]);
			lay[0]->addWidget(reading[2], 0);
			layout->addLayout(lay[0]);
		}
		else {
			lay[id]->addWidget(name[id+2]);
			lay[id]->addWidget(reading[id+2], 0);
			layout->addLayout(lay[id]);
		}

	}
	QTimer* timer = new QTimer(this);
	QObject::connect(timer, &QTimer::timeout, [this]() {
		std::pair<long long, double> timedata{0,-1.0};
		for (auto idx : range(TEMPMON_NUMBER)) {
			try {
				timedata = core.dataBroker[idx].queryDataPoint();
			}
			catch (ChimeraError& e) {
				emit warning("Temperature sensor can not read properly. Please check \n" + e.qtrace());
			}
			catch (influxdb::ConnectionError& e) {
				emit warning("Temperature sensor can not read properly. Please check \n" + qstr(e.what()));
			}
			switch (core.dataBroker[idx].dataMode)
			{
			case InfluxDataType::mode::Temperature:
				reading[idx]->setText(qstr(timedata.second, 2) + " K");
				break;
			case InfluxDataType::mode::Pressure:
				switch (core.dataBroker[idx].unitMode)
				{
				case InfluxDataUnitType::mode::mBar:
					reading[idx]->setText(qstr(timedata.second, 2, false, false, false, true) + " mBar");
					break;
				case InfluxDataUnitType::mode::Torr:
					reading[idx]->setText(qstr(timedata.second, 2, false, false, false, true) + " Torr");
					break;
				}
				break;
			default:
				break;
			}

		}});
	timer->start(60000/*1min*60*1e3*/); // every 1min to query a data that updates every 2min to avoid some rounding issue in time

}

TemperatureMonitorCore::TemperatureMonitorCore(IChimeraQtWindow* parent, bool safemode)
	: dataBroker{ 
	InfluxBroker(TEMPMON_ID[0],TEMPMON_SYNTAX[0], InfluxDataType::mode::Temperature, InfluxDataUnitType::mode::K, safemode),
	InfluxBroker(TEMPMON_ID[1],TEMPMON_SYNTAX[1], InfluxDataType::mode::Temperature, InfluxDataUnitType::mode::K, safemode),
	InfluxBroker(TEMPMON_ID[2],TEMPMON_SYNTAX[2], InfluxDataType::mode::Temperature, InfluxDataUnitType::mode::K, safemode),
	InfluxBroker(TEMPMON_ID[3],TEMPMON_SYNTAX[3], InfluxDataType::mode::Pressure, InfluxDataUnitType::mode::mBar, safemode),
	InfluxBroker(TEMPMON_ID[4],TEMPMON_SYNTAX[4], InfluxDataType::mode::Pressure, InfluxDataUnitType::mode::Torr, safemode) }//array aggregation, no copy or move involved
{
	this->setParent(parent);
	//dataBroker.reserve(TEMPMON_NUMBER);
	//for (unsigned idx = 0; idx < TEMPMON_NUMBER; idx++) {
	//	dataBroker.emplace_back(TEMPMON_ID[idx],TEMPMON_SYNTAX[idx]);
	//}
	auto shit = InfluxBroker(TEMPMON_ID[1], TEMPMON_SYNTAX[1], InfluxDataType::mode::Temperature, InfluxDataUnitType::mode::K, safemode);// this is in-place construction, no copy nor move in c++17

	QTime midnight = QTime(23, 59, 59, 999);
	//QTime midnightCreateFolder = QTime(23, 50, 0, 0); // give ten minute ahead to create folder 
	QTime now = QTime::currentTime();
	if (now.msecsTo(midnight) - 60000 < 0) {
		thrower("Hardworker at midnight!!!! The temperature data is tired and wouldn't want to log between 23:59-24:00"
			"Maybe try after 24:00, i.e a minite later?");
	}
	QTimer::singleShot(now.msecsTo(midnight)-60000/*60sec ahead*/, [this, parent]() {
		try {
			createDataFolder();
			dumpDataToFile();
		}
		catch (ChimeraError& e) {
			parent->reportErr(e.qtrace());
		}
		QTimer* timer = new QTimer(this);
		QObject::connect(timer, &QTimer::timeout, [this, parent]() {
			try {
				createDataFolder();
				dumpDataToFile();
			}
			catch (ChimeraError& e) {
				parent->reportErr(e.qtrace());
			} });
		timer->start(24*60*60*1000);
		});

	experimentActive = !safemode;
	
}

// use as notifying the broker to start collect temp data for exp
void TemperatureMonitorCore::loadExpSettings(ConfigStream& stream)
{
	for (auto& broker : dataBroker) {
		broker.experimentPrep();
		broker.experimentStart();
	}
}

void TemperatureMonitorCore::normalFinish()
{
	auto win = static_cast<IChimeraQtWindow*>(parent());
	DataLogger& logger = win->andorWin->getLogger();
	for (auto& broker : dataBroker) {
		broker.experimentEnd();
		auto timedata = broker.getDataExp();
		switch (broker.dataMode)
		{
		case InfluxDataType::mode::Temperature:
			try {
				logger.writeTemperature(timedata, broker.identifier);
			}
			catch (ChimeraError& e) {
				win->reportErr(qstr("Temperature data abandoned. Due to ") + e.qtrace());
			}
			break;
		case InfluxDataType::mode::Pressure:
			try {
				logger.writePressure(timedata, broker.identifier, broker.unitMode);
			}
			catch (ChimeraError& e) {
				win->reportErr(qstr("Pressure data abandoned. Due to ") + e.qtrace());
			}
			break;
		default:
			win->reportErr(qstr("Influx data mode is neither Temperature nor Pressure? A low level bug!"));
			break;
		}
	}
}

void TemperatureMonitorCore::errorFinish()
{
	normalFinish();
}

void TemperatureMonitorCore::createDataFolder()
{
	DataLogger::getDataLocation(DATA_SAVE_LOCATION, todayFoler, fullPath);
	struct stat info;
	fullPath += "TemperatureData"; // with "//" will cause ERROR_ALREADY_EXIT, just go without appending "//"
	int result = 1;
	int resultStat = stat(cstr(DATA_SAVE_LOCATION + fullPath), &info);
	if (resultStat != 0) {
		result = std::filesystem::create_directories((DATA_SAVE_LOCATION + fullPath).c_str());
	}
	if (!result) {
		thrower("ERROR: Failed to create save location for data at location " + DATA_SAVE_LOCATION  + fullPath +
			". Make sure you have access to it or change the save location. Error: " + str(GetLastError())
			+ "\r\n");
	}
}

void TemperatureMonitorCore::dumpDataToFile()
{
	for (auto idx : range(TEMPMON_NUMBER)) {
		char buff[128];
		auto timedata = std::move(dataBroker[idx].getData());
		std::ofstream ofs(DATA_SAVE_LOCATION + fullPath + "//" + TEMPMON_ID[idx] + ".csv", std::ofstream::out);
		switch (dataBroker[idx].dataMode)
		{
		case InfluxDataType::mode::Temperature:
			ofs << "epochTime(ms)" << ',' << "Temperature(K)" << '\n';
			for (auto idd : range(timedata.first.size())) {
				sprintf_s(buff, "%Id64,%f\n", timedata.first[idd], timedata.second[idd]);
				ofs << buff;
			}
			break;
		case InfluxDataType::mode::Pressure:
			switch (dataBroker[idx].unitMode)
			{
				case InfluxDataUnitType::mode::mBar:
					ofs << "epochTime(ms)" << ',' << "Pressure (mBar)" << '\n';
					for (auto idd : range(timedata.first.size())) {
						sprintf_s(buff, "%Id64,%e\n", timedata.first[idd], timedata.second[idd]);
						ofs << buff;
					}
				break;
				case InfluxDataUnitType::mode::Torr:
					ofs << "epochTime(ms)" << ',' << "Pressure (Torr)" << '\n';
					for (auto idd : range(timedata.first.size())) {
						sprintf_s(buff, "%Id64,%e\n", timedata.first[idd], timedata.second[idd]);
						ofs << buff;
					}
				break;
			}
			break;
		}
		ofs.close();
		dataBroker[idx].clearNonExpData();
	}

}

InfluxBroker::InfluxBroker(std::string identifier, std::string syntax, InfluxDataType::mode mode, InfluxDataUnitType::mode unit, bool safemode) :
	safemode(safemode),
	dataMode(mode),
	identifier(identifier),
	syntax(syntax), 
	unitMode(unit),
	experimentOngoing(false)
{
	influxPtr = influxdb::InfluxDBFactory::Get(dbAddr);
}

std::pair<std::vector<long long>, std::vector<double>> InfluxBroker::getData()
{
	QMutexLocker locker(&lock);
	return std::make_pair(timeStamp, data);
}

std::pair<std::vector<long long>, std::vector<double>> InfluxBroker::getDataExp()
{
	QMutexLocker locker(&lock);
	return std::make_pair(timeStampExp, dataExp);
}

void InfluxBroker::experimentPrep()
{
	QMutexLocker locker(&lock);
	timeStampExp.clear();
	dataExp.clear();
}

std::pair<long long, double> InfluxBroker::queryDataPoint()
{
	if (safemode) {
		data.push_back(-1.0);
		timeStamp.push_back(1);

		if (experimentOngoing) {
			dataExp.push_back(data.back());
			timeStampExp.push_back(timeStamp.back());
		}
		return std::make_pair(timeStamp.back(), data.back());
	}
	std::vector<influxdb::Point> points = influxPtr->query(syntax);
	std::chrono::time_point<std::chrono::system_clock> tt = points[0].getTimestamp();
	long long time = std::chrono::duration_cast<std::chrono::seconds>(tt.time_since_epoch()).count(); // somehow need *10 to be ms epoch. somehow do not need *10 again zzp 09/16/2022
	QMutexLocker locker(&lock);
	if (!timeStamp.empty() && timeStamp.back() == time) { //https://stackoverflow.com/questions/7925479/if-argument-evaluation-order
		// not a new point, skip this
		return std::make_pair(timeStamp.back(),data.back());
	}
	std::string temperature = points[0].getFields();
	temperature = temperature.substr(temperature.find("=") + 1, temperature.size());
	Expression temperatureXprs(temperature);

	data.push_back(temperatureXprs.evaluate());
	timeStamp.push_back(time);

	if (experimentOngoing) {
		dataExp.push_back(data.back());
		timeStampExp.push_back(timeStamp.back());
	}
	return std::make_pair(timeStamp.back(), data.back());
}

void InfluxBroker::clearNonExpData()
{
	QMutexLocker locker(&lock);
	timeStamp.clear();
	data.clear();
}




