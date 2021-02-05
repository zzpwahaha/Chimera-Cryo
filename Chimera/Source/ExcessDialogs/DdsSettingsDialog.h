#pragma once
#include <qdialog.h>

#include "DirectDigitalSynthesis/DdsSystem.h"

class DdsSettingsDialog :
    public QDialog
{
	Q_OBJECT;
public:
	DdsSettingsDialog(DdsSystem* inputPtr);
	void updateAllEdits();
public Q_SLOTS:
	void handleOk();
	void handleCancel();
private:
	std::array<QLabel*, size_t(AOGrid::total)> numberLabels;
	std::array<QLineEdit*, size_t(AOGrid::total)> nameEdits;
	std::array < QLineEdit*, size_t(AOGrid::total)> noteEdits;
	std::array<QLineEdit*, size_t(AOGrid::total)> minValEdits;
	std::array<QLineEdit*, size_t(AOGrid::total)> maxValEdits;
	QPushButton* okBtn;
	QPushButton* cancelBtn;
	DdsSystem* input;
};

