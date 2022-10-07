// created by Mark O. Brown
#pragma once
#include <vector>
//#include "Control.h"
#include <qlabel>
#include <qprogressbar.h>
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <qwidget.h>

class ExperimentTimer : public QWidget
{
	Q_OBJECT
	public:
		void initialize( IChimeraQtWindow* parent );
		void update( unsigned __int64 currentExpNumber, unsigned __int64 repsPerVariation, unsigned __int64 numberOfVariations, 
					 unsigned picsPerRep, bool repFirst );
		void setTimerDisplay( std::string newText );
	private:
		QLabel* timeDisplay;
		QProgressBar* repetitionProgress;
		QProgressBar* variationProgress;
		long long firstTime;
		std::vector<double> recentDataPoints;
};