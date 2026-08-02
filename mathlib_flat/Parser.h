#pragma once
//
// Parser.h — turns strings like "3*x^2 - sin(x)/2" or "f(x) = x^2 + 1"
// into an Expr tree.

#include "Expr.h"
#include <string>
#include <stdexcept>

namespace math {

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Result of parsing a full function definition, e.g. "f(x) = x^2 + 1"
// or "f(x, y) = x + y".
struct ParsedDefinition {
    std::string name;                    // "f"
    std::vector<std::string> params;     // ["x"] or ["x","y"]
    ExprPtr body;                        // x^2 + 1
};

class Parser {
public:
    explicit Parser(std::string input);

    // Parses a bare expression, e.g. "x^2 + 3*x - 1".
    static ExprPtr parseExpression(const std::string& input);

    // Parses "name(params) = body" and returns the pieces.
    static ParsedDefinition parseDefinition(const std::string& input);

private:
    std::string m_in;
    size_t m_pos = 0;

    ExprPtr parseExpr();     // + -
    ExprPtr parseTerm();     // * /
    ExprPtr parseUnary();    // unary -, +
    ExprPtr parsePow();      // ^  (right-assoc)
    ExprPtr parsePostfix();  // primary, then implicit mult / factorial-ish extension point
    ExprPtr parsePrimary();  // numbers, identifiers, calls, parens

    void skipWs();
    bool eof() const;
    char peek() const;
    char get();
    bool matchChar(char c);
    void expect(char c, const char* ctx);

    std::string parseIdentifierRaw();
    double parseNumberRaw();

    static const std::string& knownFuncName(const std::string& id);
};

} // namespace math
