#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class SyntaxHighlighter final : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression expression;
        QTextCharFormat format;
    };

    QVector<HighlightRule> rules_;
    QRegularExpression commentStart_;
    QRegularExpression commentEnd_;
    QTextCharFormat multiLineCommentFormat_;
};
