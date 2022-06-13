// created by Mark O. Brown
#include "stdafx.h"
#include "ColorBox.h"
#include <tuple>
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <qlayout.h>

ColorBox::ColorBox(IChimeraQtWindow* parent, DeviceList devices) 
{
	parent->setStatusBar(this);

	auto numCtrls = devices.list.size ()+1;
	boxes.resize (numCtrls);
	for (auto devInc : range (numCtrls-1)) {
		auto& box = boxes[devInc];
		auto& dev = devices.list[devInc].get ();
		box.delim = dev.getDelim ();
		box.ctrl = new QLabel(box.delim.c_str(), parent);
		this->addWidget(box.ctrl, 0);
		box.ctrl->setStyleSheet ("QLabel { font: 12pt; }");
	}
	//zynqProg =new QProgressBar(parent);
	//zynqProg->setRange(0, 100);
	//zynqProg->setMaximumWidth(50);
	//zynqProg->setTextVisible(false);
	//zynqProg->setAlignment(Qt::AlignCenter);
	//QWidget* container = new QWidget();
	//QHBoxLayout* layout = new QHBoxLayout(container);
	//layout->setContentsMargins(0, 0, 0, 0);
	//QLabel* zynqL = new QLabel("Zynq");
	//zynqL->setStyleSheet("QLabel { font: 8pt; }");
	//layout->addWidget(zynqL,0, Qt::AlignCenter);
	//layout->addWidget(zynqProg, 0, Qt::AlignCenter);
	//this->addWidget(container, 0);


	auto& box = boxes.back();
	box.delim = "Other";
	box.ctrl = new QLabel (qstr(box.delim), parent);
	box.ctrl->setToolTip (box.delim.c_str ());
	this->addWidget(box.ctrl, 0);
	box.ctrl->setStyleSheet ("QLabel { font: 12pt; }");

	initialized = true;
}

void ColorBox::changeColor( std::string delim, std::string color ){
	bool foundMatch = false;
	for (auto& device : boxes){
		if (device.delim == delim){
			foundMatch = true;
			device.color = color;
			device.ctrl->setStyleSheet (("QLabel {background-color: \"" + str(color) + "\"; font: 12pt;}").c_str());
		}
	}
	if (!foundMatch) {
		boxes.back ().color = color;
		boxes.back ().ctrl->setStyleSheet (("QLabel {background-color: \"" + str (color) + "\"; font: 12pt;}").c_str ());
	}
}