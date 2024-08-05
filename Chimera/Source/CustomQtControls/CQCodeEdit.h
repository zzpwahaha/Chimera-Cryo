#pragma once
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <PrimaryWindows/IChimeraQtWindow.h>
#include "qwidget.h"

class LineNumberArea;

class CQCodeEdit :
    public CQTextEdit
{
    Q_OBJECT
    
public:

    explicit CQCodeEdit(IChimeraQtWindow* parent = 0);

    int getFirstVisibleBlockId();
    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();

signals:


public slots:

    void resizeEvent(QResizeEvent* e);

private slots:

    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(QRectF /*rect_f*/);
    void updateLineNumberArea(int /*slider_pos*/);
    void updateLineNumberArea();

private:

    QWidget* lineNumberArea;

};

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    LineNumberArea(QTextEdit* editor);

    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent* event);

private:
    QTextEdit* codeEditor;
};

