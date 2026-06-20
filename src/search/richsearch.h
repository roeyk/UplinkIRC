// Public RichSearch command API.
//
// This header exposes the local IRC rich-search feature surface used by Uplink
// UI code and tests: `/search`, `/last`, `/common`, `/mentions`,
// `/lastmention`, completions, and live help. Callers are `CommandDispatcher`,
// `MainWindow`, and rich-search unit tests. Implementations are split across
// focused `src/search/*.cpp` modules.

#pragma once

#include "model/channel.h"
#include "model/ids.h"
#include "model/serversession.h"

#include <QDateTime>
#include <QList>
#include <QString>

#include <memory>

namespace RichSearch {

enum class CommonScope {
    Global,
    Network,
};

enum class Field {
    Text,
    Mention,
    Origin,
    Account,
    Type,
};

enum class ResultView {
    Inline,
    Pane,
    Tab,
};

struct Predicate {
    Field field{Field::Text};
    QString value;
    bool caseInsensitive{false};
    bool regex{false};
    bool negated{false};
};

struct Expr;
using ExprPtr = std::shared_ptr<Expr>;

struct Expr {
    enum class Kind {
        Predicate,
        And,
        Or,
        Not,
    };

    Kind kind{Kind::Predicate};
    Predicate predicate;
    QList<ExprPtr> children;
};

struct Options {
    bool textOnly{false};
    bool noTimestamp{false};
    bool otherOnly{false};
    int contextLines{0};
    qint64 spanSeconds{0};
    ResultView view{ResultView::Inline};
};

struct Command {
    QList<Predicate> predicates;
    ExprPtr query;
    Options options;
};

struct Result {
    bool ok{false};
    Command command;
    QString error;
};

struct CommonCommand {
    CommonScope scope{CommonScope::Global};
};

struct CommonResult {
    bool ok{false};
    CommonCommand command;
    QString error;
};

struct MentionResult {
    bool ok{false};
    QStringList lines;
    QString error;
    ResultView view{ResultView::Inline};
};

// Parses the raw argument string for `/search`, `/last`, and `/mentions`.
//
// `input` is everything after the command name. The returned `Result` contains
// either a normalized `Command` with options, predicates, and expression tree,
// or a user-facing syntax error. Parsing is local and has no UI, disk, or
// network side effects.
Result parse(const QString &input);

// Evaluates a parsed search command against one loaded channel buffer.
//
// Called by command UI and tests after `parse` succeeds. `channel` is the
// already-loaded in-memory buffer, `selfNick` supports `--other`, and `now`
// anchors relative `span=` filters. Returns formatted local display lines; it
// never requests history or sends IRC traffic.
QStringList renderMatches(const Channel &channel, const Command &command,
                          const QString &selfNick,
                          const QDateTime &now = QDateTime::currentDateTime());

// Parses the raw argument string for `/common`.
//
// `input` is everything after `/common`. The result contains either a
// `CommonCommand` with the selected scope or a user-facing error. The function
// only parses text and performs no model inspection or network activity.
CommonResult parseCommon(const QString &input);

// Renders `/common` output with the documented default scope.
//
// Convenience overload for callers that want `scope=global`. `sessions` is the
// in-memory server/channel model, and the current host/channel identify the
// active channel whose visible users are being inspected.
QStringList renderCommon(const QList<ServerSession> &sessions,
                         const ServerId &currentHost,
                         const BufferId &currentChannel);

// Renders `/common` output for an explicit scope.
//
// Called by command UI after `parseCommon`. It compares current-channel users
// against already-known local membership in `sessions`. Returns local display
// lines and never sends WHO, WHOIS, NAMES, or other IRC commands.
QStringList renderCommon(const QList<ServerSession> &sessions,
                         const ServerId &currentHost,
                         const BufferId &currentChannel,
                         CommonScope scope);

// Renders `/mentions` output as plain lines for simple callers.
//
// Compatibility wrapper around `renderMentionResult`. `filters` accepts the
// shared `/search` filter syntax plus mention scope handling. Invalid filters
// produce an empty list here; command UI should use `renderMentionResult` when
// it needs to display the error.
QStringList renderMentions(const QList<ServerSession> &sessions,
                           const ServerId &currentHost,
                           const QString &selfNick,
                           const QString &filters = {},
                           const QDateTime &now = QDateTime::currentDateTime());

// Renders `/mentions` and `/lastmention` with syntax/error metadata.
//
// Called by command UI. `sessions` is loaded local state, `currentHost` selects
// the active network for local scope, `selfNick` is the nick being mentioned,
// `filters` is user-supplied search syntax, and `now` anchors the default span.
// Returns formatted lines, selected output view, or a parse error. No network
// traffic or disk IO is performed.
MentionResult renderMentionResult(const QList<ServerSession> &sessions,
                                  const ServerId &currentHost,
                                  const QString &selfNick,
                                  const QString &filters = {},
                                  const QDateTime &now = QDateTime::currentDateTime());

// Completes one rich-search-family argument token.
//
// Called by `MainWindow` tab completion. `line` is the input line and
// `cursorPosition` is the cursor offset inside it. `wordStart` receives the
// replacement start offset. Returns candidate insertions only; it reads no
// channel state and has no side effects.
QStringList completeArgument(const QString &line, int cursorPosition,
                             int *wordStart);

// Returns enumerated choices for a completed argument key.
//
// Called by `MainWindow` when the user presses Tab on tokens such as `view=`.
// Returns display choices like `inline`, `pane`, and `tab`, or an empty list
// when the current token has no fixed value vocabulary.
QStringList completionChoices(const QString &line, int cursorPosition);

// Returns one-line command help for live input guidance.
//
// Called by `MainWindow` while the input text changes. It recognizes the
// rich-search command family and returns selector/option help text, or an empty
// string for unrelated input.
QString argumentHelp(const QString &line);

} // namespace RichSearch
