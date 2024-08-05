#include "stdafx.h"
#include "CQCodeEdit.h"
#include <QPlainTextEdit>
#include <qscrollbar.h>
#include <QTextBlock>
#include <QPainter>



LineNumberArea::LineNumberArea(QTextEdit* editor) : QWidget(editor) {
    codeEditor = editor;
}

QSize LineNumberArea::sizeHint() const {
    return QSize(((CQCodeEdit*)codeEditor)->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent* event) {
    ((CQCodeEdit*)codeEditor)->lineNumberAreaPaintEvent(event);
}


CQCodeEdit::CQCodeEdit(IChimeraQtWindow* parent) :
    CQTextEdit(parent)
{
    // Line numbers
    lineNumberArea = new LineNumberArea(this);
    ///
    connect(this->document(), &QTextDocument::blockCountChanged, this, &CQCodeEdit::updateLineNumberAreaWidth);
    connect(this->verticalScrollBar(), &QScrollBar::valueChanged, this, qOverload<int>(&CQCodeEdit::updateLineNumberArea));
    connect(this, &QTextEdit::textChanged, this, qOverload<>(&CQCodeEdit::updateLineNumberArea));
    connect(this, &QTextEdit::cursorPositionChanged, this, qOverload<>(&CQCodeEdit::updateLineNumberArea));
    ///
    updateLineNumberAreaWidth(0);
}

int CQCodeEdit::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, this->document()->blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 13 + fontMetrics().width(QLatin1Char('9')) * (digits);

    return space;
}

void CQCodeEdit::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}


void CQCodeEdit::updateLineNumberArea(QRectF /*rect_f*/)
{
    CQCodeEdit::updateLineNumberArea();
}
void CQCodeEdit::updateLineNumberArea(int /*slider_pos*/)
{
    CQCodeEdit::updateLineNumberArea();
}
void CQCodeEdit::updateLineNumberArea()
{
    /*
     * When the signal is emitted, the sliderPosition has been adjusted according to the action,
     * but the value has not yet been propagated (meaning the valueChanged() signal was not yet emitted),
     * and the visual display has not been updated. In slots connected to this signal you can thus safely
     * adjust any action by calling setSliderPosition() yourself, based on both the action and the
     * slider's value.
     */
     // Make sure the sliderPosition triggers one last time the valueChanged() signal with the actual value !!!!
    this->verticalScrollBar()->setSliderPosition(this->verticalScrollBar()->sliderPosition());

    // Since "QTextEdit" does not have an "updateRequest(...)" signal, we chose
    // to grab the informations from "sliderPosition()" and "contentsRect()".
    // See the necessary connections used (Class constructor implementation part).

    QRect rect = this->contentsRect();
    lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());
    updateLineNumberAreaWidth(0);
    //----------
    int dy = this->verticalScrollBar()->sliderPosition();
    if (dy > -1) {
        lineNumberArea->scroll(0, dy);
    }

    ///zzp disabled feature below
    // Addjust slider to alway see the number of the currently being edited line...
    //int first_block_id = getFirstVisibleBlockId();
    //if (first_block_id == 0 || this->textCursor().block().blockNumber() == first_block_id - 1)
    //    this->verticalScrollBar()->setSliderPosition(dy - this->document()->documentMargin());

}


void CQCodeEdit::resizeEvent(QResizeEvent* e)
{
    QTextEdit::resizeEvent(e);

    QRect cr = this->contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


int CQCodeEdit::getFirstVisibleBlockId()
{
    // Detect the first block for which bounding rect - once translated 
    // in absolute coordinated - is contained by the editor's text area

    // Costly way of doing but since "blockBoundingGeometry(...)" doesn't 
    // exists for "QTextEdit"...

    QTextCursor curs = QTextCursor(this->document());
    curs.movePosition(QTextCursor::Start);
    for (int i = 0; i < this->document()->blockCount(); ++i)
    {
        QTextBlock block = curs.block();
        this->document();
        QRect r1 = this->viewport()->geometry();
        QRect r2 = this->document()->documentLayout()->blockBoundingRect(block).translated(
            this->viewport()->geometry().x(), this->viewport()->geometry().y() - (
                this->verticalScrollBar()->sliderPosition()
                )).toRect();

        if (r1.contains(r2, true)) { return i; }

        curs.movePosition(QTextCursor::NextBlock);
    }

    return 0;
}

void CQCodeEdit::lineNumberAreaPaintEvent(QPaintEvent* event)
{
    this->verticalScrollBar()->setSliderPosition(this->verticalScrollBar()->sliderPosition());

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor(230, 231, 232)/*Qt::lightGray*/);
    int blockNumber = this->getFirstVisibleBlockId();

    QTextBlock block = this->document()->findBlockByNumber(blockNumber);
    QTextBlock prev_block = (blockNumber > 0) ? this->document()->findBlockByNumber(blockNumber - 1) : block;
    int translate_y = (blockNumber > 0) ? -this->verticalScrollBar()->sliderPosition() : 0;

    int top = this->viewport()->geometry().top();

    // Adjust text position according to the previous "non entirely visible" block 
    // if applicable. Also takes in consideration the document's margin offset.
    int additional_margin = 0;
    if (blockNumber == 0)
        // Simply adjust to document's margin
        additional_margin = (int)this->document()->documentMargin() - 1 - this->verticalScrollBar()->sliderPosition();
    else
        // Getting the height of the visible part of the previous "non entirely visible" block
        additional_margin = (int)this->document()->documentLayout()->blockBoundingRect(prev_block)
        .translated(0, translate_y).intersected(this->viewport()->geometry()).height();

    // Shift the starting point
    top += additional_margin;

    int bottom = top + (int)this->document()->documentLayout()->blockBoundingRect(block).height();

    QColor col_1(11, 33, 111 /*90, 255, 30*/);      // Current line (custom green)
    QColor col_0(50, 120, 147/*120, 120, 120*/);    // Other lines  (custom darkgrey)
    QFont font_1 = QFont(painter.font());
    font_1.setBold(true);
    QFont font_0 = QFont(painter.font());
    font_0.setBold(false);
    // Draw the numbers (displaying the current line number in green)
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen((this->textCursor().blockNumber() == blockNumber) ? col_1 : col_0);
            painter.setFont((this->textCursor().blockNumber() == blockNumber) ? font_1 : font_0);
            painter.drawText(-5, top,
                lineNumberArea->width(), fontMetrics().height(),
                Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int)this->document()->documentLayout()->blockBoundingRect(block).height();
        ++blockNumber;
    }

}