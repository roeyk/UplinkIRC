// /common local-membership renderer.
//
// This module parses and renders the `/common` command. It compares users in
// the active channel against Uplink's already-loaded in-memory channel
// membership and formats shared-channel rows. It is used by
// `CommandDispatcher` and `/common` unit tests. It never sends WHO, WHOIS,
// NAMES, or other IRC traffic.

#include "search/internal.h"

#include <QMap>
#include <QSet>

#include <algorithm>

namespace RichSearch {
namespace {

struct CommonRows {
    QMap<QString, QStringList> sharedChannels;
    QSet<QString> nicksWithOtherChannels;
};

// Finds a server session by host without creating or refreshing any state.
//
// Used by `/common` rendering. `sessions` is Uplink's current in-memory session
// list and `host` is the active server. Returns a pointer into `sessions`, or
// nullptr if the server is not present. Produces no output and sends no IRC
// traffic.
const ServerSession *findSession(const QList<ServerSession> &sessions,
                                 const ServerId &host)
{
    // Search the already-known session list. Missing state means `/common`
    // cannot safely infer anything locally.
    const auto it = std::find_if(
        sessions.cbegin(),
        sessions.cend(),
        [&](const ServerSession &session) {
            return session.host == host.str();
        });
    return it == sessions.cend() ? nullptr : &*it;
}

// Chooses the display label for cross-network `/common` rows.
//
// Used only by `renderCommon`. `session` is one known server session. Returns
// the configured display name when available, otherwise the server host.
QString networkLabel(const ServerSession &session)
{
    return session.name.isEmpty() ? session.host : session.name;
}

// Builds the set of visible users in the current channel.
//
// Called by `renderCommon` after the active channel has been found. `channel`
// is the loaded local channel, and `selfNick` is excluded because `/common`
// answers who else overlaps. Returns a lowercase-nick to display-nick map and
// performs no UI or network work.
QMap<QString, QString> currentChannelNicks(const Channel &channel,
                                           const QString &selfNick)
{
    QMap<QString, QString> nicks;
    const QString selfLower = selfNick.toLower();

    // Use the nick list already held by Uplink; do not request fresh NAMES.
    for (const NickEntry &nick : channel.nicks) {
        if (nick.lowerNick == selfLower)
            continue;
        nicks.insert(nick.lowerNick, nick.nick);
    }

    return nicks;
}

// Appends shared-channel matches from one candidate channel.
//
// Called by `collectCommonRows` while scanning loaded membership. `session` and
// `channel` are read-only local model state, `currentNicks` is the active
// channel user set, and `scope` controls row labels. Matches are appended to
// `rows`; no membership refresh or IRC command is issued.
void collectChannelRows(const ServerSession &session,
                        const Channel &channel,
                        const QMap<QString, QString> &currentNicks,
                        CommonScope scope,
                        CommonRows *rows)
{
    // For every nick in the active channel, check whether that nick is also
    // present in the candidate channel's existing nick index.
    for (auto nickIt = currentNicks.cbegin(); nickIt != currentNicks.cend(); ++nickIt) {
        // If the candidate channel does not know this nick locally, move on
        // without network refresh.
        if (!channel.nickIndex.contains(nickIt.key()))
            continue;

        // Global rows include the network label so equal nicknames on
        // different networks remain distinguishable.
        const QString rowKey = scope == CommonScope::Global
            ? networkLabel(session) + QStringLiteral("/") + nickIt.value()
            : nickIt.value();
        rows->sharedChannels[rowKey] << channel.name;
        rows->nicksWithOtherChannels.insert(nickIt.key());
    }
}

// Collects all shared-channel rows from loaded session/channel state.
//
// Called by `renderCommon` after it has built the active channel nick set.
// `sessions` is the in-memory model, `currentHost`/`currentLower` identify the
// active channel to exclude, and `scope` selects network-only versus global
// scanning. Returns collected rows and never sends WHO, WHOIS, or NAMES.
CommonRows collectCommonRows(const QList<ServerSession> &sessions,
                             const ServerId &currentHost,
                             const QString &currentLower,
                             const QMap<QString, QString> &currentNicks,
                             CommonScope scope)
{
    CommonRows rows;

    // Scan loaded membership across either all sessions or only the current
    // session, depending on `scope`.
    for (const ServerSession &session : sessions) {
        // In network scope, skip every server except the active one.
        if (scope == CommonScope::Network && session.host != currentHost.str())
            continue;

        // Inspect each known channel in the selected session. This uses
        // Uplink's existing nickIndex map and does not request fresh NAMES.
        for (auto it = session.channels.cbegin(); it != session.channels.cend(); ++it) {
            // Do not list the active channel as a shared channel; only other
            // overlaps are useful to the user.
            if (session.host == currentHost.str() && it.key() == currentLower)
                continue;

            collectChannelRows(
                session,
                it.value(),
                currentNicks,
                scope,
                &rows);
        }
    }

    return rows;
}

// Formats collected `/common` rows for display.
//
// Called by `renderCommon` after membership scanning. `rows` contains nick to
// shared-channel mappings, `currentNicks` is the full active-channel user set,
// and `currentChannel` labels the exclusive footer. Returns deterministic local
// display lines and performs no model or network work.
QStringList formatCommonRows(const CommonRows &rows,
                             const QMap<QString, QString> &currentNicks,
                             const BufferId &currentChannel)
{
    QStringList lines;

    // Normalize and format each row after collection so duplicates from local
    // state churn do not leak into the output.
    for (auto it = rows.sharedChannels.cbegin(); it != rows.sharedChannels.cend(); ++it) {
        QStringList channels = it.value();
        channels.removeDuplicates();
        channels.sort(Qt::CaseInsensitive);

        // Skip empty rows defensively; normally a row exists only after at
        // least one shared channel was recorded.
        if (!channels.isEmpty())
            lines << it.key() + QStringLiteral(": ") + channels.join(QStringLiteral(", "));
    }

    QStringList onlyHere;

    // Users who never appeared in another known channel are summarized on one
    // exclusive-membership footer instead of producing one row each.
    for (auto it = currentNicks.cbegin(); it != currentNicks.cend(); ++it) {
        if (!rows.nicksWithOtherChannels.contains(it.key()))
            onlyHere << it.value();
    }

    onlyHere.sort(Qt::CaseInsensitive);

    // Add the aggregate after shared-channel rows so it reads as a footer.
    if (!onlyHere.isEmpty()) {
        lines << QStringLiteral("exclusive to %1: %2").arg(
            currentChannel.str(),
            onlyHere.join(QStringLiteral(", ")));
    }

    return lines;
}

} // namespace

// Parses `/common` options.
//
// Called by `CommandDispatcher` before rendering common-channel results.
// `input` is the raw text after `/common`. Returns a CommonResult containing
// either a parsed scope or a user-facing syntax error. This parser has no side
// effects and never sends IRC traffic.
CommonResult parseCommon(const QString &input)
{
    CommonResult result;
    QString error;
    const QStringList args = Internal::splitArgs(input, &error);

    // If quote parsing failed, return the tokenizer error directly to the
    // command dispatcher for local display.
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }

    // Process each option token. The first slice supports only `scope=`, so any
    // other token is rejected instead of silently ignored.
    for (const QString &arg : args) {
        const qsizetype eq = arg.indexOf(u'=');

        // Every `/common` option is key=value. A bare token is therefore a
        // syntax error rather than a selector.
        if (eq == -1) {
            result.error = QStringLiteral("unknown /common argument: %1").arg(arg);
            return result;
        }

        const QString key = arg.left(eq).toLower();
        const QString value = arg.mid(eq + 1).toLower();

        // Keep the grammar intentionally small. New options should be added
        // explicitly so review can audit their network/security behavior.
        if (key != QStringLiteral("scope")) {
            result.error = QStringLiteral("unknown /common argument: %1").arg(key);
            return result;
        }

        // `scope=global` is the default and shows matches across all currently
        // connected networks.
        if (value == QStringLiteral("global")) {
            result.command.scope = CommonScope::Global;

        // `scope=network` limits results to the active server session.
        } else if (value == QStringLiteral("network")) {
            result.command.scope = CommonScope::Network;

        // Reject unknown scope values so typos do not silently broaden or
        // narrow the result set.
        } else {
            result.error = QStringLiteral("scope must be global or network");
            return result;
        }
    }

    result.ok = true;
    return result;
}

// Convenience renderer for default `/common` behavior.
//
// Called by tests and available to callers that want the documented default
// scope. The parameters identify the current in-memory app state and active
// channel. Returns local display rows and performs no network work.
QStringList renderCommon(const QList<ServerSession> &sessions,
                         const ServerId &currentHost,
                         const BufferId &currentChannel)
{
    return renderCommon(
        sessions,
        currentHost,
        currentChannel,
        CommonScope::Global);
}

// Renders common-channel rows from already-known channel membership.
//
// Called by `CommandDispatcher` after `parseCommon`. `sessions` is Uplink's
// in-memory server/channel model, `currentHost` and `currentChannel` identify
// the channel being inspected, and `scope` controls current-network versus
// all-network output. Returns formatted local rows. It does not issue WHO,
// WHOIS, NAMES, or any other IRC command.
QStringList renderCommon(const QList<ServerSession> &sessions,
                         const ServerId &currentHost,
                         const BufferId &currentChannel,
                         CommonScope scope)
{
    const ServerSession *currentSession = findSession(sessions, currentHost);

    // If the active session is unknown, there is no local membership baseline
    // to compare against.
    if (!currentSession)
        return {};

    const auto currentIt = currentSession->channels.constFind(
        currentChannel.str().toLower());

    // `/common` is meaningful only from a known channel buffer. Missing channel
    // state returns no rows rather than refreshing from the network.
    if (currentIt == currentSession->channels.constEnd())
        return {};

    const Channel &current = currentIt.value();
    const QString currentLower = currentChannel.str().toLower();
    const QMap<QString, QString> currentNicks = currentChannelNicks(
        current,
        currentSession->nick);

    return formatCommonRows(
        collectCommonRows(sessions, currentHost, currentLower, currentNicks, scope),
        currentNicks,
        currentChannel);
}

} // namespace RichSearch
