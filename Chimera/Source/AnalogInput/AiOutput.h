#pragma once
#include "AiSettings.h"
#include <qlabel.h>
#include "CustomQtControls/AutoNotifyCtrls.h"
#include <qlayout.h>

class AiSystem;
class AiOutput
{
public:
	AiOutput();
	void initialize(AiSystem* parent, int aiNumber);
	//bool eventFilter(QObject* obj, QEvent* event);
	void setValueDisplay(double mean, double std);
	void setRangeCombo();
	void updateDisplayColor();
	void setNote(std::string note);
	void setName(std::string name);
	AiValue info; // this parameter is just a replicate of the one in the core, take the core's to be the final one
	QHBoxLayout* getLayout() const { return layout; }
private:
	unsigned adcNum;
	CQComboBox* rangeCombox;
	QLabel* nameLabel;
	QLabel* valueLabel;
	QHBoxLayout* layout;

public:
	static constexpr double adcResolution = 20.0 / 0xffff; /*16bit adc*/
	const int numDigits = static_cast<int>(abs(round(log10(adcResolution) - 0.49)));
};

