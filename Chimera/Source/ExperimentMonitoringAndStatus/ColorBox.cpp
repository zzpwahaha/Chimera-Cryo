// created by Mark O. Brown
#include "stdafx.h"
#include "ColorBox.h"
#include <tuple>
#include <PrimaryWindows/IChimeraQtWindow.h>

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
		this->addWidget(box.ctrl);
		box.ctrl->setStyleSheet ("QLabel { font: 8pt; }");
	}
	auto& box = boxes.back();
	box.delim = "Other";
	box.ctrl = new QLabel (qstr(box.delim), parent);
	box.ctrl->setToolTip (box.delim.c_str ());
	this->addWidget(box.ctrl);
	box.ctrl->setStyleSheet ("QLabel { font: 8pt; }");

	initialized = true;
}

void ColorBox::changeColor( std::string delim, std::string color ){
	bool foundMatch = false;
	for (auto& device : boxes){
		if (device.delim == delim){
			foundMatch = true;
			device.color = color;
			device.ctrl->setStyleSheet (("QLabel {background-color: \"" + str(color) + "\"; font: 8pt;}").c_str());
		}
	}
	if (!foundMatch) {
		boxes.back ().color = color;
		boxes.back ().ctrl->setStyleSheet (("QLabel {background-color: \"" + str (color) + "\"; font: 8pt;}").c_str ());
	}
}