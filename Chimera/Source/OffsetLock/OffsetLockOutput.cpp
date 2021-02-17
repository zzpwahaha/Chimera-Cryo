#include "stdafx.h"
#include "OffsetLockOutput.h"

#include <QlineEdit>
#include <QKeyEvent>

OffsetLockOutput::OffsetLockOutput() {}

void OffsetLockOutput::initialize(IChimeraQtWindow* parent, int whichOL)
{
	info.name = "ol" +
		str(whichOL / size_t(OLGrid::numPERunit)) + "_" +
		str(whichOL % size_t(OLGrid::numPERunit)); /*default name, always accepted by script*/
	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);

	label = new QLabel(QString("%1").arg(whichOL % size_t(OLGrid::numPERunit), 1), parent);
	label->setToolTip(cstr(info.name + "\r\n" + info.note));

	editFreq = new CQLineEdit(QString("%1").arg(whichOL % size_t(OLGrid::numPERunit)), parent);
	editFreq->setToolTip(cstr(info.name + ": Freq[" + str(info.minFreq, numFreqDigits, true) + "," + str(info.maxFreq, numFreqDigits, true) + "]" + "\r\n" + info.note));
	editFreq->installEventFilter(parent);
	parent->connect(editFreq, &QLineEdit::textChanged,
		[this, parent]() {
			handleEdit();
		});
	//edit->setStyleSheet ("QLineEdit { border: none }");

	layout->addWidget(label, 0);
	layout->addWidget(editFreq, 1);
	layout->addStretch(1);
	editFreq->setMaximumWidth(150);
	editFreq->setMinimumWidth(90);

	updateEdit(false);
}

void OffsetLockOutput::handleEdit(bool roundToDacPrecision)
{
	double f = getVal(true);
	info.currFreq = f;
}

void OffsetLockOutput::updateEdit(bool roundToDacPrecision)
{
	std::string valStrFreq = roundToDacPrecision ? str(roundToOlResolution(info.currFreq), 13, true, false, true)
		: str(info.currFreq, numFreqDigits, false, false, false);
	int posf = editFreq->cursorPosition();
	editFreq->setText(cstr(valStrFreq));
	editFreq->setCursorPosition(posf);
}


double OffsetLockOutput::getVal(bool useDefault)
{
	double valF;
	try
	{
		valF = boost::lexical_cast<double>(str(editFreq->text(), numFreqDigits));
	}
	catch (boost::bad_lexical_cast&)
	{
		if (useDefault) {
			valF = info.defaultFreq;
		}
		else {
			throwNested("value entered in DDS #" + str(olNum) + " (" + str(editFreq->text())
				+ ") failed to convert to a double!");
		}
	}
	return checkBound(valF);
}

double OffsetLockOutput::checkBound(double valF)
{
	if (valF > info.maxFreq)
	{
		valF = info.maxFreq;
	}
	else if (valF < info.minFreq)
	{
		valF = info.minFreq;
	}

	return double(valF);
}

double OffsetLockOutput::roundToOlResolution(double num)
{
	//double dacResolution = 10.0 / pow ( 2, 16 );
	return long((num + olFreqResolution / 2) / olFreqResolution) * olFreqResolution;
}




bool OffsetLockOutput::eventFilter(QObject* obj, QEvent* event) {
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

	return false;
}



void OffsetLockOutput::setName(std::string name) {
	if (name == "") {
		// no empty names allowed.
		return;
	}
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	info.name = name;
	label->setToolTip((info.name + "\r\n" + info.note).c_str());
	editFreq->setToolTip((info.name + "\r\n" + info.note).c_str());
}

void OffsetLockOutput::setNote(std::string note) {
	info.note = note;
	label->setToolTip((info.name + "\r\n" + info.note).c_str());
	editFreq->setToolTip((info.name + "\r\n" + info.note).c_str());
}

void OffsetLockOutput::disable() {
	editFreq->setEnabled(false);
}