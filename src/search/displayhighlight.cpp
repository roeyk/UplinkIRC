// Search result text cleanup and match highlighting.
//
// This module strips IRC/ANSI presentation controls for `--textonly` and adds
// IRC highlight controls around positive text/mention predicate matches. It is
// used by `display.cpp` while formatting `/search`, `/last`, and `/mentions`
// rows. It only transforms strings; it does not inspect channels or send IRC
// traffic.

#include "search/internal.h"

#include <QRegularExpression>

#include <algorithm>

namespace RichSearch::Internal {
namespace {

struct TextRange {
    qsizetype start{0};
    qsizetype length{0};
};

// Adds one non-empty match range to the local list.
//
// Highlight builders call this after literal or regex matching. `start` and
// `length` are indexes into the display body. Empty matches are ignored so
// patterns such as `^` cannot create zero-width formatting loops.
void addRange(QList<TextRange> *ranges, qsizetype start, qsizetype length)
{
    if (start < 0 || length <= 0)
        return;

    ranges->append({start, length});
}

// Records literal body matches for one predicate.
//
// Used by `highlightedBody` for non-regex text selectors. `body` is the text
// being rendered and `predicate` supplies the literal and case mode.
void addLiteralRanges(const QString &body, const Predicate &predicate,
                      QList<TextRange> *ranges)
{
    const Qt::CaseSensitivity sensitivity = predicate.caseInsensitive
        ? Qt::CaseInsensitive
        : Qt::CaseSensitive;
    qsizetype pos = 0;

    // Walk all non-overlapping literal occurrences in display order.
    while ((pos = body.indexOf(predicate.value, pos, sensitivity)) != -1) {
        addRange(ranges, pos, predicate.value.size());
        pos += qMax<qsizetype>(1, predicate.value.size());
    }
}

// Records regex body matches for one predicate.
//
// Used by `highlightedBody` for regex text selectors. Invalid expressions are
// ignored here because parser validation has already reported syntax errors.
void addRegexRanges(const QString &body, const Predicate &predicate,
                    QList<TextRange> *ranges)
{
    QRegularExpression::PatternOptions options;

    // Case-insensitive matching is delegated to Qt's regex engine.
    if (predicate.caseInsensitive)
        options |= QRegularExpression::CaseInsensitiveOption;

    const QRegularExpression rx(predicate.value, options);
    if (!rx.isValid())
        return;

    auto it = rx.globalMatch(body);

    // Add each non-empty regex match exactly as Qt reports it.
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        addRange(ranges, match.capturedStart(), match.capturedLength());
    }
}

// Records mention selector matches with IRC nick boundaries.
//
// Used by `highlightedBody` for literal `mention=` predicates. The captured
// nick itself is highlighted, while boundary punctuation remains unmarked.
void addMentionRanges(const QString &body, const Predicate &predicate,
                      QList<TextRange> *ranges)
{
    const QString nickChars = QStringLiteral("A-Za-z0-9_\\-\\[\\]\\\\`^{}|");
    const QRegularExpression rx(
        QStringLiteral("(^|[^%1])(%2)($|[^%1])")
            .arg(nickChars, QRegularExpression::escape(predicate.value)),
        predicate.caseInsensitive
            ? QRegularExpression::CaseInsensitiveOption
            : QRegularExpression::NoPatternOption);
    auto it = rx.globalMatch(body);

    // Capture group 2 is the nick, excluding leading/trailing boundaries.
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        addRange(ranges, match.capturedStart(2), match.capturedLength(2));
    }
}

// Merges overlapping highlight ranges into one stable range list.
//
// Used before inserting IRC color controls. This prevents nested formatting
// markers when multiple predicates match the same body text.
QList<TextRange> mergedRanges(QList<TextRange> ranges)
{
    std::sort(ranges.begin(), ranges.end(), [](const TextRange &left, const TextRange &right) {
        return left.start == right.start
            ? left.length < right.length
            : left.start < right.start;
    });

    QList<TextRange> merged;
    for (const TextRange &range : ranges) {
        if (merged.isEmpty()) {
            merged.append(range);
            continue;
        }

        TextRange &last = merged.last();
        const qsizetype lastEnd = last.start + last.length;

        // Overlapping or touching ranges become one continuous highlight.
        if (range.start <= lastEnd) {
            last.length = qMax(lastEnd, range.start + range.length) - last.start;
            continue;
        }

        merged.append(range);
    }

    return merged;
}

} // namespace

QString stripControlCodes(const QString &text)
{
    QString out;
    out.reserve(text.size());

    // Walk the string by character so variable-length mIRC color and ANSI
    // escape payloads can be consumed safely.
    for (int i = 0; i < text.size(); ++i) {
        const QChar ch = text[i];

        // Drop simple one-byte IRC formatting controls such as bold, reset,
        // reverse, italic, and underline.
        if (ch == u'\x02' || ch == u'\x0f' || ch == u'\x16'
            || ch == u'\x1d' || ch == u'\x1f') {
            continue;
        }

        // Drop mIRC color controls and their optional foreground/background
        // numeric payloads.
        if (ch == u'\x03') {
            // Consume up to two foreground color digits.
            for (int n = 0; n < 2 && i + 1 < text.size()
                 && text[i + 1].isDigit(); ++n) {
                ++i;
            }

            // Consume an optional comma and up to two background color digits.
            if (i + 1 < text.size() && text[i + 1] == u',') {
                ++i;
                for (int n = 0; n < 2 && i + 1 < text.size()
                     && text[i + 1].isDigit(); ++n) {
                    ++i;
                }
            }
            continue;
        }

        // Drop ANSI CSI sequences. Malformed sequences are consumed only until
        // a normal CSI terminator range is seen or the string ends.
        if (ch == u'\x1b' && i + 1 < text.size() && text[i + 1] == u'[') {
            ++i;
            while (i + 1 < text.size()) {
                ++i;
                const ushort c = text[i].unicode();
                if (c >= 0x40 && c <= 0x7e)
                    break;
            }
            continue;
        }

        // Preserve normal display characters exactly.
        out.append(ch);
    }

    return out;
}

QString highlightedBody(const QString &body, const QList<Predicate> &predicates)
{
    QList<TextRange> ranges;

    // Only positive message-body selectors produce visible match highlights.
    for (const Predicate &predicate : predicates) {
        if (predicate.negated)
            continue;

        if (predicate.field == Field::Text) {
            if (predicate.regex)
                addRegexRanges(body, predicate, &ranges);
            else
                addLiteralRanges(body, predicate, &ranges);
        } else if (predicate.field == Field::Mention) {
            if (predicate.regex)
                addRegexRanges(body, predicate, &ranges);
            else
                addMentionRanges(body, predicate, &ranges);
        }
    }

    const QList<TextRange> merged = mergedRanges(ranges);
    if (merged.isEmpty())
        return body;

    QString out;
    qsizetype pos = 0;

    // mIRC color 1-on-8 gives a high-contrast highlight. A bare color control
    // closes the generated color span without resetting unrelated formatting.
    for (const TextRange &range : merged) {
        out += body.mid(pos, range.start - pos);
        out += QStringLiteral("\x03""01,08");
        out += body.mid(range.start, range.length);
        out += QChar(u'\x03');
        pos = range.start + range.length;
    }

    out += body.mid(pos);
    return out;
}

} // namespace RichSearch::Internal
