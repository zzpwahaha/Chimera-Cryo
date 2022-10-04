#include "stdafx.h"
#include "AiSettingsDialog.h"
#include <qboxlayout.h>

AiSettingsDialog::AiSettingsDialog(AiSystem* inputPtr)
{
	input = inputPtr;
	//this->setStyleSheet("border: 2px solid  black;");
	this->setModal(false);
	QVBoxLayout* layoutWiget = new QVBoxLayout(this);
	QGridLayout* layout = new QGridLayout();
	std::array<std::string, 2> chnlStr = { "A","B" };
	unsigned x = 4, y = 4;
	for (auto idx : range(size_t(AIGrid::total)))
	{
		numberLabels[idx] = new QLabel(qstr(chnlStr[idx / size_t(AIGrid::numPERunit)] + str(idx % size_t(AIGrid::numPERunit))));
		nameEdits[idx] = new QLineEdit();
		noteEdits[idx] = new QLineEdit();
		auto rowNum = 2 * (idx % size_t(AIGrid::numPERunit)) / x;
		auto colNum = (2 * idx + idx / size_t(AIGrid::numPERunit)) % x;
		layout->addWidget(numberLabels[idx], rowNum + 1, 3 * colNum);
		layout->addWidget(nameEdits[idx], rowNum + 1, 3 * colNum + 1);
		layout->addWidget(noteEdits[idx], rowNum + 1, 3 * colNum + 2);
		if (rowNum == 0) {
			layout->addWidget(new QLabel("Ai Num."), rowNum, 3 * colNum);
			layout->addWidget(new QLabel("Ai Name"), rowNum, 3 * colNum + 1);
			layout->addWidget(new QLabel("Ai Note"), rowNum, 3 * colNum + 2);
		}
	}

	layoutWiget->addLayout(layout, 1);

	QHBoxLayout* layoutbtns = new QHBoxLayout();
	okBtn = new QPushButton("OK", this);
	connect(okBtn, &QPushButton::released, this, &AiSettingsDialog::handleOk);
	cancelBtn = new QPushButton("Cancel", this);
	connect(cancelBtn, &QPushButton::released, this, &AiSettingsDialog::handleCancel);
	layoutbtns->addStretch(1);
	layoutbtns->addWidget(okBtn);
	layoutbtns->addWidget(cancelBtn);

	layoutWiget->addStretch(1);
	layoutWiget->addLayout(layoutbtns, 0);
}

void AiSettingsDialog::updateAllEdits()
{
	for (size_t i = 0; i < size_t(AIGrid::total); i++)
	{
		nameEdits[i]->setText(input->getName(i).c_str());
		noteEdits[i]->setText(input->getNote(i).c_str());
	}
}

void AiSettingsDialog::handleOk()
{
	for (unsigned aiInc = 0; aiInc < size_t(AIGrid::total); aiInc++)
	{
		auto text = nameEdits[aiInc]->text();
		if (text[0].isDigit()) {
			errBox("ERROR: " + str(text) + " is an invalid name; names cannot start with numbers.");
			return;
		}
		input->setName(aiInc, str(text));
		text = noteEdits[aiInc]->text();
		input->setNote(aiInc, str(text));

	}
	input->updateCoreNames(); 
	close();
}

void AiSettingsDialog::handleCancel()
{
	close();
}
