#include "stdafx.h"

#include <ExcessDialogs/doChannelInfoDialog.h>
#include <qlayout.h>

doChannelInfoDialog::doChannelInfoDialog (DoSystem* inputPtr)
{
	this->setModal(false);
	input = inputPtr;
	QVBoxLayout* layout = new QVBoxLayout(this);
	QGridLayout* DOGridLayout = new QGridLayout();
	//auto names = input->getAllNames();
	unsigned runningCount = 0;
	for (auto row : range(size_t(DOGrid::numOFunit))) 
	{
		runningCount++;
		QHBoxLayout* DOsubGridLayout = new QHBoxLayout();
		DOsubGridLayout->addWidget(new QLabel(QString::number(int(row) + 1)), 0, Qt::AlignRight);

		for (size_t number = 0; number < size_t(DOGrid::numPERunit); number++) 
		{
			edits[row][number] = new QLineEdit("");
			//edits[row][number]->setText(names(row, number).c_str());
			DOsubGridLayout->addWidget(edits[row][number]);
		}
		//DOsubGridLayout->setSpacing(8);
		DOGridLayout->addLayout(DOsubGridLayout, runningCount % 3, 2 - runningCount / 3);
	}
	//DOGridLayout->setHorizontalSpacing(20);
	//DOGridLayout->setVerticalSpacing(12);

	layout->addLayout(DOGridLayout, 1);
	layout->addStretch(1);

	//long columnWidth = 120;
	//long labelSize = 65;
	//long rowSize = 30;
	//int px=labelSize, py = 0;
	//for (unsigned numInc : range (edits.front ().size ())) 
	//{
	//	numberlabels[numInc] = new QLabel (qstr (numInc), this);
	//	numberlabels[numInc]->setGeometry (px, py, columnWidth, rowSize);
	//	numberlabels[numInc]->setAlignment (Qt::AlignCenter);
	//	px += columnWidth;
	//}
	//for (auto row : DoRows::allRows)
	//{
	//	py += rowSize;
	//	px = 0;
	//	rowLabels[int (row)] = new QLabel (qstr (DoRows::toStr (row)), this); 
	//	rowLabels[int (row)]->setGeometry (px, py, labelSize, rowSize);
	//	rowLabels[int (row)]->setAlignment (Qt::AlignCenter);
	//	px += labelSize;
	//	for (unsigned numberInc : range( edits[int (row)].size ())){
	//		edits[int (row)][numberInc] = new QLineEdit (this);
	//		edits[int (row)][numberInc]->setGeometry (px, py, columnWidth, rowSize);
	//		edits[int (row)][numberInc]->setText (qstr (input->ttls->getName (row, numberInc)));
	//		edits[int (row)][numberInc]->setToolTip("Original: " + qstr (input->ttls->getName (row, numberInc)));
	//		px += columnWidth;
	//	}
	//}
	//px = 0;
	//py += rowSize;
	QHBoxLayout* layoutBtns = new QHBoxLayout();
	okBtn = new QPushButton ("OK", this);
	connect (okBtn, &QPushButton::released, this, &doChannelInfoDialog::handleOk);
	cancelBtn = new QPushButton ("CANCEL", this);
	connect (cancelBtn, &QPushButton::released, this, &doChannelInfoDialog::handleCancel);
	
	layoutBtns->addStretch(1);
	layoutBtns->addWidget(okBtn, 0);
	layoutBtns->addWidget(cancelBtn, 0);

	layout->addLayout(layoutBtns, 0);

	this->setLayout(layout);
}

void doChannelInfoDialog::updateAllEdits()
{
	auto names = input->getAllNames();
	for (auto row : range(size_t(DOGrid::numOFunit)))
	{
		for (size_t number = 0; number < size_t(DOGrid::numPERunit); number++)
		{
			edits[row][number]->setText(names(row, number).c_str());
		}
	}
	
}

void doChannelInfoDialog::handleOk (){
	for (auto row : DoRows::allRows){
		for (unsigned numberInc = 0; numberInc < edits[int (row)].size (); numberInc++)	
		{
			QString name = edits[int(row)][numberInc]->text();
			if (name[0].isDigit ())
			{
				errBox ("ERROR: " + str (name) + " is an invalid name; names cannot start with numbers.");
				return;
			}
			input->setName(row, numberInc, name.toStdString());
		}
	}
	close ();
}

void doChannelInfoDialog::handleCancel (){
	close ();
}

