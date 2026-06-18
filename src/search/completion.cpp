// Rich-search command completion and live help.
//
// This module provides argument completion candidates, enumerated option
// choices, and one-line help text for `/search`, `/last`, `/common`,
// `/mentions`, and `/lastmention`. It is used by `MainWindow` tab completion,
// live input guidance, and completion unit tests. It reads no message buffers
// and performs no network or disk activity.

#include "search/richsearch.h"

#include <algorithm>

namespace RichSearch {
namespace {

enum class CommandFamily {
    None,
    Search,
    Mention,
    Common,
};

// Classifies one slash command into the completion/help surface it owns.
//
// Used by vocabulary and live-help helpers. `command` is a lowercase command
// token such as `/search`. Returns `None` for commands outside this module so
// MainWindow can fall back to normal command or nick completion.
CommandFamily commandFamily(const QString &command)
{
    if (command == QStringLiteral("/search") || command == QStringLiteral("/last"))
        return CommandFamily::Search;

    if (command == QStringLiteral("/mentions")
        || command == QStringLiteral("/lastmention")) {
        return CommandFamily::Mention;
    }

    if (command == QStringLiteral("/common"))
        return CommandFamily::Common;

    return CommandFamily::None;
}

// Returns the static argument vocabulary for one search-family command.
//
// Called by `completeArgument`. `command` is the lowercase slash command at the
// start of the input line. The returned tokens are parser-supported selectors,
// options, and boolean keywords only; this helper does not inspect messages or
// send IRC traffic.
QStringList vocabularyForCommand(const QString &command)
{
    static const QStringList searchVocabulary = {
        "text=", "itext=", "regex=", "iregex=", "origin=", "nick=", "nickname=",
        "account=", "type=", "type=message", "type=notice", "type=action",
        "mention=", "span=", "span=2d", "span=3d", "span=2h", "span=5m",
        "context=", "view=",
        "--other", "--textonly", "--notimestamp",
        "AND", "OR", "NOT", "(", ")",
    };
    static const QStringList mentionVocabulary = searchVocabulary + QStringList{
        "scope=local", "scope=global",
    };
    static const QStringList commonVocabulary = {
        "scope=global", "scope=network",
    };

    const CommandFamily family = commandFamily(command);

    // /mentions and /lastmention share /search filters plus mention scope.
    if (family == CommandFamily::Mention)
        return mentionVocabulary;

    // /search and /last share the same parser surface.
    if (family == CommandFamily::Search)
        return searchVocabulary;

    // /common has a separate, intentionally small option grammar.
    if (family == CommandFamily::Common)
        return commonVocabulary;

    return {};
}

// Returns value choices for a selector key that supports enumerated values.
//
// Called by completion helpers after the current argument token has been
// isolated. `token` is the argument under the cursor. The returned values are
// display-only when the token ends at `key=`, and insertable when the user has
// typed part of a value.
QStringList valueChoicesForToken(const QString &token)
{
    if (token.startsWith(QStringLiteral("view="), Qt::CaseInsensitive))
        return {QStringLiteral("inline"), QStringLiteral("pane"), QStringLiteral("tab")};

    return {};
}

// Extracts the argument token under the cursor.
//
// Used by both insertable completion and display-only choice listing. `line` is
// the current input block and `cursorPosition` is the cursor within that block.
// `wordStart` receives the token start when supplied.
QString currentArgumentToken(const QString &line, int cursorPosition, int *wordStart)
{
    const int boundedPos = std::clamp(
        cursorPosition,
        0,
        static_cast<int>(line.size()));
    const QString beforeCursor = line.left(boundedPos);
    const qsizetype start = beforeCursor.lastIndexOf(u' ') + 1;

    if (wordStart)
        *wordStart = static_cast<int>(start);

    return beforeCursor.mid(start);
}

// Extracts the lowercase slash command from an input line.
//
// Called by completion and live-help helpers. `line` is one input block from
// the editor. Returns an empty string when the line is not a slash command.
QString commandFromLine(const QString &line)
{
    if (!line.startsWith(u'/'))
        return {};

    const qsizetype firstSpace = line.indexOf(u' ');
    return firstSpace == -1
        ? line.toLower()
        : line.left(firstSpace).toLower();
}

} // namespace

// Completes arguments for rich-search slash commands.
//
// Called by `MainWindow::handleTabComplete` before nick completion. `line` is
// the current input block, `cursorPosition` is the cursor position within that
// block, and `wordStart` receives the replacement start offset. Returns matching
// completion candidates for the current token. It reads no model state and has
// no UI, disk, or network side effects.
QStringList completeArgument(const QString &line, int cursorPosition,
                             int *wordStart)
{
    const int boundedPos = std::clamp(
        cursorPosition,
        0,
        static_cast<int>(line.size()));
    const QString beforeCursor = line.left(boundedPos);
    const qsizetype firstSpace = beforeCursor.indexOf(u' ');

    // Argument completion applies only after a slash command and at least one
    // separator. Bare `/se<Tab>` remains command completion in MainWindow.
    if (!beforeCursor.startsWith(u'/') || firstSpace == -1)
        return {};

    const QString command = beforeCursor.left(firstSpace).toLower();
    const QStringList vocabulary = vocabularyForCommand(command);

    // Unknown commands are ignored so normal nick completion can continue.
    if (vocabulary.isEmpty())
        return {};

    int start = -1;
    const QString prefix = currentArgumentToken(line, cursorPosition, &start);
    QStringList matches;

    const qsizetype eq = prefix.indexOf(u'=');
    if (eq != -1) {
        const QString key = prefix.left(eq + 1);
        const QString valuePrefix = prefix.mid(eq + 1);
        const QStringList choices = valueChoicesForToken(key);

        // A bare enumerated key such as `view=` should list choices in the UI
        // rather than inserting the first/default value.
        if (!choices.isEmpty() && valuePrefix.isEmpty())
            return {};

        // A partially typed enumerated value remains insertable.
        for (const QString &choice : choices) {
            if (choice.startsWith(valuePrefix, Qt::CaseInsensitive))
                matches << key + choice;
        }
    }

    // Return every supported token that starts with the current token prefix.
    for (const QString &candidate : vocabulary) {
        if (candidate.startsWith(prefix, Qt::CaseInsensitive))
            matches << candidate;
    }

    matches.removeDuplicates();
    matches.sort(Qt::CaseInsensitive);
    *wordStart = start;
    return matches;
}

// Returns display-only choices for an argument under the cursor.
//
// MainWindow calls this to mimic HexChat-style completion listing. `line` and
// `cursorPosition` identify the current input token. The returned strings are
// values, not full `key=value` insertions.
QStringList completionChoices(const QString &line, int cursorPosition)
{
    const int boundedPos = std::clamp(
        cursorPosition,
        0,
        static_cast<int>(line.size()));
    const QString beforeCursor = line.left(boundedPos);
    const qsizetype firstSpace = beforeCursor.indexOf(u' ');

    // Choice listing is owned only by rich-search slash commands.
    if (!beforeCursor.startsWith(u'/') || firstSpace == -1)
        return {};

    if (vocabularyForCommand(beforeCursor.left(firstSpace).toLower()).isEmpty())
        return {};

    const QString token = currentArgumentToken(line, cursorPosition, nullptr);
    const qsizetype eq = token.indexOf(u'=');

    if (eq == -1 || eq + 1 != token.size())
        return {};

    return valueChoicesForToken(token.left(eq + 1));
}

// Returns live argument guidance for rich-search slash commands.
//
// Called by `MainWindow` while the input box changes. `line` is the current
// input text. The returned string is safe to show in the status bar and is empty
// for commands this module does not own. It reads no model state and sends no
// IRC traffic.
QString argumentHelp(const QString &line)
{
    const QString command = commandFromLine(line.trimmed());
    const CommandFamily family = commandFamily(command);

    // /common has one narrow option surface.
    if (family == CommandFamily::Common)
        return QStringLiteral("/common scope=global|network");

    // Mention commands add cross-network scope to the shared search grammar.
    if (family == CommandFamily::Mention) {
        return QStringLiteral(
            "%1 text= origin=/nick= mention= account= type= span=Nd|Nh|Nm "
            "context=N view=inline|pane|tab scope=local|global --other "
            "--textonly --notimestamp AND OR NOT ( )")
            .arg(command);
    }

    // Search-family commands share selectors, options, and boolean grammar.
    if (family == CommandFamily::Search) {
        return QStringLiteral(
            "%1 text= origin=/nick= mention= account= type= span=Nd|Nh|Nm "
            "context=N view=inline|pane|tab --other --textonly --notimestamp AND OR NOT ( )")
            .arg(command);
    }

    return {};
}

} // namespace RichSearch
