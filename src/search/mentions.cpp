// /mentions and /lastmention renderer.
//
// This module implements recent loaded mention listing. It reuses the shared
// `/search` parser/evaluator, injects mention-specific defaults, and renders
// scoped local/global output rows. It is used by `CommandDispatcher`,
// compatibility wrappers in `richsearch.h`, and mention unit tests. It scans
// only loaded in-memory buffers and never sends IRC traffic.

#include "search/internal.h"

#include <algorithm>

namespace RichSearch {
namespace {

constexpr qint64 kDefaultMentionSpanSeconds = 2 * 24 * 60 * 60;

struct MentionFilters {
    Result parsed;
    CommonScope scope{CommonScope::Network};
};

// Finds the currently selected server session for `/mentions`.
//
// Called by `renderMentions`. `sessions` is Uplink's in-memory session list and
// `host` is the active server identifier from `CommandDispatcher`. Returns a
// pointer into `sessions`, or nullptr when the server is not known. Produces no
// UI output and never sends IRC traffic.
const ServerSession *findSession(const QList<ServerSession> &sessions,
                                 const ServerId &host)
{
    // Search the existing session list instead of creating or refreshing
    // server state. Missing state means there are no local mentions to show.
    const auto it = std::find_if(
        sessions.cbegin(),
        sessions.cend(),
        [&](const ServerSession &session) {
            return session.host == host.str();
        });
    return it == sessions.cend() ? nullptr : &*it;
}

// Formats one `/mentions` result line for local status output.
//
// Called by `renderMentions` after a message passes all local filters.
// `channelName` identifies the loaded channel where the mention was found and
// `message` supplies timestamp, sender, and text. Returns display text only.
QString formatMentionLine(
    const QString &networkName,
    const QString &channelName,
    const Message &message,
    const Command &command,
    CommonScope scope)
{
    // Global output needs the network/channel prefix so cross-network mentions
    // remain distinguishable. Local output keeps the older channel-only shape.
    if (scope == CommonScope::Global) {
        return QStringLiteral("%1/%2: %3").arg(
            networkName,
            channelName,
            Internal::formatMessage(
                message,
                command.options,
                command.predicates));
    }

    return QStringLiteral("%1  %2").arg(
        channelName,
        Internal::formatMessage(
            message,
            command.options,
            command.predicates));
}

// Quotes a selector value for reuse in the shared /search parser.
//
// Called by `parseMentionFilters` when injecting the current nick as
// `mention=<nick>`. `value` is user/account state from the current session.
// Returns a quoted selector value with backslash and quote characters escaped.
QString quotedSelectorValue(const QString &value)
{
    QString escaped;
    escaped.reserve(value.size());

    // Preserve literal backslashes and quotes so injected nicks cannot change
    // the generated query syntax.
    for (const QChar ch : value) {
        if (ch == u'\\' || ch == u'"')
            escaped.append(u'\\');
        escaped.append(ch);
    }

    return QStringLiteral("\"%1\"").arg(escaped);
}

// Recreates one token for the shared /search parser after /mentions has removed
// scope=. Whole-token quotes must be restored so `"origin=alice"` remains a
// literal text search instead of becoming a selector.
QString renderArgForQuery(const Internal::Arg &arg)
{
    if (arg.quoted)
        return quotedSelectorValue(arg.text);

    return arg.text;
}

// Builds the /mentions query by combining user filters with required defaults.
//
// Called by `renderMentions`. `filters` is raw text after `/mentions`, and
// `selfNick` is the current network nick. Returns the same Result shape as
// `/search`, with --other, text-like message types, and mention=<selfNick>
// injected so /mentions reuses the shared parser and evaluator.
MentionFilters parseMentionFilters(const QString &filters, const QString &selfNick)
{
    MentionFilters mentionFilters;
    QStringList queryParts;
    QString error;
    const QList<Internal::Arg> args = Internal::tokenize(filters, &error);

    // Tokenizer errors must be reported before adding default mention clauses.
    if (!error.isEmpty()) {
        mentionFilters.parsed.error = error;
        return mentionFilters;
    }

    // Separate /mentions-only scope= from ordinary /search filters. Search
    // selectors keep their original spelling and quote state.
    for (const Internal::Arg &arg : args) {
        const qsizetype eq = arg.text.indexOf(u'=');
        if (!arg.quoted && eq != -1
            && arg.text.left(eq).compare(QStringLiteral("scope"), Qt::CaseInsensitive) == 0) {
            const QString value = arg.text.mid(eq + 1).toLower();

            // Local is the default and means current network only.
            if (value == QStringLiteral("local") || value == QStringLiteral("network")) {
                mentionFilters.scope = CommonScope::Network;

            // Global searches all loaded networks and prefixes output rows.
            } else if (value == QStringLiteral("global")) {
                mentionFilters.scope = CommonScope::Global;

            // Unknown scope values fail closed instead of silently broadening
            // or narrowing the result set.
            } else {
                mentionFilters.parsed.error = QStringLiteral("scope must be local or global");
                return mentionFilters;
            }
            continue;
        }

        queryParts << renderArgForQuery(arg);
    }

    QString query = queryParts.join(u' ');

    // Keep caller-supplied filters first so they can provide options such as
    // `span=1d` and `--notimestamp`.
    if (!query.isEmpty())
        query.append(u' ');

    query.append(QStringLiteral("--other (type=message OR type=notice OR type=action) mention=%1")
        .arg(quotedSelectorValue(selfNick)));
    mentionFilters.parsed = parse(query);
    return mentionFilters;
}

// Returns the display name for a loaded server session.
//
// Called by global mention rendering. `session` is read-only local state. The
// returned string is stable enough for output and never triggers network work.
QString networkLabel(const ServerSession &session)
{
    return session.name.isEmpty() ? session.host : session.name;
}

// Renders matching mention rows from one loaded channel.
//
// Called by `appendSessionMentions` after scope filtering has selected a
// session. `session` provides the network label, `channel` is one local buffer,
// `command` is the parsed shared-search command with mention defaults applied,
// `selfNick` supports `--other`, `oldest` enforces the span window, and `scope`
// controls row prefixes. Matching rows are appended to `lines`. This helper
// reads only the channel's loaded messages and never sends IRC traffic.
void appendChannelMentions(const ServerSession &session,
                           const Channel &channel,
                           const Command &command,
                           const QString &selfNick,
                           const QDateTime &oldest,
                           CommonScope scope,
                           QStringList *lines)
{
    // Skip pseudo-buffers such as the server/status buffer because `/mentions`
    // answers "which channel mentioned me?"
    if (!(channel.name.startsWith(u'#') || channel.name.startsWith(u'&')))
        return;

    const Internal::NickIdentityIndex identities(channel);

    // Inspect each loaded message in the channel's capped in-memory buffer.
    for (const Message &message : channel.messages) {
        // Apply the same parser/evaluator path as `/search`, with `/mentions`
        // defaults injected by parseMentionFilters.
        if (!Internal::messageMatches(message, command, selfNick, oldest, identities))
            continue;

        // The message passed all local filters, so include a scoped output
        // prefix and the formatted message body.
        *lines << formatMentionLine(
            networkLabel(session),
            channel.name,
            message,
            command,
            scope);
    }
}

// Renders matching mention rows from one loaded server session.
//
// Called by `renderMentionResult` after it decides which sessions are in scope.
// `session` is read-only model state, `command` is the normalized search
// command, `selfNick` and `oldest` are evaluator inputs, and `scope` controls
// row formatting. Matching rows are appended to `lines`; no server state is
// refreshed and no IRC commands are sent.
void appendSessionMentions(const ServerSession &session,
                           const Command &command,
                           const QString &selfNick,
                           const QDateTime &oldest,
                           CommonScope scope,
                           QStringList *lines)
{
    // Inspect each loaded channel buffer in the selected session.
    for (auto it = session.channels.cbegin(); it != session.channels.cend(); ++it) {
        appendChannelMentions(
            session,
            it.value(),
            command,
            selfNick,
            oldest,
            scope,
            lines);
    }
}

} // namespace

// Lists recent loaded mentions of the user's current nick on one network.
//
// Called by `CommandDispatcher` for `/mentions` and `/lastmention`. `sessions`
// is Uplink's in-memory server/channel state, `currentHost` selects the active
// network, `selfNick` is the current local nick, and `now` sets the default
// two-day span boundary. Returns formatted local result lines. This function
// intentionally reads only loaded channel buffers and never sends IRC traffic,
// opens files, or requests history.
MentionResult renderMentionResult(const QList<ServerSession> &sessions,
                                  const ServerId &currentHost,
                                  const QString &selfNick,
                                  const QString &filters,
                                  const QDateTime &now)
{
    MentionResult result;

    // Without a known session or current nick there is no safe local identity
    // to search for, so return no results instead of guessing.
    const ServerSession *currentSession = findSession(sessions, currentHost);
    if (!currentSession || selfNick.isEmpty()) {
        result.ok = true;
        return result;
    }

    const MentionFilters mentionFilters = parseMentionFilters(filters, selfNick);
    if (!mentionFilters.parsed.ok) {
        result.error = mentionFilters.parsed.error;
        return result;
    }

    Command command = mentionFilters.parsed.command;
    result.view = command.options.view;
    if (command.options.spanSeconds <= 0)
        command.options.spanSeconds = kDefaultMentionSpanSeconds;
    command.options.otherOnly = true;
    const QDateTime oldest = now.addSecs(-command.options.spanSeconds);

    // Walk either the active network or all loaded networks, depending on
    // scope=. This reads only local buffers and never sends WHO/WHOIS/NAMES.
    for (const ServerSession &session : sessions) {
        if (mentionFilters.scope == CommonScope::Network
            && session.host != currentSession->host) {
            continue;
        }

        appendSessionMentions(
            session,
            command,
            selfNick,
            oldest,
            mentionFilters.scope,
            &result.lines);
    }

    // Keep output deterministic for tests and readable in the client.
    result.lines.sort(Qt::CaseInsensitive);
    result.ok = true;
    return result;
}

// Compatibility wrapper for callers that only need formatted mention lines.
//
// Used by existing tests and any simple caller that does not need syntax-error
// details. Invalid filters return an empty list here; command UI should prefer
// `renderMentionResult` so it can display the parse error explicitly.
QStringList renderMentions(const QList<ServerSession> &sessions,
                           const ServerId &currentHost,
                           const QString &selfNick,
                           const QString &filters,
                           const QDateTime &now)
{
    return renderMentionResult(
        sessions,
        currentHost,
        selfNick,
        filters,
        now).lines;
}

} // namespace RichSearch
