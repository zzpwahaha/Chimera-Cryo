// created by Mark O. Brown
#include "stdafx.h"
#include "AoSettingsDialog.h"
#include <boost/lexical_cast.hpp>
#include <qlayout.h>

AoSettingsDialog::AoSettingsDialog (AoSystem* inputPtr) 
{
	input = inputPtr;
	//this->setStyleSheet("border: 2px solid  black;");
	this->setModal(false);
	QVBoxLayout* layoutWiget = new QVBoxLayout(this);
	QHBoxLayout* layout = new QHBoxLayout();
	auto daclayout = std::array<QGridLayout*,2>({ new QGridLayout(),new QGridLayout() });
	short cnts = 0;
	for (auto* lay : daclayout)
	{
		lay->setContentsMargins(0, 0, 0, 0);
		lay->addWidget(new QLabel(QString("DAC %1").arg(cnts)), 0, 0, 1, 7, Qt::AlignHCenter);
		{
			lay->addWidget(new QLabel("#"), 1, 0, 1, 1, Qt::AlignRight);
			lay->addWidget(new QLabel("DAC Name"), 1, 1, 1, 1);
			lay->addWidget(new QLabel("Min value"), 1, 2, 1, 1);
			lay->addWidget(new QLabel("Max value"), 1, 3, 1, 1);
			lay->addWidget(new QLabel("Notes"), 1, 4, 1, 2);
		}
		auto layofst = 2;
		auto ofst = cnts * size_t(AOGrid::numPERunit);
		for (size_t i = 0; i < size_t(AOGrid::numPERunit); i++)
		{
			numberLabels[ofst + i] = new QLabel(QString("%1").arg(i, 2));
			lay->addWidget(numberLabels[ofst + i], i + layofst, 0, 1, 1, Qt::AlignRight);

			nameEdits[ofst + i] = new QLineEdit("");
			//nameEdits[ofst + i]->setText(input->aoSys->getName(ofst + i).c_str());
			lay->addWidget(nameEdits[ofst + i], i + layofst, 1, 1, 1);

			//auto [min, max] = input->aoSys->getDacRange(ofst + i);
			minValEdits[ofst + i] = new QLineEdit("");
			//minValEdits[ofst + i]->setText(QString::number(min));
			maxValEdits[ofst + i] = new QLineEdit("");
			//maxValEdits[ofst + i]->setText(QString::number(max));
			lay->addWidget(minValEdits[ofst + i], i + layofst, 2, 1, 1);
			lay->addWidget(maxValEdits[ofst + i], i + layofst, 3, 1, 1);

			nameEdits[ofst + i]->setMaximumWidth(150);
			minValEdits[ofst + i]->setMaximumWidth(100);
			maxValEdits[ofst + i]->setMaximumWidth(100);


			noteEdits[ofst + i] = new QLineEdit("");
			//noteEdits[ofst + i]->setText(input->aoSys->getNote(ofst + i).c_str());
			lay->addWidget(noteEdits[ofst + i], i + layofst, 4, 1, 2);
	
		}
		layout->addLayout(lay, 1);
		cnts++;
	}

	layout->insertSpacing(1, 30);
	layoutWiget->addLayout(layout, 1);

	QHBoxLayout* layoutbtns = new QHBoxLayout();
	okBtn = new QPushButton ("OK", this);
	connect (okBtn, &QPushButton::released, this, &AoSettingsDialog::handleOk);
	cancelBtn = new QPushButton ("Cancel", this);
	connect (cancelBtn, &QPushButton::released, this, &AoSettingsDialog::handleCancel);
	layoutbtns->addStretch(1);
	layoutbtns->addWidget(okBtn);
	layoutbtns->addWidget(cancelBtn);

	layoutWiget->addStretch(1);
	layoutWiget->addLayout(layoutbtns, 0);


}

void AoSettingsDialog::updateAllEdits()
{
	for (size_t i = 0; i < size_t(AOGrid::total); i++)
	{
		nameEdits[i]->setText(input->getName(i).c_str());
		auto [min, max] = input->getDacRange(i);
		minValEdits[i]->setText(QString::number(min));
		maxValEdits[i]->setText(QString::number(max));
		noteEdits[i]->setText(input->getNote(i).c_str());
	}

}



void AoSettingsDialog::handleOk()
{
	std::array<std::string, size_t(AOGrid::total)> namesIn;
	for (unsigned dacInc = 0; dacInc < nameEdits.size(); dacInc++){
		auto text = nameEdits[dacInc]->text ();
		if (text[0].isDigit()){
			errBox("ERROR: " + str(text) + " is an invalid name; names cannot start with numbers.");
			return;
		}
		input->setName(dacInc, str(text));
		namesIn[dacInc] = str(text);
		double min, max;
		try	{
			text = minValEdits[dacInc]->text ();
			min = boost::lexical_cast<double>(str(text));
			text = maxValEdits[dacInc]->text ();
			max = boost::lexical_cast<double>(str(text));
			input->setMinMax(dacInc, min, max);
		}
		catch ( boost::bad_lexical_cast& err) {
			errBox(err.what());
			return;
		}
		/*make sure set ampminmax is not the last one, relies on setname/setnote to update the tooltip*/
		text = noteEdits[dacInc]->text();
		input->setNote(dacInc, str(text));
	}
	input->getCore().setNames(namesIn);
	emit updateSyntaxHighLight();
	close ();
}

void AoSettingsDialog::handleCancel(){
	close();
}
