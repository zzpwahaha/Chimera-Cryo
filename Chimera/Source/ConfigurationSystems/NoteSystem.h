// created by Mark O. Brown
#pragma once

#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "GeneralObjects/commonTypes.h"

#include <QLabel>
#include <QTextEdit>
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <CustomQtControls/AutoNotifyCtrls.h>

#include <string>

class NoteSystem : public QWidget
{
	Q_OBJECT
	public:
		void handleSaveConfig(ConfigStream& saveFile);
		void handleOpenConfig(ConfigStream& openFile );
		void setConfigurationNotes(std::string notes);
		void initialize( IChimeraQtWindow* parent );
		std::string getConfigurationNotes();
		bool eventFilter(QObject* obj, QEvent* event);

	protected:
		//void mouseDoubleClickEvent(QMouseEvent* event) override;
	private:
		QLabel* header;
		CQTextEdit* edit;
		int editZoom;
};
