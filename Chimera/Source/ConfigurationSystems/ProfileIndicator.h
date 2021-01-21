// created by Mark O. Brown
#pragma once

#include "Control.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <qlabel.h>
#include <QtWidgets>

class ProfileIndicator : public QWidget {
	Q_OBJECT
	
	enum {
		widgetWidthMax = 1200,
		widgetHeigthMax = 100000
	};

	public:
		void initialize(QWidget* parent);
		void update(std::string text);
	private:
		QLabel* header;
		QLabel* indicator;
};

