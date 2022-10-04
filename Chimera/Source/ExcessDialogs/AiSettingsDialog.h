#pragma once
#include <qdialog.h>
#include <AnalogInput/AiSystem.h>

class AiSettingsDialog :
    public QDialog
{
	Q_OBJECT;
public:
	AiSettingsDialog(AiSystem* inputPtr);
	void updateAllEdits();
public Q_SLOTS:
	void handleOk();
	void handleCancel();
//signals:
//	void updateSyntaxHighLight();
private:
	std::array<QLabel*, size_t(AIGrid::total)> numberLabels;
	std::array<QLineEdit*, size_t(AIGrid::total)> nameEdits;
	std::array<QLineEdit*, size_t(AIGrid::total)> noteEdits;
	QPushButton* okBtn;
	QPushButton* cancelBtn;
	AiSystem* input;
};

