// Search expression evaluator.
//
// This module evaluates parsed `/search` expression trees against loaded
// messages, including text, mention, origin, account, type, span, context, and
// `--other` behavior. It is used by `/search`, `/last`, and `/mentions`
// renderers. It reads immutable in-memory model state only and performs no UI,
// disk, or network side effects.

#include "search/internal.h"

#include <QRegularExpression>

#include <algorithm>

namespace RichSearch {
namespace {

// Converts a user-facing `type=` selector into Uplink's MessageType enum.
//
// Called by `predicateMatches` for `type=` predicates. `value` is the parsed
// selector value and `ok` reports whether it was recognized. Returns a fallback
// enum value when unknown, but callers must check `ok`; this keeps invalid
// values from accidentally matching a default type. Produces no side effects.
MessageType typeFromName(const QString &value, bool *ok)
{
    const QString v = value.toLower();

    struct Alias {
        QStringView name;
        MessageType type;
    };

    static constexpr Alias aliases[] = {
        {u"privmsg", MessageType::Privmsg},
        {u"message", MessageType::Privmsg},
        {u"notice", MessageType::Notice},
        {u"action", MessageType::Action},
        {u"me", MessageType::Action},
        {u"server", MessageType::Server},
        {u"status", MessageType::Server},
        {u"reply", MessageType::Reply},
        {u"join", MessageType::Join},
        {u"part", MessageType::Part},
        {u"quit", MessageType::Quit},
        {u"nick", MessageType::Nick},
        {u"topic", MessageType::Topic},
    };

    // Look up the lowercased selector value in the static alias table so adding
    // new message-type aliases is data-only.
    const auto it = std::find_if(
        std::begin(aliases),
        std::end(aliases),
        [&](const Alias &alias) {
            return v == alias.name;
        });

    if (it != std::end(aliases)) {
        *ok = true;
        return it->type;
    }

    // Unknown values fail closed in predicateMatches through the `ok` flag.
    *ok = false;
    return MessageType::Privmsg;
}

// Tests a text-like field against a literal or regex predicate.
//
// Called by `predicateMatches` and `mentionMatches`. `haystack` is the loaded
// field value and `predicate` supplies the user selector, regex flag, and case
// mode. Returns whether the field matches. Inputs are already parsed by the
// command parser; this helper performs no model, UI, disk, or network work.
bool textMatches(const QString &haystack, const Predicate &predicate)
{
    // Regex searches compile per predicate check so syntax validity and match
    // behavior stay tied to the exact user-provided expression.
    if (predicate.regex) {
        QRegularExpression::PatternOptions options;

        // Case-insensitive regexes use Qt's regex option rather than mutating
        // the user's pattern.
        if (predicate.caseInsensitive)
            options |= QRegularExpression::CaseInsensitiveOption;

        const QRegularExpression rx(predicate.value, options);
        return rx.isValid() && rx.match(haystack).hasMatch();
    }

    // Literal searches use QString containment with the requested case mode.
    return haystack.contains(
        predicate.value,
        predicate.caseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive);
}

// Matches a nick mention in message text.
//
// Called by `predicateMatches` for `mention=` selectors. `haystack` is one
// loaded message body and `predicate` supplies the nick or regex. Returns true
// when the mention is present. Literal mention searches require nick
// boundaries; regex mention searches intentionally use generic regex matching.
bool mentionMatches(const QString &haystack, const Predicate &predicate)
{
    // Regex mentions let advanced callers define their own boundaries.
    if (predicate.regex)
        return textMatches(haystack, predicate);

    const QString nickChars = QStringLiteral("A-Za-z0-9_\\-\\[\\]\\\\`^{}|");
    const QRegularExpression rx(
        QStringLiteral("(^|[^%1])(%2)($|[^%1])")
            .arg(nickChars, QRegularExpression::escape(predicate.value)),
        predicate.caseInsensitive
            ? QRegularExpression::CaseInsensitiveOption
            : QRegularExpression::NoPatternOption);
    return rx.match(haystack).hasMatch();
}

// Evaluates one leaf predicate against one loaded message.
//
// Called by `exprMatches` for predicate nodes. `message` is a loaded local
// message, `predicate` is one normalized selector, and `identities` supplies
// local nick/account alias matching for origin selectors. Returns the predicate
// result after applying modifier-level negation. Produces no side effects.
bool predicateMatches(const Message &message, const Predicate &predicate,
                      const Internal::NickIdentityIndex &identities)
{
    bool matched = false;

    // Dispatch by normalized field so parser aliases do not leak into
    // evaluation logic.
    if (predicate.field == Field::Text) {
        matched = textMatches(message.text, predicate);
    } else if (predicate.field == Field::Mention) {
        matched = mentionMatches(message.text, predicate);
    } else if (predicate.field == Field::Origin) {
        matched = predicate.regex
            ? textMatches(message.nick, predicate)
            : identities.matchesOrigin(predicate.value, message);
    } else if (predicate.field == Field::Account) {
        matched = textMatches(message.account, predicate);
    } else if (predicate.field == Field::Type) {
        bool ok = false;
        matched = message.type == typeFromName(predicate.value, &ok) && ok;
    }

    // String modifier `n` negates only this predicate, distinct from boolean
    // keyword NOT which negates a whole subexpression.
    return predicate.negated ? !matched : matched;
}

// Evaluates a parsed boolean expression tree against one message.
//
// Called by `messageMatches` after global filters such as `span=` and `--other`
// have passed. `message` is read-only local state, `expr` is the parser output,
// and `identities` supports origin matching. Returns true when the expression
// matches the message and performs no IO or network activity.
bool exprMatches(const Message &message, const ExprPtr &expr,
                 const Internal::NickIdentityIndex &identities)
{
    // A missing expression is treated as match-all for defensive internal use;
    // normal user parsing rejects empty queries before evaluation.
    if (!expr)
        return true;

    // Leaf nodes defer to field-specific predicate matching.
    if (expr->kind == Expr::Kind::Predicate)
        return predicateMatches(message, expr->predicate, identities);

    // NOT with no child is treated as true; parser-generated NOT nodes always
    // have one child, so this is defensive against malformed internal callers.
    if (expr->kind == Expr::Kind::Not)
        return expr->children.isEmpty()
            || !exprMatches(message, expr->children.first(), identities);

    // AND short-circuits through std::all_of over child expressions.
    if (expr->kind == Expr::Kind::And) {
        return std::all_of(
            expr->children.cbegin(),
            expr->children.cend(),
            [&](const ExprPtr &child) {
                return exprMatches(message, child, identities);
            });
    }

    // OR short-circuits through std::any_of over child expressions.
    if (expr->kind == Expr::Kind::Or) {
        return std::any_of(
            expr->children.cbegin(),
            expr->children.cend(),
            [&](const ExprPtr &child) {
                return exprMatches(message, child, identities);
            });
    }

    return false;
}

} // namespace

namespace Internal {

NickIdentityIndex::NickIdentityIndex(const Channel &channel)
{
    // Current nick-list account tags link visible nicks to accounts.
    for (const NickEntry &nick : channel.nicks) {
        if (!nick.account.isEmpty())
            addAccount(nick.nick, nick.account);
    }

    // Loaded messages provide historical account tags and nick-change edges,
    // allowing origin=oldnick to find newnick messages when scrollback contains
    // the rename event.
    for (const Message &message : channel.messages) {
        if (!message.account.isEmpty())
            addAccount(message.nick, message.account);

        if (message.type == MessageType::Nick && !message.nick.isEmpty()
            && !message.replyTo.isEmpty()) {
            connectNicks(message.nick, message.replyTo);
        }
    }
}

bool NickIdentityIndex::matchesOrigin(const QString &queryNick,
                                      const Message &message) const
{
    const QString queryLower = queryNick.toLower();
    const QString messageLower = message.nick.toLower();

    // Exact nick match is the common case and avoids extra lookup work.
    if (queryLower == messageLower)
        return true;

    const QSet<QString> aliases = aliasesFor(queryLower);

    // Nick-change aliases let searches follow locally observed renames.
    if (aliases.contains(messageLower))
        return true;

    const QSet<QString> queryAccounts = m_accountsByNick.value(queryLower);

    // Without any known account for the query nick, account matching cannot
    // safely broaden the identity set.
    if (queryAccounts.isEmpty())
        return false;

    // Prefer the account tag on the message itself when it is present.
    if (!message.account.isEmpty()
        && queryAccounts.contains(message.account.toLower())) {
        return true;
    }

    const QSet<QString> messageAccounts = m_accountsByNick.value(messageLower);

    // If the message nick has known account tags elsewhere in the loaded
    // channel, any shared account links the two displayed nicks.
    return std::any_of(
        messageAccounts.cbegin(),
        messageAccounts.cend(),
        [&](const QString &account) {
            return queryAccounts.contains(account);
        });
}

void NickIdentityIndex::connectNicks(const QString &left, const QString &right)
{
    // Store undirected lowercase edges so aliases are case-insensitive and can
    // be traversed from either old or new nick.
    const QString a = left.toLower();
    const QString b = right.toLower();
    m_nickGraph[a].insert(b);
    m_nickGraph[b].insert(a);
}

void NickIdentityIndex::addAccount(const QString &nick, const QString &account)
{
    // Ignore incomplete account data so partial IRCv3 tags do not create
    // meaningless identity links.
    if (nick.isEmpty() || account.isEmpty())
        return;

    m_accountsByNick[nick.toLower()].insert(account.toLower());
}

QSet<QString> NickIdentityIndex::aliasesFor(const QString &lowerNick) const
{
    QSet<QString> seen;
    QList<QString> pending{lowerNick};

    // Walk the alias graph iteratively to avoid recursive depth surprises from
    // long-running clients with many nick changes.
    while (!pending.isEmpty()) {
        const QString current = pending.takeLast();

        // Already-seen nicks are skipped to break cycles in the graph.
        if (seen.contains(current))
            continue;

        seen.insert(current);

        // Add unseen neighbors for later traversal.
        for (const QString &next : m_nickGraph.value(current)) {
            if (!seen.contains(next))
                pending << next;
        }
    }

    return seen;
}

// Applies global /search filters and the parsed expression to one loaded
// message. renderMatches and renderMentions call this before formatting output.
// Inputs are immutable model state; the function returns a boolean and produces
// no UI, disk, or network side effects.
bool messageMatches(const Message &message, const Command &command,
                    const QString &selfNick, const QDateTime &oldest,
                    const NickIdentityIndex &identities)
{
    // --other excludes messages authored by the current user.
    if (command.options.otherOnly
        && message.nick.compare(selfNick, Qt::CaseInsensitive) == 0) {
        return false;
    }

    // A valid oldest timestamp enforces span= against loaded message times.
    if (oldest.isValid() && message.timestamp < oldest)
        return false;

    return exprMatches(message, command.query, identities);
}

} // namespace Internal

// Renders local /search or /last results for one channel. CommandDispatcher
// calls this after parsing succeeds. It searches only loaded messages, adds
// context lines when requested, formats output lines, and returns text for the
// caller to display in Uplink's existing status output path.
QStringList renderMatches(const Channel &channel, const Command &command,
                          const QString &selfNick, const QDateTime &now)
{
    QStringList lines;
    QSet<int> included;

    // span= creates an oldest allowed timestamp; absence of span leaves the
    // timestamp invalid so no time filter is applied.
    const QDateTime oldest = command.options.spanSeconds > 0
        ? now.addSecs(-command.options.spanSeconds)
        : QDateTime{};
    const Internal::NickIdentityIndex identities(channel);

    // Visit loaded messages in display order so output follows the buffer.
    for (int i = 0; i < channel.messages.size(); ++i) {
        // Non-matching messages are skipped before any context expansion.
        if (!Internal::messageMatches(
                channel.messages[i],
                command,
                selfNick,
                oldest,
                identities))
            continue;

        const int first = std::max(0, i - command.options.contextLines);
        const int last = std::min(
            static_cast<int>(channel.messages.size()) - 1,
            i + command.options.contextLines);

        // Add the requested context range once, even when overlapping matches
        // would otherwise include the same line multiple times.
        for (int j = first; j <= last; ++j) {
            if (included.contains(j))
                continue;
            included.insert(j);
            lines << Internal::formatMessage(
                channel.messages[j],
                command.options,
                command.predicates);
        }
    }

    return lines;
}

} // namespace RichSearch
