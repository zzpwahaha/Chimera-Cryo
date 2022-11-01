// created by Mark O. Brown
#pragma once

#include "Control.h"
#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "GeneralImaging/imageParameters.h"
#include <qlabel.h>
#include <qlineedit.h>
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <qwidget.h>

struct cameraPositions;
class AndorWindow;
class MainWindow;
class IChimeraQtWindow;


class ImageDimsControl : public QWidget
{
	Q_OBJECT
	public:
		ImageDimsControl(std::string whichCam);
		void initialize(/*CQComboBox* binningCombo, */IChimeraQtWindow* parentWindow);
		imageParameters readImageParameters();
		void setImageParametersFromInput( imageParameters param );
		bool checkReady();
		void handleSave(ConfigStream& saveFile );
		void handleOpen(ConfigStream& openFile );
		static imageParameters getImageDimSettingsFromConfig (ConfigStream& configFile );
		const std::string camType;
		void updateEnabledStatus (bool viewRunning);
		void saveParams (ConfigStream& saveFile, imageParameters params);
		//void setBinningMode(AndorBinningMode::mode mode);
	private:
		QLabel* leftText = nullptr;
		QLabel* rightText = nullptr;
		QLabel* horBinningText = nullptr;
		QLabel* bottomLabel = nullptr;
		QLabel* topLabel = nullptr;
		QLabel* vertBinningText = nullptr;
		CQLineEdit* leftEdit = nullptr;
		CQLineEdit* rightEdit = nullptr;
		CQLineEdit* horBinningEdit = nullptr;
		CQLineEdit* bottomEdit = nullptr;
		CQLineEdit* topEdit = nullptr;
		CQLineEdit* vertBinningEdit = nullptr;

		//QLabel* binningLabel = nullptr;
		//CQComboBox* binningComboCopy = nullptr;
		// just for imageParameters, the actual control is from the AndorCameraSettingsControl class
		//AndorBinningMode::mode binningMode;

		bool isReady;
		imageParameters currentImageParameters;
};
