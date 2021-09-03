#pragma once 
#include <iostream> 
#include <GeneralObjects/IChimeraSystem.h>
#include "Scripts/Script.h" 
#include <Scripts/ScriptStream.h>
#include <thread> 
#include <chrono> 
#include <windows.h> 
#include <GigaMOOG/GigaMoogCore.h>


class IChimeraQtWindow;
class CQCheckBox;

class GigaMoogSystem : public IChimeraSystem
{

public:
	// THIS CLASS IS NOT COPYABLE.
	GigaMoogSystem& operator=(const GigaMoogSystem&) = delete;
	GigaMoogSystem(const GigaMoogSystem&) = delete;

	GigaMoogSystem(std::string portID, int baudrate, IChimeraQtWindow* parent);
	virtual ~GigaMoogSystem(void);

	void initialize(IChimeraQtWindow* win);
	// configs
	void handleSaveConfig(ConfigStream& saveFile);
	void handleOpenConfig(ConfigStream& openFile);
	std::string getDelim() { return core.configDelim; }
	GigaMoogCore& getCore() { return core; }

	//Attempt to parse moog script 
	//void loadMoogScript(std::string scriptAddress); //should use gmoogScript.openParentScript instead

	Script gmoogScript;
	CQCheckBox* expActive;
	std::string scriptAddress;

private:
	// the moog script file contents get dumped into this. 
	//std::string currentMoogScriptText;
	//ScriptStream currentMoogScript;

	GigaMoogCore core;

};

