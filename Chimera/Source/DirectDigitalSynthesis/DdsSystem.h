#pragma once
#include "DdsCore.h"

#include "ParameterSystem/ParameterSystemStructures.h"
#include "ConfigurationSystems/Version.h"
#include "GeneralFlumes/ftdiFlume.h"
#include "ParameterSystem/Expression.h"
#include "CustomQtControls/functionCombo.h"

#include "DirectDigitalSynthesis/DdsSystemStructures.h"
#include "DirectDigitalSynthesis/DdsOutput.h"

#include "GeneralObjects/ExpWrap.h"
#include "ftd2xx.h"
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <qlabel.h>
#include <qtablewidget.h>
#include <qpushbutton.h>
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <GeneralObjects/IChimeraSystem.h>

#include "ZynqTCP/ZynqTCP.h"

class DdsSystem : public IChimeraSystem{	
	public:
		// THIS CLASS IS NOT COPYABLE.
		DdsSystem& operator=(const DdsSystem&) = delete;
		DdsSystem (const DdsSystem&) = delete;

		DdsSystem(IChimeraQtWindow* parent, bool ftSafemode);
		void redrawListview ( );
		void handleSaveConfig (ConfigStream& file );
		void handleOpenConfig (ConfigStream& file );
		void handleContextMenu (const QPoint& pos);
		void initialize( IChimeraQtWindow* master, std::string title );
		void initialize(IChimeraQtWindow* master);
		void refreshCurrentRamps ();
		void programNow (std::vector<parameterType>& constants);
		std::string getSystemInfo ( );
		std::string getDelim ( );
		DdsCore& getCore ( );
		
		/*******************************************************************************/
		bool eventFilter(QObject* obj, QEvent* event);
		void handleSetDdsButtonPress(bool useDefault);
		//void forceDds(DdsSnapshot initSnap);

		void zeroDds();

		void resetDds();

		void prepareForce();
		void initializeDataObjects(unsigned cmdNum);
		void setDDSs();
		void resetDdsEvents();
		void updateEdits();
		void updateDdsValues();


	private:
		QLabel* ddsHeader;
		QTableWidget* rampListview;
		QPushButton* programNowButton;
		CQCheckBox* controlCheck;


		bool controlActive = true;
		std::vector<ddsIndvRampListInfo> currentRamps;
		
		
		QLabel* ddsTitle;
		CQPushButton* ddsSetButton;
		CQPushButton* zeroDdsButton;
		CQCheckBox* quickChange;

		std::array<DdsOutput, size_t(DDSGrid::total)> outputs;

		//std::array<Control<CStatic>, 12> ddsLabels;
		//std::array<Control<CEdit>, 12> breakoutBoardFreqEdits;
		std::array<std::array<double, 2>, size_t(DDSGrid::total)> ddsValues;
		//std::array<std::string, size_t(DDSGrid::numOFunit)> ddsNames;
		//std::array<double, size_t(DDSGrid::numOFunit)> ddsMinAmp;
		//std::array<double, size_t(DDSGrid::numOFunit)> ddsMaxAmp;
		//std::array<double, size_t(DDSGrid::numOFunit)> ddsMinFreq;
		//std::array<double, size_t(DDSGrid::numOFunit)> ddsMaxFreq;
		//std::array<std::array<double, 2>, size_t(DDSGrid::numOFunit)> defaultVals;
		//std::array <const double, 2> ddsResolution;
		std::vector<DdsCommandForm> ddsCommandFormList;
		// the first vector is for each variation.
		std::vector<std::vector<DdsCommand>> ddsCommandList;
		std::vector<std::vector<DdsSnapshot>> ddsSnapshots;
		std::vector<std::vector<DdsChannelSnapshot>> ddsChannelSnapshots;
		std::vector<std::pair<double, std::vector<DdsCommand>>> timeOrganizer;


		ZynqTCP zynq_tcp;
		bool roundToDdsPrecision;

		constexpr static double ddsFreqResolution = 500.0 / 0xffffffff; /*32bit, 500MHz clock freq*/
		constexpr static double ddsAmplResolution = 10.0 / 0x3ff; /*10bit dac 0b1111111111, 10mA max dac current*/
		const int numFreqDigits = static_cast<int>(abs(round(log10(ddsFreqResolution) - 0.49)));
		const int numAmplDigits = static_cast<int>(abs(round(log10(ddsAmplResolution) - 0.49)));


		DdsCore core;/*seems useless*/
};

