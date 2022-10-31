#include "stdafx.h"
#include "AiOutput.h"
#include "AiSystem.h"
#include <qdebug.h>

AiOutput::AiOutput()
{
}

void AiOutput::initialize(AiSystem* parent, int aiNumber)
{
	std::array<std::string, 2> chnlStr = { "A","B" };
	adcNum = aiNumber;
	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	rangeCombox = new CQComboBox(parent->parentWin);
	rangeCombox->setMaximumWidth(50);

	for (auto& whichR : AiChannelRange::allRanges) {
		rangeCombox->addItem(AiChannelRange::toStr(whichR).c_str());
	}
	layout->addWidget(rangeCombox, 0);

	nameLabel = new QLabel(qstr(chnlStr[adcNum / size_t(AIGrid::numPERunit)] + str(adcNum % size_t(AIGrid::numPERunit))));
	QFont font = nameLabel->font();
	font.setUnderline(true);
	nameLabel->setFont(font);
	layout->addWidget(nameLabel, 0);

	valueLabel= new QLabel(qstr(0.0, numDigits), parent);
	layout->addWidget(valueLabel, 0);
	layout->addStretch(1);

	parent->connect(rangeCombox, qOverload<int>(&QComboBox::activated), [parent, this](int) {
		int selection = rangeCombox->currentIndex();
		info.status.range = AiChannelRange::fromInt(selection);
		parent->getCore().setAiRange(adcNum, AiChannelRange::fromInt(selection));
		try { parent->getCore().updateChannelRange(); }
		catch (ChimeraError& e) { emit parent->error(e.qtrace()); }
		updateDisplayColor();
		});
	setName("adc_" + nameLabel->text().toStdString());
}

void AiOutput::setValueDisplay(double mean, double std)
{
	int numDigitStd;
	if (int(std) == 0) { // a decimal number
		if (std < adcResolution / 10.0) { //e.g. std = 0.0
			numDigitStd = 4;
		}
		else {
			numDigitStd = static_cast<int>(abs(round(log10(std) - 0.49)));
		}
	}
	else {
		numDigitStd = std / 10.0 < 1.0 ? 1 : 2; // can be at most 10, so do not bother higher digit
	}
	numDigitStd = numDigitStd > 4 ? 4 : numDigitStd;
	qDebug() << "AiSystem::refreshDisplays for disp" << adcNum << "mean, std = " << mean << std;
	/*align to left, give a space for sign, [sign]xx.xx...(x), 8 is the standard length of +10.0000*/
	int sz = snprintf(nullptr, 0, "% -*.*f(%-.0f)\r\n", 8 - numDigits + numDigitStd - 1, numDigitStd,
		mean, std * pow(10, int(std) == 0 ? numDigitStd : numDigitStd - 1));
	std::vector<char> buf(sz + 1);
	snprintf(buf.data(), sz, "% -*.*f(%-.0f)\r\n", 8 - numDigits + numDigitStd - 1, numDigitStd,
		mean, std * pow(10, int(std) == 0 ? numDigitStd : numDigitStd - 1));
	std::vector<char> buff(11 + 1); /*11 is the length of +10.0000(0)*/
	snprintf(buff.data(), 11, "%-11s", buf.data());
	valueLabel->setText(buff.data());

	//voltDisplays[dispInc]->setText(qstr(core.getAiVals()[dispInc].mean, numDigits)
	//	+ "(" + qstr(core.getAiVals()[dispInc].std * pow(10.0, numDigits), 0) + ")");
	rangeCombox->setCurrentIndex(AiChannelRange::toInt(info.status.range)/*int(aiSys->getCore().getAiVals()[adcNum].status.range)*/);
}

void AiOutput::setRangeCombo()
{
	rangeCombox->setCurrentIndex(AiChannelRange::toInt(info.status.range));
}

void AiOutput::updateDisplayColor()
{
	switch (info.status.range)
	{
	case AiChannelRange::which::off:
		valueLabel->setStyleSheet("QLabel { background-color : rgb(240,240,240); }");
		break;
	case AiChannelRange::which::quarter:
		valueLabel->setStyleSheet("QLabel { background-color :" + QVariant(QColor("lightgreen")).toString() + " ; }");
		break;
	case AiChannelRange::which::half:
		valueLabel->setStyleSheet("QLabel { background-color :" + QVariant(QColor("darkseagreen")).toString() + " ; }");
		break;
	case AiChannelRange::which::full:
		valueLabel->setStyleSheet("QLabel { background-color :" + QVariant(QColor("limegreen")).toString() + " ; }");
		break;
	default:
		break;
	}
}

void AiOutput::setNote(std::string note)
{
	info.note = note;
	rangeCombox->setToolTip((info.name + "\r\n" + info.note).c_str());
	nameLabel->setToolTip((info.name + "\r\n" + info.note).c_str());
	valueLabel->setToolTip((info.name + "\r\n" + info.note).c_str());
}

void AiOutput::setName(std::string name)
{
	if (name == "") {
		// no empty names allowed.
		return;
	}
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	info.name = name;
	rangeCombox->setToolTip((info.name + "\r\n" + info.note).c_str());
	nameLabel->setToolTip((info.name + "\r\n" + info.note).c_str());
	valueLabel->setToolTip((info.name + "\r\n" + info.note).c_str());
}
