// Search option and selector normalization helpers.
//
// This module consumes global command options such as `span=`, `context=`,
// `view=`, `--textonly`, and selector aliases such as `itext=` and `iregex=`.
// It is used by `queryparser.cpp` while building a normalized command. It only
// mutates parser data structures supplied by the caller and performs no IO.

#include "search/internal.h"

namespace RichSearch::Internal {

// Consumes a boolean command flag from the token stream.
//
// Called by `RichSearch::parse` while it separates global options from the
// boolean query expression. `arg` is one token after quote/escape handling, and
// `options` is the command option object being built. Returns true only when
// the token is a recognized unquoted flag. Produces no output and performs no
// network, disk, or UI work.
bool consumeFlag(const Arg &arg, Options *options)
{
    // Quoted strings are search text, so `"--textonly"` must remain searchable
    // instead of mutating command output options.
    if (arg.quoted)
        return false;

    // --textonly strips IRC/ANSI presentation controls from displayed results.
    if (arg.text == QStringLiteral("--textonly")) {
        options->textOnly = true;
        return true;
    }

    // --notimestamp keeps result rows compact by omitting rendered timestamps.
    if (arg.text == QStringLiteral("--notimestamp")) {
        options->noTimestamp = true;
        return true;
    }

    // --other filters out messages authored by the current user.
    if (arg.text == QStringLiteral("--other")) {
        options->otherOnly = true;
        return true;
    }

    return false;
}

// Normalizes selector aliases onto the internal selector vocabulary.
//
// Called by `parsePredicate` before field assignment. `key` is rewritten in
// place, while `predicate` receives any implied flags such as case-insensitive
// or regex matching. Returns no value because unknown selectors are handled by
// `assignSelectorField`.
void normalizeSelector(QString *key, Predicate *predicate)
{
    // itext= is the case-insensitive form of text=.
    if (*key == QStringLiteral("itext")) {
        *key = QStringLiteral("text");
        predicate->caseInsensitive = true;

    // Regex aliases all target message text and set the regex flag.
    } else if (*key == QStringLiteral("regex") || *key == QStringLiteral("regexp")
               || *key == QStringLiteral("re") || *key == QStringLiteral("rx")) {
        *key = QStringLiteral("text");
        predicate->regex = true;

    // iregex= combines regex matching with case-insensitive matching.
    } else if (*key == QStringLiteral("iregex")) {
        *key = QStringLiteral("text");
        predicate->caseInsensitive = true;
        predicate->regex = true;
    }
}

// Maps a normalized selector name to the evaluator field enum.
//
// Called by `parsePredicate` after alias normalization and string-modifier
// parsing. `key` is a lowercase selector name, `predicate` receives the field,
// and `error` receives a user-facing message for unknown selectors. Returns
// false only when parsing must stop.
bool assignSelectorField(const QString &key, Predicate *predicate, QString *error)
{
    // text= matches the message body.
    if (key == QStringLiteral("text")) {
        predicate->field = Field::Text;

    // mention= matches a nick mention in the message body.
    } else if (key == QStringLiteral("mention")) {
        predicate->field = Field::Mention;

    // origin/nick/nickname all refer to the sender identity.
    } else if (key == QStringLiteral("origin") || key == QStringLiteral("nick")
               || key == QStringLiteral("nickname")) {
        predicate->field = Field::Origin;

    // account= matches the account tag when Uplink has one loaded locally.
    } else if (key == QStringLiteral("account")) {
        predicate->field = Field::Account;

    // type= matches Uplink's loaded message kind.
    } else if (key == QStringLiteral("type")) {
        predicate->field = Field::Type;

    // Unknown selectors fail closed instead of becoming broad text searches.
    } else {
        *error = QStringLiteral("unknown selector: %1").arg(key);
        return false;
    }

    return true;
}

// Consumes a key/value command option from the token stream.
//
// Called by `RichSearch::parse` while splitting options from expression terms.
// `arg` is one parsed token, `options` is updated for recognized options, and
// `error` receives validation failures. Returns true when the token belongs to
// the option grammar, even if validation failed. It never performs IO or sends
// IRC traffic.
bool consumeOption(const Arg &arg, Options *options, QString *error)
{
    // A quoted key/value-looking string is literal search text.
    if (arg.quoted)
        return false;

    const qsizetype eq = arg.text.indexOf(u'=');

    // Tokens without '=' cannot be key/value options.
    if (eq == -1)
        return false;

    const QString key = arg.text.left(eq).toLower();
    const QString value = arg.text.mid(eq + 1);

    // context=N includes nearby lines around each match.
    if (key == QStringLiteral("context")) {
        bool ok = false;
        const int lines = value.toInt(&ok);

        // Negative and non-numeric context values fail before evaluation.
        if (!ok || lines < 0) {
            *error = QStringLiteral("context must be a non-negative integer");
            return true;
        }
        options->contextLines = lines;
        return true;
    }

    // span=Nd|Nh|Nm restricts results to a recent loaded time window.
    if (key == QStringLiteral("span")) {
        qint64 seconds = 0;

        // parseSpan validates unit syntax and positive duration.
        if (!parseSpan(value, &seconds)) {
            *error = QStringLiteral("span must be a positive duration like 3d, 2h, or 5m");
            return true;
        }
        options->spanSeconds = seconds;
        return true;
    }

    // view= selects where result rows are rendered.
    if (key == QStringLiteral("view")) {
        const QString lowerValue = value.toLower();

        // Inline output writes into the current buffer.
        if (lowerValue == QStringLiteral("inline"))
            options->view = ResultView::Inline;

        // Pane output opens or reuses a side pane for the result buffer.
        else if (lowerValue == QStringLiteral("pane"))
            options->view = ResultView::Pane;

        // Tab output opens or switches to a synthetic result buffer.
        else if (lowerValue == QStringLiteral("tab"))
            options->view = ResultView::Tab;

        // Unknown view values are reported rather than silently defaulted.
        else
            *error = QStringLiteral("view must be inline, pane, or tab");
        return true;
    }

    return false;
}

} // namespace RichSearch::Internal
