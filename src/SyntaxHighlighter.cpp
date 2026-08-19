#include "SyntaxHighlighter.h"

#include <QTextDocument>
#include <QFont>
#include <QStringList>

namespace {
QTextCharFormat ink(const QString &color, bool bold = false) {
    QTextCharFormat format;
    format.setForeground(QColor(color));
    if (bold) format.setFontWeight(QFont::DemiBold);
    return format;
}
}

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent),
      commentStart_(QStringLiteral("/\\*")),
      commentEnd_(QStringLiteral("\\*/")) {
    const auto keyword = ink("#7fd7ff", true);
    const auto type = ink("#b6a7ff");
    const auto literal = ink("#ffd18a");
    const auto function = ink("#7de0b5");
    const auto comment = ink("#6b95b9");

    const QStringList keywords = {
        "alignas", "alignof", "auto", "await", "break", "case", "catch", "class",
        "const", "constexpr", "continue", "default", "delete", "do", "else", "enum",
        "export", "extends", "for", "from", "function", "if", "import", "in", "let",
        "namespace", "new", "operator", "private", "protected", "public", "return",
        "static", "struct", "switch", "template", "this", "throw", "try", "typedef",
        "typename", "using", "virtual", "void", "while", "yield"
    };
    for (const auto &word : keywords) {
        rules_.append({QRegularExpression(QStringLiteral("\\b%1\\b").arg(word)), keyword});
    }

    const QStringList types = {"bool", "char", "double", "float", "int", "long", "QString",
                               "size_t", "std", "string", "true", "false", "null", "nullptr"};
    for (const auto &word : types) {
        rules_.append({QRegularExpression(QStringLiteral("\\b%1\\b").arg(word)), type});
    }

    rules_.append({QRegularExpression(QStringLiteral("\\b[0-9]+(?:\\.[0-9]+)?\\b")), literal});
    rules_.append({QRegularExpression(QStringLiteral("\"[^\"\\n]*\"|'[^'\\n]*'")), literal});
    rules_.append({QRegularExpression(QStringLiteral("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\s*\\()")), function});
    rules_.append({QRegularExpression(QStringLiteral("//[^\\n]*")), comment});
    multiLineCommentFormat_ = comment;
}

void SyntaxHighlighter::highlightBlock(const QString &text) {
    for (const HighlightRule &rule : rules_) {
        QRegularExpressionMatchIterator matches = rule.expression.globalMatch(text);
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1) startIndex = text.indexOf(commentStart_);

    while (startIndex >= 0) {
        const QRegularExpressionMatch endMatch = commentEnd_.match(text, startIndex);
        int commentLength;
        if (!endMatch.hasMatch()) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endMatch.capturedEnd() - startIndex;
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat_);
        startIndex = text.indexOf(commentStart_, startIndex + commentLength);
    }
}
