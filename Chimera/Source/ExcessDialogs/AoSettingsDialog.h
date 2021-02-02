// created by Mark O. Brown
#pragma once

#include "AnalogOutput/AoSystem.h"
#include <unordered_map>
#include <QLabel.h>
#include <QLineEdit.h>
#include <QObject.h>

struct aoInputStruct {
	AoSystem* aoSys;
};

/*
 * This control is an interface for extra Analog-Output settings. In particular, at the moment, the names of the 
 * different channels, and the min/max voltages allowed for each channel.
 */
class AoSettingsDialog : public QDialog 
{
	Q_OBJECT;
	public:
		AoSettingsDialog (AoSystem* inputPtr);
		void updateAllEdits();
	public Q_SLOTS:
		void handleOk ();
		void handleCancel ();
	private:
		std::array<QLabel*, size_t(AOGrid::total)> numberLabels;
		std::array<QLineEdit*, size_t(AOGrid::total)> nameEdits;
		std::array < QLineEdit*, size_t(AOGrid::total)> noteEdits;
		std::array<QLineEdit*, size_t(AOGrid::total)> minValEdits;
		std::array<QLineEdit*, size_t(AOGrid::total)> maxValEdits;
		QPushButton* okBtn;
		QPushButton* cancelBtn;
		AoSystem* input;
};
