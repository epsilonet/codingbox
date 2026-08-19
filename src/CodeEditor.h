#pragma once

#include <QPlainTextEdit>

class QWidget;
class QPaintEvent;
class QResizeEvent;

class CodeEditor final : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int deltaY);
    void highlightCurrentLine();

private:
    QWidget *lineNumberArea_;
};

class LineNumberArea final : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), editor_(editor) {}
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    CodeEditor *editor_;
};
