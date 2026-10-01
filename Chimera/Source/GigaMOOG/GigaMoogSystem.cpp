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
	, gmoogScript420(parent)
	, core(GIGAMOOG_SAFEMODE, GIGAMOOG_IPADDRESS, GIGAMOOG_IPPORT)
	, core420(GIGAMOOG420_SAFEMODE, GIGAMOOG420_IPADDRESS, GIGAMOOG420_IPPORT)
{
	if (!GIGAMOOG_SAFEMODE) {
		//writeOff(); 
	}
}

GigaMoogSystem::~GigaMoogSystem(void) {
}

void GigaMoogSystem::initialize(IChimeraQtWindow* win)
{
	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	
	// ------------
	// GIGAMOOG
	QVBoxLayout* left = new QVBoxLayout(this);
	left->setContentsMargins(0, 0, 0, 0);
	QLabel* header = new QLabel("GIGAMOOG", win);
	expActive = new CQCheckBox("Exp. Active?", win);
	QPushButton* programNowBtn = new QPushButton("Program", win);
	QPushButton* trigMoveBtn = new QPushButton("Trig Move", win);
	QPushButton* trigLoadBtn = new QPushButton("Trig Load", win);
	//QPushButton* disconnectBtn= new QPushButton("Disconnect", win);
	//QPushButton* reconnectBtn = new QPushButton("Reconnect", win);
	QPushButton* resetMemBtn = new QPushButton("Reset", win);
	QHBoxLayout* left1 = new QHBoxLayout();
	left1->setContentsMargins(0, 0, 0, 0);
	left1->addWidget(header, 1);
	QHBoxLayout* left2 = new QHBoxLayout();
	left2->setContentsMargins(0, 0, 0, 0);
	left2->addWidget(resetMemBtn, 0);
	//left2->addWidget(disconnectBtn, 0);
	//left2->addWidget(reconnectBtn, 0);
	left2->addWidget(trigLoadBtn, 0);
	left2->addWidget(trigMoveBtn, 0);
	left2->addWidget(programNowBtn, 0);
	left2->addWidget(expActive, 0);


	left->addLayout(left1, 0);
	left->addLayout(left2, 0);
	gmoogScript.initialize(win, "GMoog", "GigaMoog Script");
	gmoogScript.setEnabled(true, false);
	left->addWidget(&gmoogScript, 1);

	// ------------
	// GIGAMOOG 420
	QVBoxLayout* right = new QVBoxLayout(this);
	right->setContentsMargins(0, 0, 0, 0);
	QLabel* header420 = new QLabel("GMOOG420", win);
	expActive420 = new CQCheckBox("Exp. Active?", win);
	QPushButton* programNow420Btn = new QPushButton("Program", win);
	QPushButton* trigLoad420Btn = new QPushButton("Trigger", win);
	//QPushButton* disconnect420Btn= new QPushButton("Disconnect", win);
	//QPushButton* reconnect420Btn = new QPushButton("Reconnect", win);
	QPushButton* resetMem420Btn = new QPushButton("Reset", win);
	QHBoxLayout* right1 = new QHBoxLayout();
	right1->setContentsMargins(0, 0, 0, 0);
	right1->addWidget(header420, 1);
	QHBoxLayout* right2 = new QHBoxLayout();
	right2->setContentsMargins(0, 0, 0, 0);
	right2->addWidget(resetMem420Btn, 0);
	//right2->addWidget(disconnect420Btn, 0);
	//right2->addWidget(reconnect420Btn, 0);
	right2->addWidget(trigLoad420Btn, 0);
	right2->addWidget(programNow420Btn, 0);
	right2->addWidget(expActive420, 0);


	right->addLayout(right1, 0);
	right->addLayout(right2, 0);
	gmoogScript420.initialize(win, "GMoog", "GMoog420 Script");
	gmoogScript420.setEnabled(true, false);
	right->addWidget(&gmoogScript420, 1);

	mainLayout->addLayout(left, 0);
	mainLayout->addLayout(right, 0);

	// ------------
	// GIGAMOOG
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
		win->reportStatus("----------------------\r\nTriggering GigaMoog Load... ");
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

	connect(resetMemBtn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nDisconnect GigaMoog... ");
		try {
			auto& doCore = win->auxWin->getTtlCore();
			auto dostatus = win->auxWin->getTtlSystem().getCurrentStatus();
			core.resetMemory(doCore, dostatus);
			win->reportStatus("Reset GigaMoog \r\n");
		}
		catch (ChimeraError& err) {
			//errBox(err.trace());
			win->reportErr(": " + err.qtrace() + "\r\n");
		}
		});

	// ------------
	// GIGAMOOG420
	connect(programNow420Btn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nSetting GigaMoog420... ");
		try {
			gmoogScript420.checkSave(win->mainWin->getProfileSettings().configLocation, win->mainWin->getRunInfo());
			std::string fileAddr = gmoogScript420.getScriptPathAndName();
			core420.programGMoogNow(fileAddr, win->auxWin->getUsableConstants(), win->auxWin->getTtlCore(), win->auxWin->getTtlSystem().getCurrentStatus());
			win->reportStatus(qstr("Programmed GigaMoog420 " + core420.getLastSentHex() + ".\r\n"));
			win->reportStatus("Finished Setting GigaMoog420.\r\n");
		}
		catch (ChimeraError& err) {
			errBox(err.trace());
			win->reportStatus(": " + err.qtrace() + "\r\n");
			win->reportErr(qstr("Error while programming GigaMoog420 " + core420.getDelim() + ": " + err.trace() + "\r\n"));
			win->mainWin->updateConfigurationSavedStatus(false);
		}});

	connect(trigLoad420Btn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nTriggering GigaMoog420... ");
		try {
			auto& doCore = win->auxWin->getTtlCore();
			auto dostatus = win->auxWin->getTtlSystem().getCurrentStatus();
			doCore.FPGAForcePulse(dostatus, std::vector<std::pair<unsigned, unsigned>>{GM420_TRIGGER_LINE[0]}, GM420_TRIGGER_TIME);
			win->reportStatus("Finished Triggering GigaMoog420 with " + qstr(GM420_TRIGGER_TIME) + "ms .\r\n");
		}
		catch (ChimeraError& err) {
			errBox(err.trace());
			win->reportStatus(": " + err.qtrace() + "\r\n");
			win->reportErr(qstr("Error while triggering GigaMoog420 " + core420.getDelim() + " Load: " + err.trace() + "\r\n"));
			win->mainWin->updateConfigurationSavedStatus(false);
		}});

	connect(resetMem420Btn, &QPushButton::released, this, [this, win]() {
		win->reportStatus("----------------------\r\nDisconnect GigaMoog420... ");
		try {
			auto& doCore = win->auxWin->getTtlCore();
			auto dostatus = win->auxWin->getTtlSystem().getCurrentStatus();
			core420.resetMemory(doCore, dostatus);
			win->reportStatus("Reset GigaMoog420 \r\n");
		}
		catch (ChimeraError& err) {
			//errBox(err.trace());
			win->reportErr(": " + err.qtrace() + "\r\n");
		}
		});

	//connect(disconnectBtn, &QPushButton::released, this, [this, win]() {
	//	win->reportStatus("----------------------\r\nDisconnect GigaMoog... ");
	//	try {
	//		core.disconnectPort();
	//		win->reportStatus("Disconnected GigaMoog \r\n");
	//	}
	//	catch (ChimeraError& err) {
	//		//errBox(err.trace());
	//		win->reportErr(": " + err.qtrace() + "\r\n");
	//	}
	//	});

	//connect(reconnectBtn, &QPushButton::released, this, [this, win]() {
	//	win->reportStatus("----------------------\r\nReconnect GigaMoog... ");
	//	try {
	//		core.reconnectPort();
	//		win->reportStatus("Reconnected GigaMoog \r\n");
	//	}
	//	catch (ChimeraError& err) {
	//		//errBox(err.trace());
	//		win->reportErr(": " + err.qtrace() + "\r\n");
	//	}
	//	});
}

void GigaMoogSystem::handleSaveConfig(ConfigStream& saveFile)
{
	saveFile << core.getDelim() << "\n";
	saveFile << "/*Experiment Active:*/ " << expActive->isChecked() << "\n";
	saveFile << "/*Scripted Arb Address:*/" << gmoogScript.getScriptPathAndName() + "\n";
	saveFile << "END_" + core.configDelim + "\n";

	saveFile << core420.getDelim() << "\n";
	saveFile << "/*Experiment Active:*/ " << expActive420->isChecked() << "\n";
	saveFile << "/*Scripted Arb Address:*/" << gmoogScript420.getScriptPathAndName() + "\n";
	saveFile << "END_" + core420.configDelim + "\n";
}

void GigaMoogSystem::handleOpenConfig(ConfigStream& openFile)
{
	scriptAddress = core.getSettingsFromConfig(openFile);
	expActive->setChecked(core.experimentActive);
}

void GigaMoogSystem::handleOpenConfig420(ConfigStream& openFile) 
{
	scriptAddress420 = core420.getSettingsFromConfig(openFile);
	expActive420->setChecked(core420.experimentActive);
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


