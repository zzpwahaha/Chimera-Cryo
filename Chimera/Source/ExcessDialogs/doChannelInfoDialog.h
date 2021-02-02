#pragma once

#include <QDialog.h>
#include <qlabel.h>
#include <QLineEdit.h>
#include <QPushButton.h>
#include <array>

#include <DigitalOutput/DoSystem.h>


struct ttlInputStruct{
	DoSystem* ttls;
};

class doChannelInfoDialog : public QDialog
{
	Q_OBJECT;
	public:
		doChannelInfoDialog (DoSystem* inputPtr);
		void updateAllEdits();
	public Q_SLOTS:
		void handleOk ();
		void handleCancel ();
	private:
		DoSystem* input;
		std::array<QLabel*, size_t(DOGrid::total)> numberlabels;
		std::array<QLabel*, size_t(DOGrid::numOFunit)> rowLabels;
		std::array<std::array<QLineEdit*, size_t(DOGrid::numPERunit)>, size_t(DOGrid::numOFunit)> edits;
		QPushButton * okBtn, * cancelBtn;
};
