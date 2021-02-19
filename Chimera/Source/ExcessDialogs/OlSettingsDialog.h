#pragma once
#include <qdialog.h>
#include "OffsetLock/OlSystem.h"

class OlSettingsDialog :
    public QDialog
{
	Q_OBJECT;
public:
	OlSettingsDialog(OlSystem* inputPtr);
	void updateAllEdits();
public Q_SLOTS:
	void handleOk();
	void handleCancel();
signals:
	void updateSyntaxHighLight();
private:
	std::array<QLabel*, size_t(OLGrid::total)> numberLabels;
	std::array<QLineEdit*, size_t(OLGrid::total)> nameEdits;
	std::array<QLineEdit*, size_t(OLGrid::total)> noteEdits;
	//std::array<QLineEdit*, size_t(OLGrid::total)> minValEdits;
	//std::array<QLineEdit*, size_t(OLGrid::total)> maxValEdits;
	QPushButton* okBtn;
	QPushButton* cancelBtn;
	OlSystem* input;
};

