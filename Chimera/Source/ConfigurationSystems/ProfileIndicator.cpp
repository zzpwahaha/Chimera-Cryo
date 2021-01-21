// created by Mark O. Brown
#include "stdafx.h"
#include "ProfileIndicator.h"
#include "GeneralUtilityFunctions/my_str.h"

void ProfileIndicator::initialize(QWidget* parent ) {
	QHBoxLayout* layout = new QHBoxLayout(this);
	setLayout(layout);
	header = new QLabel("Congfiguration: ", parent);
	indicator = new QLabel("placeholder", parent);
	layout->addWidget(header, 0, Qt::AlignTop);
	layout->addWidget(indicator, 1, Qt::AlignTop);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(widgetWidthMax);

}

void ProfileIndicator::update(std::string text)
{
	indicator->setText(cstr(text));
}

