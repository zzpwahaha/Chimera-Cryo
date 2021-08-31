#include "stdafx.h" 
#include "GigaMoogSystem.h" 
#include <PrimaryWindows/IChimeraQtWindow.h>

//using namespace::boost::asio; 
//using namespace::std; 
//using namespace::std::placeholders; 

//const UINT gigaMoog::freqstartoffset = 512; 
//const UINT gigaMoog::freqstopoffset = 1024; 
//const UINT gigaMoog::gainoffset = 1536; 
//const UINT gigaMoog::loadoffset = 2048; 
//const UINT gigaMoog::moveoffset = 2560; 

GigaMoogSystem::GigaMoogSystem(std::string portID, int baudrate, IChimeraQtWindow* parent)
	: IChimeraSystem(parent)
	, gmoogScript(parent)
	, core(portID, baudrate)
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
	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	layout1->addWidget(header, 1);
	layout1->addWidget(expActive, 0);
	layout->addLayout(layout1, 0);
	layout->addWidget(&gmoogScript, 1);
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


