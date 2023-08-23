// created by Mark O. Brown
#pragma once

#include "Plotting/PlottingInfo.h"
#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "atomGrid.h"
#include "Plotting/tinyPlotInfo.h"
#include "ParameterSystem/Expression.h"

#include <CustomQtControls/AutoNotifyCtrls.h>
#include <RealTimeDataAnalysis/analysisSettings.h>
#include <GeneralObjects/IChimeraSystem.h>

#include <deque>
#include <map>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qtableWidget.h>
#include <qcombobox.h>

struct realTimePlotterInput;
struct cameraPositions;

// variation data is to be organized
// variationData[datasetNumber][groupNumber][variationNumber];
typedef std::vector<std::vector<std::vector<double>>> variationData;
typedef std::vector<std::vector<double>> avgData;

class DataAnalysisControl : public IChimeraSystem {
	public:
		static constexpr auto PLOTTING_EXTENSION = "plot";

		DataAnalysisControl(IChimeraQtWindow* parent );

		void initialize( IChimeraQtWindow* parent );
		void handleOpenConfig(ConfigStream& file );
		static analysisSettings getAnalysisSettingsFromFile (ConfigStream& file);
		void setAnalysisSettings (analysisSettings settings);
		void updateSettings ();
		void handleSaveConfig(ConfigStream& file );
		static std::pair<bool, std::string> getBumpAnalysisOptions (ConfigStream& file);
		void updateDataSetNumberEdit( int number );
		
		void reloadListView();
		void reloadTinyPlotInfo();
		void handleAtomGridCombo( );
		void reloadGridCombo( );
		void fillPlotThreadInput( realTimePlotterInput* input );
		void loadGridParams( atomGrid& grid );
		void saveGridParams( );
		void handleDeleteGrid(int sel);

		void handleContextMenu (const QPoint& pos);

		std::vector<std::string> getActivePlotList ();

		// used to determine what plots to show in the listview. should be updated from the official number when the 
		// official changes. 
		void updateUnofficialPicsPerRep (unsigned ppr);
		void updateDisplays (analysisSettings settings);
		analysisSettings getConfigSettings ();
		void setRunningSettings (analysisSettings options);
		analysisSettings getRunningSettings ();
	private:
		std::vector<std::string> getGridFileNames();

	private:
		analysisSettings currentSettings, currentlyRunningSettings;

		unsigned unofficialPicsPerRep = 1;
		// real time plotting

		QLabel* header;
		QTableWidget* plotListview;
		std::vector<tinyPlotInfo> allTinyPlots;
		
		// other data analysis
		QLabel* currentDataSetNumberText;
		QLabel* currentDataSetNumberDisp;

		CQComboBox* gridSelector;
		CQLineEdit* originEditX = nullptr;
		CQLineEdit* originEditY = nullptr;
		CQLineEdit* gridSpacingX = nullptr;
		CQLineEdit* gridSpacingY = nullptr;
		CQLineEdit* gridNumberX = nullptr;
		CQLineEdit* gridNumberY = nullptr;
		CQLineEdit* includePixelX = nullptr;
		CQLineEdit* includePixelY = nullptr;


		CQPushButton* doBumpAnalysis;
		CQCheckBox* autoBumpAnalysis;
		CQLineEdit* bumpEditParam;

		CQCheckBox* autoThresholdAnalysisButton;
		CQCheckBox* displayGridBtn;
		CQPushButton* newGrid;
		CQPushButton* deleteGrid;
		CQPushButton* refreshGrid;

};

/*
struct analysisSettings {
	bool autoThresholdAnalysisOption = false;
	bool displayGridOption;
	std::vector<atomGrid> grids;
	std::string bumpParam = "";
	bool autoBumpOption = false;
	std::vector<std::string> activePlotNames;
	std::vector<unsigned> whichGrids;
};*/