#include "stdafx.h"
#include "MessageDisplay.h"

MessageDisplay::MessageDisplay(int maxLines, IChimeraQtWindow* parent)
    : CQCodeEdit(parent), maxLines(maxLines) {}

void MessageDisplay::appendText(const QString& text)
{
    append(text);
    ensureLineLimit();
}


void MessageDisplay::ensureLineLimit()
{
    QTextDocument* doc = document();
    int blockCount = doc->blockCount();

    if (blockCount > maxLines) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::Start);
        for (int i = 0; i < blockCount - maxLines; ++i) {
            cursor.select(QTextCursor::LineUnderCursor);
            cursor.removeSelectedText();
            cursor.deleteChar();  // Remove the newline character
        }
    }
}
