#include "Parser.h"

#include <cctype>
#include <set>
#include <algorithm>

namespace math {

static const std::set<std::string>& knownFunctions() {
    static const std::set<std::string> F = {
        "sin", "cos", "tan", "asin", "acos", "atan",
        "sinh", "cosh", "tanh",
        "exp", "ln", "log", "sqrt", "abs", "floor", "ceil", "sign"
    };
    return F;
}

// A small set of non-English function-name aliases, canonicalized to their
// knownFunctions() name right here at parse time so every downstream
// consumer (Expr::eval, Calculus.cpp's derivative()/integral(), Domain-
// Inference.cpp) only ever sees the canonical name and needs no changes.
// "sen" is Portuguese/Spanish for sine.
static std::string canonicalFuncName(const std::string& id) {
    if (id == "sen") return "sin";
    return id;
}

Parser::Parser(std::string input) : m_in(std::move(input)) {}

void Parser::skipWs() { while (!eof() && std::isspace((unsigned char)peek())) m_pos++; }
bool Parser::eof() const { return m_pos >= m_in.size(); }
char Parser::peek() const { return eof() ? '\0' : m_in[m_pos]; }
char Parser::get() { return m_in[m_pos++]; }

bool Parser::matchChar(char c) {
    skipWs();
    if (!eof() && peek() == c) { m_pos++; return true; }
    return false;
}

void Parser::expect(char c, const char* ctx) {
    if (!matchChar(c)) {
        throw ParseError(std::string("expected '") + c + "' " + ctx +
                          " at position " + std::to_string(m_pos) + " in \"" + m_in + "\"");
    }
}

std::string Parser::parseIdentifierRaw() {
    skipWs();
    size_t start = m_pos;
    while (!eof() && (std::isalnum((unsigned char)peek()) || peek() == '_')) m_pos++;
    if (m_pos == start) throw ParseError("expected identifier at position " + std::to_string(m_pos));
    return m_in.substr(start, m_pos - start);
}

double Parser::parseNumberRaw() {
    skipWs();
    size_t start = m_pos;
    while (!eof() && (std::isdigit((unsigned char)peek()) || peek() == '.')) m_pos++;
    // scientific notation, e.g. 1e-5
    if (!eof() && (peek() == 'e' || peek() == 'E')) {
        size_t save = m_pos;
        m_pos++;
        if (!eof() && (peek() == '+' || peek() == '-')) m_pos++;
        if (!eof() && std::isdigit((unsigned char)peek())) {
            while (!eof() && std::isdigit((unsigned char)peek())) m_pos++;
        } else {
            m_pos = save; // not actually exponent notation
        }
    }
    if (m_pos == start) throw ParseError("expected number at position " + std::to_string(m_pos));
    return std::stod(m_in.substr(start, m_pos - start));
}

// ---- grammar --------------------------------------------------------------
//
// expr    := term (('+'|'-') term)*
// term    := unary (('*'|'/') unary)*
// unary   := ('-'|'+')? pow
// pow     := postfix ('^' unary)?          // right-assoc, unary allows -x^2 = -(x^2)
// postfix := primary (implicit-mult primary)*
// primary := number | identifier ('(' args ')')? | '(' expr ')'

ExprPtr Parser::parseExpr() {
    ExprPtr left = parseTerm();
    for (;;) {
        skipWs();
        if (matchChar('+')) {
            left = Expr::add(left, parseTerm());
        } else if (matchChar('-')) {
            left = Expr::sub(left, parseTerm());
        } else break;
    }
    return left;
}

ExprPtr Parser::parseTerm() {
    ExprPtr left = parseUnary();
    for (;;) {
        skipWs();
        if (matchChar('*')) {
            left = Expr::mul(left, parseUnary());
        } else if (matchChar('/')) {
            left = Expr::div(left, parseUnary());
        } else break;
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    skipWs();
    if (matchChar('-')) return Expr::neg(parseUnary());
    if (matchChar('+')) return parseUnary();
    return parsePow();
}

ExprPtr Parser::parsePow() {
    ExprPtr base = parsePostfix();
    skipWs();
    if (matchChar('^')) {
        ExprPtr exp = parseUnary(); // right-assoc, allows 2^-1
        return Expr::pow(base, exp);
    }
    return base;
}

ExprPtr Parser::parsePostfix() {
    ExprPtr left = parsePrimary();
    for (;;) {
        skipWs();
        if (eof()) break;
        char c = peek();
        // implicit multiplication: "2x", "2(x+1)", "x sin(x)" but NOT across
        // an operator or closing paren/comma that belongs to the caller.
        bool canImplicit =
            std::isdigit((unsigned char)c) || std::isalpha((unsigned char)c) ||
            c == '_' || c == '(';
        if (!canImplicit) break;
        left = Expr::mul(left, parsePow());
    }
    return left;
}

ExprPtr Parser::parsePrimary() {
    skipWs();
    if (eof()) throw ParseError("unexpected end of input");

    char c = peek();

    if (c == '(') {
        get();
        ExprPtr e = parseExpr();
        expect(')', "to close '('");
        return e;
    }

    if (std::isdigit((unsigned char)c) || c == '.') {
        return Expr::num(parseNumberRaw());
    }

    if (std::isalpha((unsigned char)c) || c == '_') {
        std::string id = parseIdentifierRaw();
        skipWs();
        if (!eof() && peek() == '(') {
            get(); // consume '('
            std::vector<ExprPtr> callArgs;
            skipWs();
            if (!matchChar(')')) {
                callArgs.push_back(parseExpr());
                while (matchChar(',')) callArgs.push_back(parseExpr());
                expect(')', "to close function/call arguments");
            }
            std::string canon = canonicalFuncName(id);
            if (knownFunctions().count(canon)) {
                if (callArgs.size() == 1) return Expr::func(canon, callArgs[0]);
                if (callArgs.size() == 2) return Expr::func(canon, callArgs[0], callArgs[1]);
                throw ParseError("built-in function '" + id + "' takes 1 or 2 arguments");
            }
            return Expr::call(id, callArgs);
        }
        // bare identifier: symbol (covers x, y, pi, e, or a 0-arg reference to
        // a user function used as a constant, which is unusual but harmless)
        return Expr::sym(id);
    }

    throw ParseError(std::string("unexpected character '") + c + "' at position " + std::to_string(m_pos));
}

ExprPtr Parser::parseExpression(const std::string& input) {
    Parser p(input);
    ExprPtr e = p.parseExpr();
    p.skipWs();
    if (!p.eof()) throw ParseError("unexpected trailing input at position " + std::to_string(p.m_pos) +
                                    " in \"" + input + "\"");
    return e;
}

ParsedDefinition Parser::parseDefinition(const std::string& input) {
    // find the top-level '=' that isn't part of '==', '<=', '>=', '!='
    size_t eqPos = std::string::npos;
    int depth = 0;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '(') depth++;
        else if (c == ')') depth--;
        else if (c == '=' && depth == 0) {
            char prev = i > 0 ? input[i - 1] : '\0';
            char next = i + 1 < input.size() ? input[i + 1] : '\0';
            if (next == '=' ) continue; // "=="
            if (prev == '<' || prev == '>' || prev == '!' || prev == '=') continue;
            eqPos = i;
            break;
        }
    }
    if (eqPos == std::string::npos) {
        throw ParseError("expected a function definition of the form 'f(x) = ...' in \"" + input + "\"");
    }

    std::string lhs = input.substr(0, eqPos);
    std::string rhs = input.substr(eqPos + 1);

    Parser lp(lhs);
    lp.skipWs();
    std::string fname = lp.parseIdentifierRaw();
    lp.skipWs();
    std::vector<std::string> params;
    if (lp.matchChar('(')) {
        lp.skipWs();
        if (!lp.matchChar(')')) {
            params.push_back(lp.parseIdentifierRaw());
            while (lp.matchChar(',')) {
                lp.skipWs();
                params.push_back(lp.parseIdentifierRaw());
            }
            lp.expect(')', "to close parameter list");
        }
    } else {
        throw ParseError("expected '(' after function name in definition \"" + input + "\"");
    }
    lp.skipWs();
    if (!lp.eof()) throw ParseError("unexpected trailing tokens on left-hand side of \"" + input + "\"");

    ExprPtr body = parseExpression(rhs);

    ParsedDefinition def;
    def.name = fname;
    def.params = params;
    def.body = body;
    return def;
}

} // namespace math
