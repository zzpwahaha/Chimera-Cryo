// created by Mark O. Brown
#pragma once

#include "Control.h"
#include "GeneralObjects/commonTypes.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <qlabel.h>

class StatusIndicator
{
	public:
		void initialize(IChimeraQtWindow* parent );
		void setText(std::string text);
		void setColor(std::string color);
		QLabel* statusLabel() const { return status; }
	private:
		QLabel* status;
		std::string currentColor;
};
