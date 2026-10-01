#pragma once 
#include <iostream> 
#include <GeneralObjects/IChimeraSystem.h>
#include "Scripts/Script.h" 
#include <Scripts/ScriptStream.h>
#include <thread> 
#include <chrono> 
#include <windows.h> 
#include <GigaMOOG/GigaMoogCore.h>
#include <GigaMOOG/GigaMoog420Core.h>


class IChimeraQtWindow;
class CQCheckBox;

class GigaMoogSystem : public IChimeraSystem
{

public:
	// THIS CLASS IS NOT COPYABLE.
	GigaMoogSystem& operator=(const GigaMoogSystem&) = delete;
	GigaMoogSystem(const GigaMoogSystem&) = delete;

	GigaMoogSystem(IChimeraQtWindow* parent);
	virtual ~GigaMoogSystem(void);

	void initialize(IChimeraQtWindow* win);
	// configs
	void handleSaveConfig(ConfigStream& saveFile);
	void handleOpenConfig(ConfigStream& openFile);
	void handleOpenConfig420(ConfigStream& openFile);
	std::string getDelim420() { return core420.configDelim; }
	std::string getDelim() { return core.configDelim; }
	GigaMoog420Core& getCore420() { return core420; }
	GigaMoogCore& getCore() { return core; }

	//Attempt to parse moog script 
	//void loadMoogScript(std::string scriptAddress); //should use gmoogScript.openParentScript instead

	Script gmoogScript;
	CQCheckBox* expActive;
	std::string scriptAddress;
	Script gmoogScript420;
	CQCheckBox* expActive420;
	std::string scriptAddress420;

private:
	GigaMoogCore core;
	GigaMoog420Core core420;

};

