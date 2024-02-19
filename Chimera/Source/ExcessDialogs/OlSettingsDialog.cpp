#include "stdafx.h"
#include "OlSettingsDialog.h"

OlSettingsDialog::OlSettingsDialog(OlSystem* inputPtr)
{
	input = inputPtr;
	//this->setStyleSheet("border: 2px solid  black;");
	this->setModal(false);
	QVBoxLayout* layoutWiget = new QVBoxLayout(this);
	QHBoxLayout* layout = new QHBoxLayout();
	auto ollayout = std::array<QGridLayout*, size_t(OLGrid::numOFunit)>(/*{ new QGridLayout() }*/);
	short cnts = 0;
	short ofst = 0;
	for (auto* lay : ollayout)
	{
		lay = new QGridLayout();
		lay->setContentsMargins(0, 0, 0, 0);
		lay->addWidget(new QLabel(QString("OffsetLock %1").arg(cnts)), 0, 0, 1, 7, Qt::AlignHCenter);
		{
			lay->addWidget(new QLabel("#"), 1, 0, 1, 1, Qt::AlignRight);
			lay->addWidget(new QLabel("OffsetLock Name"), 1, 1, 1, 1);
			//lay->addWidget(new QLabel("Min Amp"), 1, 2, 1, 1);
			//lay->addWidget(new QLabel("Max Amp"), 1, 3, 1, 1);
			lay->addWidget(new QLabel("Notes"), 1, 4, 1, 2);
		}
		auto layofst = 2;
		//auto ofst = cnts * size_t(OLGrid::numPERunit);
		for (size_t i = 0; i < input->getCore().getNumCmdChannel(cnts); i++)
		{
			numberLabels[ofst + i] = new QLabel(QString("%1").arg(i, 2));
			lay->addWidget(numberLabels[ofst + i], i + layofst, 0, 1, 1, Qt::AlignRight);

			nameEdits[ofst + i] = new QLineEdit("");
			lay->addWidget(nameEdits[ofst + i], i + layofst, 1, 1, 1);

			//auto [min, max] = input->aoSys->getDacRange(ofst + i);
			//minValEdits[ofst + i] = new QLineEdit("");
			//maxValEdits[ofst + i] = new QLineEdit("");
			//lay->addWidget(minValEdits[ofst + i], i + layofst, 2, 1, 1);
			//lay->addWidget(maxValEdits[ofst + i], i + layofst, 3, 1, 1);

			nameEdits[ofst + i]->setMaximumWidth(150);
			//minValEdits[ofst + i]->setMaximumWidth(100);
			//maxValEdits[ofst + i]->setMaximumWidth(100);


			noteEdits[ofst + i] = new QLineEdit("");
			noteEdits[ofst + i]->setMinimumWidth(150);
			lay->addWidget(noteEdits[ofst + i], i + layofst, 4, 1, 2);

		}
		layout->addLayout(lay, 1);
		ofst += input->getCore().getNumCmdChannel(cnts);
		cnts++;
	}

	layout->insertSpacing(1, 50);
	layout->insertSpacing(3, 50);
	layoutWiget->addLayout(layout, 1);

	QHBoxLayout* layoutbtns = new QHBoxLayout();
	okBtn = new QPushButton("OK", this);
	connect(okBtn, &QPushButton::released, this, &OlSettingsDialog::handleOk);
	cancelBtn = new QPushButton("Cancel", this);
	connect(cancelBtn, &QPushButton::released, this, &OlSettingsDialog::handleCancel);
	layoutbtns->addStretch(1);
	layoutbtns->addWidget(okBtn);
	layoutbtns->addWidget(cancelBtn);

	layoutWiget->addStretch(1);
	layoutWiget->addLayout(layoutbtns, 0);
}

void OlSettingsDialog::updateAllEdits()
{
	for (size_t i = 0; i < size_t(OLGrid::total); i++)
	{
		nameEdits[i]->setText(input->getName(i).c_str());
		//auto [min, max] = input->getAmplRange(i);
		//minValEdits[i]->setText(QString::number(min));
		//maxValEdits[i]->setText(QString::number(max));
		noteEdits[i]->setText(input->getNote(i).c_str());
	}
}

void OlSettingsDialog::handleOk()
{
	for (unsigned olInc = 0; olInc < size_t(OLGrid::total); olInc++)
	{
		auto text = nameEdits[olInc]->text();
		if (text[0].isDigit()) {
			errBox("ERROR: " + str(text) + " is an invalid name; names cannot start with numbers.");
			return;
		}
		input->setName(olInc, str(text));
		text = noteEdits[olInc]->text();
		input->setNote(olInc, str(text));

	}
	input->updateCoreNames();
	emit updateSyntaxHighLight();
	close();
}

void OlSettingsDialog::handleCancel() 
{
	close();
}