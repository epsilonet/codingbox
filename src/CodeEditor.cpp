#include "CodeEditor.h"

#include <QPainter>
#include <QFrame>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextFormat>
#include <QScrollBar>

LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget(editor), editor_(editor) {}

QSize LineNumberArea::sizeHint() const {
    return QSize(editor_->lineNumberAreaWidth(), 0);
}

void LineNumberArea::paintEvent(QPaintEvent *event) {
    editor_->lineNumberAreaPaintEvent(event);
}

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent), lineNumberArea_(new LineNumberArea(this)) {
    setFrameStyle(QFrame::NoFrame);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * 2);
    setCursorWidth(2);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int CodeEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int maximum = qMax(1, blockCount());
    while (maximum >= 10) {
        maximum /= 10;
        ++digits;
    }
    return 18 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int deltaY) {
    if (deltaY) {
        lineNumberArea_->scroll(0, deltaY);
    } else {
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());
    }

    if (rect.contains(viewport()->rect())) {
        updateLineNumberAreaWidth(0);
    }
}

void CodeEditor::resizeEvent(QResizeEvent *event) {
    QPlainTextEdit::resizeEvent(event);
    const QRect contents = contentsRect();
    lineNumberArea_->setGeometry(QRect(contents.left(), contents.top(),
                                       lineNumberAreaWidth(), contents.height()));
}

void CodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> selections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(QColor("#0e2b5f"));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }
    setExtraSelections(selections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event) {
    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), QColor("#081d43"));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            const bool current = textCursor().blockNumber() == blockNumber;
            painter.setPen(QColor(current ? "#79ceff" : "#4d7eb3"));
            painter.drawText(0, top, lineNumberArea_->width() - 8,
                             fontMetrics().height(), Qt::AlignRight,
                             QString::number(blockNumber + 1));
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
