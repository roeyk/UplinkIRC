// Search query parser.
//
// This module parses raw `/search`, `/last`, and `/mentions` argument text into
// command options, selector predicates, and a boolean expression tree with
// parentheses and AND/OR/NOT precedence. It is used by command rendering paths
// and parser unit tests. It performs syntax validation only and has no UI,
// disk, or network side effects.

#include "search/internal.h"

#include <QRegularExpression>

namespace RichSearch {
namespace {

// Builds a leaf expression node from one parsed selector. Parser::parsePrimary
// uses this after it has validated a command token and converted it into a
// Predicate. The returned node has no side effects; it is consumed later by the
// evaluator in queryeval.cpp.
ExprPtr makePredicateExpr(const Predicate &predicate)
{
    auto expr = std::make_shared<Expr>();
    expr->kind = Expr::Kind::Predicate;
    expr->predicate = predicate;
    return expr;
}

// Builds a boolean expression node around child nodes. The recursive descent
// parser uses this for AND, OR, and NOT groups. `kind` selects the boolean
// operation, `children` are already-parsed subexpressions, and the result is
// evaluated later against each loaded message.
ExprPtr makeBranch(Expr::Kind kind, const QList<ExprPtr> &children)
{
    auto expr = std::make_shared<Expr>();
    expr->kind = kind;
    expr->children = children;
    return expr;
}

// Checks whether a token is an unquoted boolean keyword.
//
// Called by parser precedence methods. `arg` is the current token and `word` is
// the operator name being tested. Returns false for quoted tokens so text such
// as `"and"` remains searchable as literal message text. Produces no side
// effects.
bool isOperator(const Internal::Arg &arg, const QString &word)
{
    return !arg.quoted && arg.text.compare(word, Qt::CaseInsensitive) == 0;
}

// Extracts leading string modifiers from values such as `ix"deploy.*"`.
//
// Called by `parsePredicate` after selector aliases have been normalized.
// `value` is rewritten to the inner string, `predicate` receives modifier
// flags, and a non-empty return value is a user-facing syntax error. It only
// mutates caller-owned parser data and has no IO or network side effects.
QString takeModifierPrefix(QString *value, Predicate *predicate)
{
    // Values without an embedded quote are ordinary selector values, so there
    // is no modifier prefix to consume.
    if (!value->contains(u'"'))
        return {};

    const qsizetype quote = value->indexOf(u'"');
    const QString modifiers = value->left(quote).toLower();
    QString rest = value->mid(quote + 1);

    // The tokenizer already removes whole-token quotes. This handles the
    // modifier-specific inner quote form, where a closing quote may remain.
    if (rest.endsWith(u'"'))
        rest.chop(1);

    // Each modifier toggles one predicate flag. Unknown modifiers fail closed
    // so misspelled query logic cannot silently broaden a search.
    for (const QChar modifier : modifiers) {
        if (modifier == u'i') {
            predicate->caseInsensitive = true;
        } else if (modifier == u'n') {
            predicate->negated = true;
        } else if (modifier == u'x') {
            predicate->regex = true;
        } else if (modifier == u'a' || modifier == u'o') {
            // Boolean relationship modifiers are accepted for portability, but
            // the keyword/parenthesis grammar owns actual grouping here.
        } else {
            return QStringLiteral("unknown string modifier: %1").arg(modifier);
        }
    }

    *value = rest;
    return {};
}

// Converts one command token into a Predicate.
//
// Called by `Parser::parsePrimary` for leaf terms. `arg` is the token to
// interpret, `predicate` receives the normalized selector, and `error` receives
// a user-facing parse failure. Returns true on success. It performs syntax
// interpretation only and does not evaluate messages.
bool parsePredicate(const Internal::Arg &arg, Predicate *predicate, QString *error)
{
    // A fully quoted token is always literal message text, so
    // `"origin=alice"` searches for that text instead of selecting by origin.
    if (arg.quoted) {
        predicate->field = Field::Text;
        predicate->value = arg.text;
        predicate->caseInsensitive = true;
        return true;
    }

    const qsizetype eq = arg.text.indexOf(u'=');
    // Bare words are shorthand for case-insensitive text search.
    if (eq == -1) {
        predicate->field = Field::Text;
        predicate->value = arg.text;
        predicate->caseInsensitive = true;
        return true;
    }

    QString key = arg.text.left(eq).toLower();
    QString value = arg.text.mid(eq + 1);

    // Empty selector values are almost always typos, so reject them instead of
    // matching everything or nothing ambiguously.
    if (value.isEmpty()) {
        *error = QStringLiteral("expected selector value");
        return false;
    }

    // Normalize compatibility selector names before applying string modifiers.
    Internal::normalizeSelector(&key, predicate);

    const QString modifierError = takeModifierPrefix(&value, predicate);
    if (!modifierError.isEmpty()) {
        *error = modifierError;
        return false;
    }

    // Convert the normalized selector key into the evaluator's field enum.
    if (!Internal::assignSelectorField(key, predicate, error))
        return false;

    predicate->value = value;
    return true;
}

// Recursive descent parser for the /search expression grammar. RichSearch::parse
// extracts options first, then this class returns the boolean tree plus a flat
// predicate list for future highlighting/completion work.
class Parser {
public:
    // Creates a parser over an already-tokenized expression argument list.
    //
    // Called only by `RichSearch::parse` after global options have been
    // removed. `tokens` is copied into parser state, and the parser produces an
    // expression tree plus a flat predicate list for highlighting.
    explicit Parser(const QList<Internal::Arg> &tokens)
        : m_tokens(tokens)
    {}

    // Parses a full expression and rejects any trailing unconsumed tokens.
    // Callers pass `error` for user-visible diagnostics. On success the return
    // value is the root expression used by renderMatches/renderMentions.
    ExprPtr parse(QString *error)
    {
        ExprPtr expr = parseOr(error);

        // Stop immediately when a nested parser level has already identified a
        // syntax error.
        if (!error->isEmpty())
            return {};

        // Any token left after a valid expression is unexpected input.
        if (m_pos < m_tokens.size()) {
            *error = QStringLiteral("unexpected token: %1").arg(m_tokens[m_pos].text);
            return {};
        }
        return expr;
    }

    // Returns the flat predicate list collected while parsing.
    //
    // Called by `RichSearch::parse` after successful expression parsing.
    // `renderMatches` and `formatMessage` later use this list for visible match
    // highlighting. The return value is a copy and has no side effects.
    QList<Predicate> predicates() const { return m_predicates; }

private:
    // Parses the lowest-precedence OR level.
    ExprPtr parseOr(QString *error)
    {
        QList<ExprPtr> children{parseAnd(error)};

        // If the left-hand side failed, preserve that error instead of trying
        // to recover inside the same expression.
        if (!error->isEmpty())
            return {};

        // Consume as many OR-separated groups as are present at this level.
        while (m_pos < m_tokens.size() && isOperator(m_tokens[m_pos], QStringLiteral("OR"))) {
            ++m_pos;
            children << parseAnd(error);

            // Stop after a malformed right-hand side.
            if (!error->isEmpty())
                return {};
        }

        // A single child does not need a wrapper node; multiple children become
        // one OR branch for the evaluator.
        return children.size() == 1 ? children.first() : makeBranch(Expr::Kind::Or, children);
    }

    // Parses explicit AND and implicit adjacent-term AND groups.
    ExprPtr parseAnd(QString *error)
    {
        QList<ExprPtr> children{parseUnary(error)};

        // A failed first child means this group is invalid; keep the original
        // diagnostic intact.
        if (!error->isEmpty())
            return {};

        // Continue until an OR or close parenthesis returns control to an outer
        // parser level. Any other adjacent term is treated as implicit AND.
        while (m_pos < m_tokens.size()) {
            if (isOperator(m_tokens[m_pos], QStringLiteral("OR"))
                || m_tokens[m_pos].text == QStringLiteral(")")) {
                break;
            }

            // Explicit AND is optional; when present, consume it before parsing
            // the next unary expression.
            if (isOperator(m_tokens[m_pos], QStringLiteral("AND")))
                ++m_pos;

            children << parseUnary(error);

            // Stop on malformed children.
            if (!error->isEmpty())
                return {};
        }

        // A single child does not need a wrapper node; multiple children become
        // one AND branch for the evaluator.
        return children.size() == 1 ? children.first() : makeBranch(Expr::Kind::And, children);
    }

    // Parses unary NOT. The recursive call allows repeated negation, such as
    // `NOT NOT text=deploy`, while preserving normal primary parsing.
    ExprPtr parseUnary(QString *error)
    {
        // A leading NOT wraps the following unary expression in a Not branch.
        if (m_pos < m_tokens.size() && isOperator(m_tokens[m_pos], QStringLiteral("NOT"))) {
            ++m_pos;
            return makeBranch(Expr::Kind::Not, {parseUnary(error)});
        }

        // Without NOT, parsing falls through to the primary expression level.
        return parsePrimary(error);
    }

    // Parses a parenthesized expression or a leaf predicate.
    ExprPtr parsePrimary(QString *error)
    {
        // End-of-input at primary position means an operator or open group did
        // not receive the operand it needs.
        if (m_pos >= m_tokens.size()) {
            *error = QStringLiteral("expected search term");
            return {};
        }

        const Internal::Arg arg = m_tokens[m_pos++];

        // Parentheses create a nested expression and require an explicit close
        // parenthesis before returning to the caller's precedence level.
        if (!arg.quoted && arg.text == QStringLiteral("(")) {
            ExprPtr expr = parseOr(error);
            if (!error->isEmpty())
                return {};

            // Missing close parenthesis is reported at the group boundary,
            // which points the user at the unmatched open group.
            if (m_pos >= m_tokens.size() || m_tokens[m_pos].text != QStringLiteral(")")) {
                *error = QStringLiteral("expected ')'");
                return {};
            }
            ++m_pos;
            return expr;
        }

        // A close parenthesis cannot begin a primary; it belongs to an outer
        // parse level.
        if (!arg.quoted && arg.text == QStringLiteral(")")) {
            *error = QStringLiteral("unexpected ')'");
            return {};
        }

        Predicate predicate;

        // Leaf parsing validates selectors, aliases, and string modifiers.
        if (!parsePredicate(arg, &predicate, error))
            return {};

        // Keep a flat copy of every positive/negative leaf predicate so
        // display formatting can highlight body matches without walking the
        // boolean tree again.
        m_predicates << predicate;
        return makePredicateExpr(predicate);
    }

    QList<Internal::Arg> m_tokens;
    qsizetype m_pos{0};
    QList<Predicate> m_predicates;
};

} // namespace

// Parses the argument text for /search, /last, and /mentions. The result is a
// normalized Command or a user-facing error; it performs no I/O or IRC traffic.
Result parse(const QString &input)
{
    Result result;
    QString error;
    QList<Internal::Arg> args = Internal::tokenize(input, &error);

    // Tokenizer errors, such as unterminated quotes, stop parsing before any
    // selector interpretation happens.
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }

    QList<Internal::Arg> expressionArgs;

    // Split global command options from expression terms.
    for (const Internal::Arg &arg : std::as_const(args)) {
        if (Internal::consumeFlag(arg, &result.command.options))
            continue;
        if (Internal::consumeOption(arg, &result.command.options, &result.error)) {
            if (!result.error.isEmpty())
                return result;
            continue;
        }
        expressionArgs << arg;
    }
    // A command with only flags/options has no predicate to evaluate.
    if (expressionArgs.isEmpty()) {
        result.error = QStringLiteral("expected search query");
        return result;
    }

    Parser parser(expressionArgs);
    result.command.query = parser.parse(&result.error);

    // Preserve parser diagnostics directly for the command error box/output.
    if (!result.error.isEmpty())
        return result;

    result.command.predicates = parser.predicates();
    result.ok = true;
    return result;
}

} // namespace RichSearch
