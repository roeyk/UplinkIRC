// Search-family result-line formatter.
//
// This module formats one loaded message into the text row shown by `/search`,
// `/last`, and `/mentions`. It applies timestamp, origin, text-only, and
// highlighting output options by calling helpers from `displayhighlight.cpp`.
// It is used by search and mention renderers and produces display text only.

#include "search/internal.h"

namespace RichSearch::Internal {

// Formats one search-family result line.
//
// Used by `/search`, `/last`, and `/mentions` renderers. `message` is one
// loaded Uplink message and `options` controls timestamp and text-only output.
// Returns a local display line and performs no IO or network activity.
QString formatMessage(const Message &message, const Options &options,
                      const QList<Predicate> &predicates)
{
    // Server/status-like messages do not carry a sender nick. Use a stable
    // pseudo-origin so all rows remain attributable.
    const QString origin = message.nick.isEmpty()
        ? QStringLiteral("status")
        : message.nick;

    // `--textonly` is an output transform only. It does not change matching
    // semantics or mutate the underlying message.
    QString body = options.textOnly
        ? stripControlCodes(message.text)
        : message.text;

    // `--textonly` promises no generated control codes in output, so highlight
    // controls are inserted only for normal rich output.
    if (!options.textOnly)
        body = highlightedBody(body, predicates);

    // `--notimestamp` preserves the origin and body while omitting time.
    if (options.noTimestamp)
        return QStringLiteral("<%1> %2").arg(origin, body);

    // Timestamps are intentionally second-precision for scan-friendly output.
    return QStringLiteral("[%1] <%2> %3").arg(
        message.timestamp.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
        origin,
        body);
}

} // namespace RichSearch::Internal
