#pragma once

#include "OlStructure.h"
#include "CustomQtControls/AutoNotifyCtrls.h"
#include <qlabel.h>
#include <qlayout.h>

class IChimeraQtWindow;

class OffsetLockOutput
{
public:
	OffsetLockOutput();
	void initialize(IChimeraQtWindow* parent, int whichOL);
	void handleEdit(bool roundToDacPrecision = false);
	void updateEdit(bool roundToDacPrecision = false);
	double getVal(bool useDefault);
	double checkBound(double valF);
	static double roundToOlResolution(double num);
	bool eventFilter(QObject* obj, QEvent* event);
	void setName(std::string name);
	void setNote(std::string note);
	void disable();
	void setExpActiveColor(bool usedInExp, bool expFinished = false);

	OlInfo info;
	bool isEdit(QObject* obj);
	QHBoxLayout* getLayout() const { return layout; }
private:
	unsigned olNum;
	CQLineEdit* editFreq;
	QLabel* label;
	QHBoxLayout* layout;


public:
	constexpr static double olFreqResolution = 50.0 / 0x1ffffff; /*12 bit N + 25bit FRAC, 50MHz phase-freq detector frequency*/
	const int numFreqDigits = 4/*static_cast<int>(abs(round(log10(olFreqResolution) - 0.49)))*/;

};

