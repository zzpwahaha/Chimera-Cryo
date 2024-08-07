#pragma once
#include <CustomQtControls/CQCodeEdit.h>

class MessageDisplay : public CQCodeEdit
{
	Q_OBJECT
public:
    MessageDisplay(int maxLines = 100, IChimeraQtWindow* parent = nullptr);

private:
    void ensureLineLimit(); 

public slots:
    void appendText(const QString& text);


private:
    int maxLines;
};

