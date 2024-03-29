#include "stdafx.h" 
#include "GigaMoogSystem.h" 
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>

//using namespace::boost::asio; 
//using namespace::std; 
//using namespace::std::placeholders; 

//const UINT gigaMoog::freqstartoffset = 512; 
//const UINT gigaMoog::freqstopoffset = 1024; 
//const UINT gigaMoog::gainoffset = 1536; 
//const UINT gigaMoog::loadoffset = 2048; 
//const UINT gigaMoog::moveoffset = 2560; 

GigaMoogSystem::GigaMoogSystem(IChimeraQtWindow* parent)
	: IChimeraSystem(parent)
	, gmoogScript(parent)
	, core(GIGAMOOG_SAFEMODE, GIGAMOOG_IPADDRESS, GIGAMOOG_IPPORT)
{
	if (!GIGAMOOG_SAFEMODE) {
		//writeOff(); 
	}
}

GigaMoogSystem::~GigaMoogSystem(void) {
}

void GigaMoogSystem::initialize(IChimeraQtWindow* win)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	QLabel* header = new QLabel("GIGAMOOG", win);
	expActive = new CQCheckBox("Exp. Active?", win);
	QPushButton* programNowBtn = new QPushButton("Program", win);
	QPushButton* trigMoveBtn = new QPushButton("Trig Move", win);
	QPushButton* trigLoadBtn = new QPushButton("Trig Load", win);
	QPushButton* disconnectBtn= new QPushButton("Disconnect", win);
	QPushButton* reconnectBtn = new QPushButton("Reconnect", win);
	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	layout1->addWidget(header, 1);
	QHBoxLayout* layout2 = new QHBoxLayout();
	layout2->setContentsMargins(0, 0, 0, 0);
	layout2->addWidget(disconnectBtn, 0);
	layout2->addWidget(reconnectBtn, 0);
	layout2->addWidget(trigLoadBtn, 0);
	layout2->addWidget(trigMoveBtn, 0);
	layout2->addWidget(programNowBtn, 0);
	layout2->addWidget(expActive, 0);


	layout->addLayout(layout1, 0);
	layout->addLayout(layout2, 0);
	gmoogScript.initialize(win, "GMoog", "GigaMoog Script");
	gmoogScript.setEnabled(true, false);
	layout->addWidget(&gmoogScript, 1);

	connect(programNowBtn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nSetting GigaMoog... ");
		try {
			gmoogScript.checkSave(win->mainWin->getProfileSettings().configLocation, win->mainWin->getRunInfo());
			std::string fileAddr = gmoogScript.getScriptPathAndName();
			core.programGMoogNow(fileAddr, win->auxWin->getUsableConstants(),win->auxWin->getTtlCore(), win->auxWin->getTtlSystem().getCurrentStatus());
			win->reportStatus(qstr("Programmed GigaMoog " + core.getDelim() + ".\r\n"));
			win->reportStatus("Finished Setting GigaMoog.\r\n");
		}
		catch (ChimeraError& err) {
			errBox(err.trace());
			win->reportStatus(": " + err.qtrace() + "\r\n");
			win->reportErr(qstr("Error while programming GigaMoog " + core.getDelim() + ": " + err.trace() + "\r\n"));
		win->mainWin->updateConfigurationSavedStatus(false);
		}});

	connect(trigLoadBtn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nTriggering GigaMoog Move... ");
		try {
			auto& doCore = win->auxWin->getTtlCore();
			auto dostatus = win->auxWin->getTtlSystem().getCurrentStatus();
			doCore.FPGAForcePulse(dostatus, std::vector<std::pair<unsigned, unsigned>>{GM_TRIGGER_LINE[0]}, GM_TRIGGER_TIME);
			win->reportStatus("Finished Triggering GigaMoog Load with " + qstr(GM_TRIGGER_TIME) + "ms .\r\n");
		}
		catch (ChimeraError& err) {
			errBox(err.trace());
			win->reportStatus(": " + err.qtrace() + "\r\n");
			win->reportErr(qstr("Error while triggering GigaMoog " + core.getDelim() + " Load: " + err.trace() + "\r\n"));
			win->mainWin->updateConfigurationSavedStatus(false);
		}});

	connect(trigMoveBtn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nTriggering GigaMoog Move... ");
		try {
			auto& doCore = win->auxWin->getTtlCore();
			auto dostatus = win->auxWin->getTtlSystem().getCurrentStatus();
			doCore.FPGAForcePulse(dostatus, std::vector<std::pair<unsigned, unsigned>>{GM_TRIGGER_LINE[1]}, GM_TRIGGER_TIME);
			win->reportStatus("Finished Triggering GigaMoog Move with " + qstr(GM_TRIGGER_TIME) + "ms .\r\n");
		}
		catch (ChimeraError& err) {
			errBox(err.trace());
			win->reportStatus(": " + err.qtrace() + "\r\n");
			win->reportErr(qstr("Error while triggering GigaMoog " + core.getDelim() + " Move: " + err.trace() + "\r\n"));
			win->mainWin->updateConfigurationSavedStatus(false);
		}});


	connect(disconnectBtn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nDisconnect GigaMoog... ");
		try {
			core.disconnectPort();
			win->reportStatus("Disconnected GigaMoog \r\n");
		}
		catch (ChimeraError& err) {
			//errBox(err.trace());
			win->reportErr(": " + err.qtrace() + "\r\n");
		}
		});

	connect(reconnectBtn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nReconnect GigaMoog... ");
		try {
			core.reconnectPort();
			win->reportStatus("Reconnected GigaMoog \r\n");
		}
		catch (ChimeraError& err) {
			//errBox(err.trace());
			win->reportErr(": " + err.qtrace() + "\r\n");
		}
		});
}

void GigaMoogSystem::handleSaveConfig(ConfigStream& saveFile)
{
	saveFile << core.getDelim() << "\n";
	saveFile << "/*Experiment Active:*/ " << expActive->isChecked() << "\n";
	saveFile << "/*Scripted Arb Address:*/" << gmoogScript.getScriptPathAndName() + "\n";
	saveFile << "END_" + core.configDelim + "\n";
}

void GigaMoogSystem::handleOpenConfig(ConfigStream& openFile)
{
	scriptAddress = core.getSettingsFromConfig(openFile);
	expActive->setChecked(core.experimentActive);
}

//void GigaMoogSystem::loadMoogScript(std::string scriptAddress)
//{
//	std::ifstream scriptFile;
//	// check if file address is good. 
//	FILE *file;
//	fopen_s(&file, cstr(scriptAddress), "r");
//	if (!file)
//	{
//		thrower("ERROR: Moog Script File " + scriptAddress + " does not exist!");
//	}
//	else
//	{
//		fclose(file);
//	}
//	scriptFile.open(cstr(scriptAddress));
//	// check opened correctly 
//	if (!scriptFile.is_open())
//	{
//		thrower("ERROR: Moog script file passed test making sure the file exists, but it still failed to open!");
//	}
//	// dump the file into the stringstream. 
//	std::stringstream buf(std::ios_base::app | std::ios_base::out | std::ios_base::in);
//	buf << scriptFile.rdbuf();
//	// This is used to more easily deal some of the analysis of the script. 
//	buf << "\r\n\r\n__END__";
//	// for whatever reason, after loading rdbuf into a stringstream, the stream seems to not  
//	// want to >> into a string. tried resetting too using seekg, but whatever, this works. 
//	currentMoogScript.str("");
//	currentMoogScript.str(buf.str());
//	currentMoogScript.clear();
//	currentMoogScript.seekg(0);
//	//std::string str(currentMoogScript.str()); 
//	scriptFile.close();
//}


