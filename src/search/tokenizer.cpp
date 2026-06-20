// Search-family command tokenizer.
//
// This module turns raw slash-command argument text into shell-like tokens with
// quote metadata. It is used by `/search`, `/mentions`, and `/common` parsers
// so they can distinguish selectors such as `origin=alice` from literal quoted
// text such as `"origin=alice"`. It performs no IO and produces syntax errors
// only through its output parameter.

#include "search/internal.h"

namespace RichSearch::Internal {

// Tokenizes a search-family command argument string.
//
// Called by `/search`, `/mentions`, and `/common` parsers. `input` is the raw
// text after the slash command and `error` receives syntax failures such as an
// unterminated quote. Returns shell-like tokens with enough quote metadata to
// distinguish `origin=alice` from `"origin=alice"`. Produces no side effects.
QList<Arg> tokenize(const QString &input, QString *error)
{
    QList<Arg> out;
    QString current;
    bool quoted = false;
    bool tokenQuoted = false;
    bool tokenStartedWithQuote = false;
    bool escaped = false;

    // Iterate over the raw text once, preserving escaped characters and
    // splitting only on unquoted whitespace or grouping parentheses.
    for (const QChar ch : input) {
        // If the previous character was a backslash, this character is literal
        // and cannot terminate quotes or split tokens.
        if (escaped) {
            current.append(ch);
            escaped = false;
            continue;
        }

        // A backslash quotes the next character. A trailing backslash is handled
        // after the loop by preserving it literally.
        if (ch == u'\\') {
            escaped = true;
            continue;
        }

        // Double quotes either start/end a quoted bare token or become part of
        // modifier syntax such as `text=i"deploy"`.
        if (ch == u'"') {
            // A quote at the start of a token marks the whole token as quoted,
            // so `"origin=bob"` becomes literal text rather than a selector.
            if (current.isEmpty() && !quoted) {
                tokenStartedWithQuote = true;

            // Modifier quotes are kept in the token so the predicate parser can
            // interpret prefixes like `i`, `n`, and `x`.
            } else if (!tokenStartedWithQuote) {
                current.append(ch);
            }
            quoted = !quoted;
            tokenQuoted = tokenStartedWithQuote;
            continue;
        }

        // Whitespace ends a token only outside quotes.
        if (!quoted && ch.isSpace()) {
            // Emit the completed token and reset quote metadata for the next
            // token.
            if (!current.isEmpty()) {
                out << Arg{current, tokenQuoted};
                current.clear();
                tokenQuoted = false;
                tokenStartedWithQuote = false;
            }
            continue;
        }

        // Parentheses are standalone expression tokens unless they appear
        // inside quotes.
        if (!quoted && (ch == u'(' || ch == u')')) {
            // Flush any token before the parenthesis.
            if (!current.isEmpty()) {
                out << Arg{current, tokenQuoted};
                current.clear();
                tokenQuoted = false;
                tokenStartedWithQuote = false;
            }
            out << Arg{QString(ch), false};
            continue;
        }

        // Normal character content stays in the current token.
        current.append(ch);
    }

    // Preserve a trailing backslash literally rather than dropping user input.
    if (escaped)
        current.append(u'\\');

    // Unterminated quotes are syntax errors for all search-family commands.
    if (quoted) {
        *error = QStringLiteral("unterminated quoted value");
        return {};
    }

    // Emit a final token if the input did not end with whitespace.
    if (!current.isEmpty())
        out << Arg{current, tokenQuoted};

    return out;
}

// Converts tokenized arguments to plain text tokens.
//
// Used by parsers that do not care whether a whole token was quoted, currently
// `/common`. `error` is forwarded from `tokenize`. Returns token text only.
QStringList splitArgs(const QString &input, QString *error)
{
    QStringList values;

    // Preserve tokenizer behavior while discarding quote metadata.
    for (const Arg &arg : tokenize(input, error))
        values << arg.text;
    return values;
}

// Parses `span=` duration values into seconds.
//
// Used by `/search` and `/mentions` option parsing. `value` must be a positive
// integer followed by `d`, `h`, or `m`; `seconds` receives the converted value.
// Returns false for malformed or non-positive spans.
bool parseSpan(const QString &value, qint64 *seconds)
{
    // Need at least one digit and one unit character.
    if (value.size() < 2)
        return false;

    bool ok = false;
    const qint64 count = value.left(value.size() - 1).toLongLong(&ok);

    // Reject non-numeric and zero/negative spans so callers never produce a
    // surprising all-history search.
    if (!ok || count <= 0)
        return false;

    const QChar unit = value.back().toLower();

    // Convert days, hours, and minutes into seconds.
    if (unit == u'd') {
        *seconds = count * 24 * 60 * 60;
        return true;
    }
    if (unit == u'h') {
        *seconds = count * 60 * 60;
        return true;
    }
    if (unit == u'm') {
        *seconds = count * 60;
        return true;
    }

    // Unknown units are rejected rather than guessed.
    return false;
}

} // namespace RichSearch::Internal
