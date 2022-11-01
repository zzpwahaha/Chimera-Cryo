#include "stdafx.h"
#include "DdsOutput.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include "boost\lexical_cast.hpp"
#include <QlineEdit>
#include <QKeyEvent>


DdsOutput::DdsOutput() {}

void DdsOutput::initialize(IChimeraQtWindow* parent, int whichDDS) 
{
	info.name = "dds" +
		str(whichDDS / size_t(DDSGrid::numPERunit)) + "_" +
		str(whichDDS % size_t(DDSGrid::numPERunit)); /*default name, always accepted by script*/
	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);

	label = new QLabel(QString("%1").arg(whichDDS % size_t(DDSGrid::numPERunit), 1), parent);
	label->setToolTip(cstr(info.name + "\r\n" + info.note));

	editFreq = new CQLineEdit(QString("%1").arg(whichDDS % size_t(DDSGrid::numPERunit)), parent);
	editFreq->setToolTip(cstr(info.name + ": Freq[" + str(info.minFreq, numFreqDigits, true) + "," + str(info.maxFreq, numFreqDigits, true) + "]" + "\r\n" + info.note));
	editFreq->installEventFilter(parent);
	parent->connect(editFreq, &QLineEdit::textChanged,
		[this, parent]() {
			handleEdit();
		});
	//edit->setStyleSheet ("QLineEdit { border: none }");

	layout->addWidget(label, 0);
	layout->addWidget(editFreq, 1);
	//layout->addStretch(1);
	editFreq->setMaximumWidth(150);
	editFreq->setMinimumWidth(90);

	editAmp = new CQLineEdit("", parent);
	editAmp->setToolTip(cstr(info.name + ": Amp[" + str(info.minAmp, numAmplDigits, true) + "," + str(info.maxAmp, numAmplDigits, true) + "]" + "\r\n" + info.note));
	editAmp->installEventFilter(parent);
	parent->connect(editAmp, &QLineEdit::textChanged,
		[this, parent]() {
			handleEdit();
		});
	//edit->setStyleSheet ("QLineEdit { border: none }");

	//layout->addWidget(new QLabel("Amp"), 0);
	layout->addWidget(editAmp, 1);
	layout->addStretch(1);
	editAmp->setMaximumWidth(150);

	updateEdit(false);
}

bool DdsOutput::eventFilter(QObject* obj, QEvent* event) {
	if (obj == editFreq) 
	{
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			bool up = true;
			if (keyEvent->key() == Qt::Key_Up) {
				up = true;
			}
			else if (keyEvent->key() == Qt::Key_Down) {
				up = false;
			}
			else {
				/*if not up or down, just update the stored value from text and the eventfileter is called in DdsSystem eventfilter*/
				handleEdit();
				return false;
			}
			auto txt = editFreq->text();
			// make sure value in edit matches current value, else unclear what this functionality should do.
			double val;
			try {
				val = boost::lexical_cast<double>(str(txt));
			}
			catch (boost::bad_lexical_cast) {
				return true;
			}
			if (fabs(val - info.currFreq) > 1e-12) {
				return true;
			}


			auto decimalPos = txt.indexOf(".");
			auto cursorPos = editFreq->cursorPosition();
			if (decimalPos == -1) {
				// value is an integer, decimal is effectively at end.
				decimalPos = txt.size();
			}
			// the order of the first digit
			double size = pow(10, decimalPos - 1);
			// handle the extra decimal character with the ternary operator here. 
			int editPlace = (cursorPos > decimalPos ? cursorPos - 1 : cursorPos);
			double inc = size / pow(10, editPlace - 1);
			info.currFreq += up ? inc : -inc;
			updateEdit(false);
			auto newTxt = editFreq->text();
			if (txt.indexOf("-") == -1 && newTxt.indexOf("-") != -1) {
				// then need to shift cursor to account for the negative.
				editFreq->setCursorPosition(cursorPos + 1);
			}
			else if (txt.indexOf("-") != std::string::npos && newTxt.indexOf("-") == std::string::npos) {
				editFreq->setCursorPosition(cursorPos - 1);
			}
			else {
				editFreq->setCursorPosition(cursorPos);
			}
			updateEdit(false);
			return true;
		}
		return false;
	}
	else if (obj == editAmp)
	{
		if (event->type() == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			bool up = true;
			if (keyEvent->key() == Qt::Key_Up) 
			{
				up = true;
			}
			else if (keyEvent->key() == Qt::Key_Down) 
			{
				up = false;
			}
			else 
			{
				handleEdit();
				return false;
			}
			auto txt = editAmp->text();
			// make sure value in edit matches current value, else unclear what this functionality should do.
			double val;
			try {
				val = boost::lexical_cast<double>(str(txt));
			}
			catch (boost::bad_lexical_cast) {
				return true;
			}
			if (fabs(val - info.currAmp) > 1e-12) {
				return true;
			}


			auto decimalPos = txt.indexOf(".");
			auto cursorPos = editAmp->cursorPosition();
			if (decimalPos == -1) {
				// value is an integer, decimal is effectively at end.
				decimalPos = txt.size();
			}
			// the order of the first digit
			double size = pow(10, decimalPos - 1);
			// handle the extra decimal character with the ternary operator here. 
			int editPlace = (cursorPos > decimalPos ? cursorPos - 1 : cursorPos);
			double inc = size / pow(10, editPlace - 1);
			info.currAmp += up ? inc : -inc;
			updateEdit(false);
			auto newTxt = editAmp->text();
			if (txt.indexOf("-") == -1 && newTxt.indexOf("-") != -1) {
				// then need to shift cursor to account for the negative.
				editAmp->setCursorPosition(cursorPos + 1);
			}
			else if (txt.indexOf("-") != std::string::npos && newTxt.indexOf("-") == std::string::npos) {
				editAmp->setCursorPosition(cursorPos - 1);
			}
			else {
				editAmp->setCursorPosition(cursorPos);
			}
			updateEdit(false);
			return true;
		}
		return false;
	}
	
	return false;
}

std::pair<double,double> DdsOutput::getValFA(bool useDefault) 
{
	double valF, valA;
	try 
	{
		valF = boost::lexical_cast<double>(str(editFreq->text(),numFreqDigits));
		valA = boost::lexical_cast<double>(str(editAmp->text(),numAmplDigits));
	}
	catch (boost::bad_lexical_cast&) 
	{
		if (useDefault) {
			valF = info.defaultFreq;
			valA = info.defaultAmp;
		}
		else {
			throwNested("value entered in DDS #" + str(ddsNum) + " (" + str(editAmp->text())
				+ ") failed to convert to a double!" + 
				"value entered in DDS #" + str(ddsNum) + " (" + str(editFreq->text())
				+ ") failed to convert to a double!");
		}
	}
	return checkBound(valF, valA);
}


void DdsOutput::updateEdit(bool roundToDacPrecision) 
{
	
	
	std::string valStrFreq = roundToDacPrecision ? str(roundToDdsResolution(info.currFreq, true), 13, true, false, true)
		: str(info.currFreq, numFreqDigits, false, false, false);
	int posf = editFreq->cursorPosition();
	editFreq->setText(cstr(valStrFreq));
	editFreq->setCursorPosition(posf);

	std::string valStrAmp = roundToDacPrecision ? str(roundToDdsResolution(info.currAmp, true), 13, true, false, true)
		: str(info.currAmp, numAmplDigits, false, false, false);
	int posa = editAmp->cursorPosition();
	editAmp->setText(cstr(valStrAmp));
	editAmp->setCursorPosition(posa);
	
}

void DdsOutput::setName(std::string name) {
	if (name == "") {
		// no empty names allowed.
		return;
	}
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	info.name = name;
	label->setToolTip((info.name + "\r\n" + info.note).c_str());
	editAmp->setToolTip(cstr(info.name + ": [" + str(info.minAmp, numAmplDigits,true) + "," + str(info.maxAmp, numAmplDigits,true) + "]" + "\r\n" + info.note));
	editFreq->setToolTip((info.name + "\r\n" + info.note).c_str());
}


void DdsOutput::handleEdit(bool roundToDacPrecision) 
{
	auto [f, a] = getValFA(true);
	info.currFreq = f;
	info.currAmp = a;


	//bool matches = false;
	//try {
	//	if (roundToDacPrecision) 
	//	{
	//		double roundNumFreq = roundToDdsResolution(info.currFreq, true);
	//		if (fabs(roundToDdsResolution(info.currFreq, true) - boost::lexical_cast<double>(str(editFreq->text()))) < 1e-8) 
	//		{
	//			matches = true; /*seems match is unused*/
	//		}
	//	}
	//	else {
	//		if (fabs(info.currFreq - boost::lexical_cast<double>(str(editFreq->text()))) < 1e-8) {
	//			matches = true;
	//		}
	//	}
	//}
	//catch (boost::bad_lexical_cast&) { /* failed to convert to double. Effectively, doesn't match. */ }
	//matches = false;
	//try {
	//	if (roundToDacPrecision)
	//	{
	//		double roundNumAmp = roundToDdsResolution(info.currAmp, false);
	//		if (fabs(roundToDdsResolution(info.currAmp, false) - boost::lexical_cast<double>(str(editAmp->text()))) < 1e-8)
	//		{
	//			matches = true; /*seems match is unused*/
	//		}
	//	}
	//	else {
	//		if (fabs(info.currAmp - boost::lexical_cast<double>(str(editAmp->text()))) < 1e-8) 
	//		{
	//			matches = true;
	//		}
	//	}
	//}
	//catch (boost::bad_lexical_cast&) { /* failed to convert to double. Effectively, doesn't match. */ }


}

double DdsOutput::roundToDdsResolution(double num, bool isFreq) 
{
	//double dacResolution = 10.0 / pow ( 2, 16 );
	return isFreq ? long((num + ddsFreqResolution / 2) / ddsFreqResolution) * ddsFreqResolution :
		long((num + ddsAmplResolution / 2) / ddsAmplResolution) * ddsAmplResolution;
}


void DdsOutput::setNote(std::string note) {
	info.note = note;
	label->setToolTip((info.name + "\r\n" + info.note).c_str());
	editAmp->setToolTip(cstr(info.name + ": [" + str(info.minAmp, numAmplDigits,true) +"," + str(info.maxAmp, numAmplDigits,true) + "]" + "\r\n" + info.note));
	editFreq->setToolTip((info.name + "\r\n" + info.note).c_str());
}

void DdsOutput::disable() {
	editAmp->setEnabled(false);
	editFreq->setEnabled(false);
}

void DdsOutput::setExpActiveColor(bool usedInExp, bool expFinished)
{
	for (auto check : { editFreq,editAmp }) {
		check->setStyleSheet(usedInExp ? 
			(expFinished ? "QLineEdit { background: rgb(255, 255, 0)}" : "QLineEdit { background: rgb(0, 255, 0)}") :
			"QLineEdit { background: rgb(255, 255, 255)}");
	}
}

std::pair<double, double> DdsOutput::checkBound(double valF, double valA)
{
	if (valF > info.maxFreq)
	{
		valF = info.maxFreq;
	}
	else if (valF < info.minFreq)
	{
		valF = info.minFreq;
	}

	if (valA > info.maxAmp)
	{
		valA = info.maxAmp;
	}
	else if (valA < info.minAmp)
	{
		valA = info.minAmp;
	}

	return std::pair<double, double>(valF,valA);
}

bool DdsOutput::isEdit(QObject* obj)
{
	return (editFreq == obj) || (editAmp == obj);
}
