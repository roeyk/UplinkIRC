// Internal RichSearch interfaces.
//
// This header declares helpers shared only among the rich-search implementation
// modules: tokenization, option parsing, selector normalization, result
// formatting, identity tracking, and message matching. It is included by
// `src/search/*.cpp` files and is not intended as a UI or model-facing API.

#pragma once

#include "search/richsearch.h"

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

namespace RichSearch::Internal {

// One shell-like command argument after quote and escape processing. `quoted`
// means the whole token started as a quoted string, which preserves the
// semantic difference between `origin=alice` and `"origin=alice"`.
struct Arg {
    QString text;
    bool quoted{false};
};

// Used by /search and /common parsing. `input` is the raw command argument
// string after the slash command itself. `error` receives a human-readable
// syntax error. Returns parsed tokens and has no network or UI side effects.
QList<Arg> tokenize(const QString &input, QString *error);

// Used by /common parsing. This is a convenience wrapper over `tokenize` for
// callers that do not need whole-token quote metadata.
QStringList splitArgs(const QString &input, QString *error);

// Used by /search option parsing. Accepts positive duration strings such as
// `3d`, `2h`, and `5m`. Returns true and writes seconds on success.
bool parseSpan(const QString &value, qint64 *seconds);

// Used by /search option parsing. Consumes whole-token flags such as
// `--textonly` and updates `options` when the token belongs to command options.
// Returns false when the caller should treat the token as a query term.
bool consumeFlag(const Arg &arg, Options *options);

// Used by /search option parsing. Consumes key/value options such as `span=2h`,
// `context=2`, and `view=pane`. Returns true when the token was recognized as
// an option; `error` receives validation failures.
bool consumeOption(const Arg &arg, Options *options, QString *error);

// Used by /search selector parsing. Normalizes compatibility aliases such as
// `itext=`, `regex=`, and `iregex=` onto the internal `text` selector while
// applying any implied predicate flags.
void normalizeSelector(QString *key, Predicate *predicate);

// Used by /search selector parsing. Maps a normalized selector key to the
// internal field enum. Returns false and writes `error` for unknown selectors.
bool assignSelectorField(const QString &key, Predicate *predicate, QString *error);

// Used by result formatting when `--textonly` is present. Removes IRC and ANSI
// presentation controls from `text` without mutating the stored message.
QString stripControlCodes(const QString &text);

// Used by result formatting to mark positive text/mention matches. Returns
// `body` with IRC color controls around matched ranges so the renderer can
// highlight them in every output destination.
QString highlightedBody(const QString &body, const QList<Predicate> &predicates);

// Used by result rendering. Applies output options such as timestamp omission,
// control-code stripping, and optional body-match highlighting. `predicates`
// is the parsed command predicate list; callers pass it when result text should
// visually mark positive message-body matches.
QString formatMessage(const Message &message, const Options &options,
                      const QList<Predicate> &predicates = {});

// In-memory identity index for origin matching.
//
// Built once per channel scan by `/search`, `/last`, and `/mentions`. `channel`
// is the loaded local channel model. The index links visible nick-list account
// tags, message account tags, and loaded nick-change messages so `origin=`
// can follow locally observed nick changes without WHO/WHOIS/NAMES traffic.
// `matchesOrigin` returns true when the message sender matches the requested
// nick, a known alias, or a locally observed shared account.
class NickIdentityIndex {
public:
    explicit NickIdentityIndex(const Channel &channel);

    bool matchesOrigin(const QString &queryNick, const Message &message) const;

private:
    void connectNicks(const QString &left, const QString &right);
    void addAccount(const QString &nick, const QString &account);
    QSet<QString> aliasesFor(const QString &lowerNick) const;

    QHash<QString, QSet<QString>> m_nickGraph;
    QHash<QString, QSet<QString>> m_accountsByNick;
};

// Used by result selection. Applies global filters, then evaluates the parsed
// expression tree. It has no side effects and never sends IRC traffic.
bool messageMatches(const Message &message, const Command &command,
                    const QString &selfNick, const QDateTime &oldest,
                    const NickIdentityIndex &identities);

} // namespace RichSearch::Internal
