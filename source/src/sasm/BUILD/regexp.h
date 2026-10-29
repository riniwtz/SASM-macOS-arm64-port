#ifndef SASM_REGEXP_H
#define SASM_REGEXP_H
#include <QRegularExpression>
// Retain the listing parsers' last-match API while using Qt 6's regex engine.
class SasmRegExp {
    QRegularExpression expression;
    mutable QRegularExpressionMatch lastMatch;
public:
    SasmRegExp(const QString &pattern = QString()) : expression(pattern) {}
    operator QRegularExpression() const { return expression; }
    void setCaseSensitivity(Qt::CaseSensitivity sensitivity) {
        auto flags = expression.patternOptions();
        flags.setFlag(QRegularExpression::CaseInsensitiveOption, sensitivity == Qt::CaseInsensitive);
        expression.setPatternOptions(flags);
    }
    void setMinimal(bool minimal) {
        auto flags = expression.patternOptions();
        flags.setFlag(QRegularExpression::InvertedGreedinessOption, minimal);
        expression.setPatternOptions(flags);
    }
    int indexIn(const QString &text, int offset = 0) const {
        lastMatch = expression.match(text, offset);
        return lastMatch.hasMatch() ? int(lastMatch.capturedStart()) : -1;
    }
    bool exactMatch(const QString &text) const {
        QRegularExpression anchored(QRegularExpression::anchoredPattern(expression.pattern()), expression.patternOptions());
        lastMatch = anchored.match(text);
        return lastMatch.hasMatch();
    }
    int matchedLength() const { return lastMatch.hasMatch() ? int(lastMatch.capturedLength()) : -1; }
    QString cap(int index = 0) const { return lastMatch.captured(index); }
};
#endif
