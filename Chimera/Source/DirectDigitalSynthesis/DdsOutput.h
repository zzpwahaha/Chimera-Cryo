#pragma once

//#include "control.h"
#include "DdsSystemStructures.h"
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <qlabel.h>
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <qlayout.h>
#include <utility>

class DdsOutput
{
public:
	DdsOutput();
	void initialize(IChimeraQtWindow* parent, int whichDDS);
	void handleEdit(bool roundToDacPrecision = false);
	void updateEdit(bool roundToDacPrecision = false);
	static double roundToDdsResolution(double num, bool isFreq);
	std::pair<double,double> getValFA(bool useDefault);
	bool eventFilter(QObject* obj, QEvent* event);
	void setNote(std::string note);
	DdsInfo info;
	void setName(std::string name);
	void disable();
	void setExpActiveColor(bool usedInExp, bool expFinished = false);
	std::pair<double, double> checkBound(double valF, double valA);
	bool isEdit(QObject* obj);
	QHBoxLayout* getLayout() const { return layout; }
private:
	unsigned ddsNum;
	CQLineEdit* editFreq;
	CQLineEdit* editAmp;
	QLabel* label;
	QHBoxLayout* layout;


public:
	constexpr static double ddsFreqResolution = 500.0 / 0xffffffff; /*32bit, 500MHz clock freq*/
	constexpr static double ddsAmplResolution = 100.0 / 0x3ff; /*10bit dac 0b1111111111, 10mA max dac current*/
	const int numFreqDigits = static_cast<int>(abs(round(log10(ddsFreqResolution) - 0.49)));
	const int numAmplDigits = static_cast<int>(abs(round(log10(ddsAmplResolution) - 0.49)));

};

