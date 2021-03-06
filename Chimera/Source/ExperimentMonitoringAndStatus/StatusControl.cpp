// created by Mark O. Brown
#include "stdafx.h"
#include "StatusControl.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <qlayout.h>

void StatusControl::initialize(IChimeraQtWindow* parent, std::string headerText, std::vector<std::string> textColors)
{
	
	QVBoxLayout* layout = new QVBoxLayout(this);
	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(widgetWidthMax);

	if (textColors.size () == 0) {
		thrower ("Need to set a nonzero number of colors for status control!");
	}
	colors = textColors;
	//defaultColor = textColor;
	header = new QLabel (headerText.c_str (), parent);	
	debugLevelLabel = new QLabel ("Debug Level", parent);	
	debugLevelEdit = new CQLineEdit (parent);
	debugLevelEdit->setText ("-1");
	parent->connect (debugLevelEdit, &QLineEdit::textChanged, [this]() {
		try {
			currentLevel = boost::lexical_cast<unsigned>(str (debugLevelEdit->text ()));
		}
		catch (boost::bad_lexical_cast&) {
			currentLevel = 0;
		}
		addStatusText ("Changed Debug Level to \"" + str (currentLevel) + "\"\n");
		});
	clearBtn = new QPushButton (parent);
	clearBtn->setText ("Clear");
	edit = new QPlainTextEdit (parent);
	edit->setReadOnly(true);
	edit->setStyleSheet("QPlainTextEdit { color: " + qstr(textColors[0]) + "; }");
	parent->connect(clearBtn, &QPushButton::released, [this]() {clear(); });

	layout1->addWidget(header, 0);
	layout1->addWidget(debugLevelLabel, 1);
	layout1->addWidget(debugLevelEdit, 0);
	layout1->addWidget(clearBtn, 0);
	layout->addLayout(layout1, 0);
	layout->addWidget(edit, 1);

}

void StatusControl::addStatusText (std::string text, unsigned level){
	if (colors.size () == 0) {
		return;
	}
	if (currentLevel >= level) {
		for (auto lvl : range (level)) {
			// visual indication of what level a message is.
			text = "> " + text;
		}
		if (level >= colors.size ()) {
			addStatusText (text, colors.back ());
		}
		else {
			addStatusText (text, colors[level]);
		}
	}
}

void StatusControl::addStatusText(std::string text, std::string color){
	QString htmlTxt = ("<font color = \"" + color + "\">" + text + "</font>").c_str();
	htmlTxt.replace ("\r", "");
	htmlTxt.replace ("\n", "<br/>");

	//e.g. <font color = "red">This is some text!< / font>
	//edit->appendHtml (htmlTxt);
	edit->moveCursor (QTextCursor::End);
	//edit->textCursor ().insertHtml (htmlTxt);
	//edit->insertHtml (htmlTxt);
	edit->insertPlainText (qstr(text));
	//edit->appendPlainText (qstr (text));
	edit->moveCursor (QTextCursor::End);
}

void StatusControl::clear() {
	edit->clear ();
	addStatusText("******************************\r\n", "#FFFFFF");
}


void StatusControl::appendTimebar() {
	time_t time_obj = time(0);   // get time now
	struct tm currentTime;
	localtime_s(&currentTime, &time_obj);
	std::string timeStr = "(" + str(currentTime.tm_year + 1900) + ":" + str(currentTime.tm_mon + 1) + ":"
		+ str(currentTime.tm_mday) + ") " + str(currentTime.tm_hour) + ":"
		+ str(currentTime.tm_min) + ":" + str(currentTime.tm_sec);
	addStatusText("\r\n**********" + timeStr + "**********\r\n", "#FFFFFF");
}

