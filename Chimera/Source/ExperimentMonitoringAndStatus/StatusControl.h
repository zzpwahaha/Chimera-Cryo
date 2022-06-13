// created by Mark O. Brown
#pragma once
#include "Control.h"
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit.h>
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <qplaintextedit.h>
#include <queue>
#include <ExperimentMonitoringAndStatus/StatusControlOptions.h>

class StatusControl : public QWidget
{
	Q_OBJECT

	enum {
		widgetWidthMax = 640,
		widgetHeigthMax = 100000
	};
	public:
		void initialize(IChimeraQtWindow* parent, std::string headerText, std::vector<std::string> textColors);
		void addStatusText(statusMsg newMsg);
		void clear();
		void appendTimebar();
		void redrawControl();

	private:
		void addStatusToQue(statusMsg newMsg);
		void addHtmlStatusText(std::string text, std::string colorStr);
		void addPlainText(std::string text);
		void addPlainStatusTextInner(statusMsg newMsg);
		void addHtmlStatusTextInner(statusMsg newMsg);


	private:
		QLabel* header = nullptr;
		QTextEdit* edit = nullptr;
		StatusControlOptions opts;
		QPushButton* clearBtn = nullptr;
		QPushButton* redrawBtn = nullptr;
		QPushButton* options = nullptr;

		std::vector<std::string> colors;
		std::deque<statusMsg> msgHistory;
};