#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cctype>
#include <algorithm>
#include <stdexcept>

enum class TokType {
    Number, Ident, String,
    Var, Print, If, Else, While, For, TrueKw, FalseKw, CharKw, LenKw, StringKw,
    Function, Return, StructKw,
    Parallel, ThreadKw, JoinKw, JoinAllKw,
    HttpKw, HeaderKw, BodyKw,
    Plus, Minus, Star, Slash, Percent,
    Assign,
    EqualEqual, BangEqual, Less, LessEqual, Greater, GreaterEqual,
    AndAnd, OrOr, Bang,
    Semicolon, Comma, Dot, Colon,
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    End
};

struct Token {
    TokType type;
    std::string text;
    double number = 0.0;
    int line = 1;
};

struct Lexer {
    std::string src;
    size_t pos = 0;
    int line = 1;
    Lexer(std::string s) : src(std::move(s)) {}
    char peek(int off = 0) {
        size_t p = pos + off;
        if (p >= src.size()) return '\0';
        return src[p];
    }
    char advance() {
        char c = src[pos++];
        if (c == '\n') line++;
        return c;
    }
    bool match(char c) {
        if (peek() != c) return false;
        pos++;
        return true;
    }
    void skipWhitespace() {
        while (true) {
            while (std::isspace((unsigned char)peek())) advance();
            if (peek() == '/' && peek(1) == '/') {
                while (peek() != '\n' && peek() != '\0') advance();
                continue;
            }
            break;
        }
    }
    Token identifier() {
        int startLine = line;
        std::string s;
        while (std::isalnum((unsigned char)peek()) || peek() == '_') s += advance();
        if (s == "var") return {TokType::Var,s,0,startLine};
        if (s == "print") return {TokType::Print,s,0,startLine};
        if (s == "if") return {TokType::If,s,0,startLine};
        if (s == "else") return {TokType::Else,s,0,startLine};
        if (s == "while") return {TokType::While,s,0,startLine};
        if (s == "for") return {TokType::For,s,0,startLine};
        if (s == "true") return {TokType::TrueKw,s,0,startLine};
        if (s == "false") return {TokType::FalseKw,s,0,startLine};
        if (s == "char") return {TokType::CharKw,s,0,startLine};
        if (s == "len") return {TokType::LenKw,s,0,startLine};
        if (s == "function") return {TokType::Function,s,0,startLine};
        if (s == "return") return {TokType::Return,s,0,startLine};
        if (s == "Struct") return {TokType::StructKw,s,0,startLine};
        if (s == "Parallel") return {TokType::Parallel,s,0,startLine};
        if (s == "thread") return {TokType::ThreadKw,s,0,startLine};
        if (s == "join") return {TokType::JoinKw,s,0,startLine};
        if (s == "joinAll") return {TokType::JoinAllKw,s,0,startLine};
        if (s == "string") return {TokType::StringKw,s,0,startLine};
        if (s == "Http") return {TokType::HttpKw,s,0,startLine};
        if (s == "header") return {TokType::HeaderKw,s,0,startLine};
        if (s == "body") return {TokType::BodyKw,s,0,startLine};
        return {TokType::Ident,s,0,startLine};
    }
    Token number() {
        int startLine = line;
        std::string s;
        while (std::isdigit((unsigned char)peek())) s += advance();
        if (peek()=='.') {
            s+=advance();
            while (std::isdigit((unsigned char)peek())) s+=advance();
        }
        return {TokType::Number,s,std::stod(s),startLine};
    }
    Token stringLiteral() {
        int startLine = line;
        advance(); // consume opening '"'
        std::string s;
        while (peek() != '"') {
            if (peek() == '\0')
                throw std::runtime_error("Unterminated string literal starting at line " + std::to_string(startLine));
            char c = advance();
            if (c == '\\') {
                char esc = advance();
                switch (esc) {
                    case 'n': s += '\n'; break;
                    case 't': s += '\t'; break;
                    case '"': s += '"'; break;
                    case '\\': s += '\\'; break;
                    default: s += esc; break;
                }
            } else {
                s += c;
            }
        }
        advance(); // consume closing '"'
        return {TokType::String, s, 0, startLine};
    }
    std::vector<Token> tokenize() {
        std::vector<Token> out;
        while (true) {
            skipWhitespace();
            if (peek()=='\0') { out.push_back({TokType::End,"",0,line}); break; }
            char c=peek();
            int l=line;
            if (std::isdigit((unsigned char)c)) { out.push_back(number()); continue; }
            if (std::isalpha((unsigned char)c) || c=='_') { out.push_back(identifier()); continue; }
            if (c=='"') { out.push_back(stringLiteral()); continue; }
            advance();
            switch(c) {
            case '+': out.push_back({TokType::Plus,"+",0,l}); break;
            case '-': out.push_back({TokType::Minus,"-",0,l}); break;
            case '*': out.push_back({TokType::Star,"*",0,l}); break;
            case '%': out.push_back({TokType::Percent,"%",0,l}); break;
            case ';': out.push_back({TokType::Semicolon,";",0,l}); break;
            case ',': out.push_back({TokType::Comma,",",0,l}); break;
            case '.': out.push_back({TokType::Dot,".",0,l}); break;
            case ':': out.push_back({TokType::Colon,":",0,l}); break;
            case '(': out.push_back({TokType::LParen,"(",0,l}); break;
            case ')': out.push_back({TokType::RParen,")",0,l}); break;
            case '{': out.push_back({TokType::LBrace,"{",0,l}); break;
            case '}': out.push_back({TokType::RBrace,"}",0,l}); break;
            case '[': out.push_back({TokType::LBracket,"[",0,l}); break;
            case ']': out.push_back({TokType::RBracket,"]",0,l}); break;
            case '=':
                if(match('=')) out.push_back({TokType::EqualEqual,"==",0,l});
                else out.push_back({TokType::Assign,"=",0,l});
                break;
            case '!':
                if(match('=')) out.push_back({TokType::BangEqual,"!=",0,l});
                else out.push_back({TokType::Bang,"!",0,l});
                break;
            case '<':
                if(match('=')) out.push_back({TokType::LessEqual,"<=",0,l});
                else out.push_back({TokType::Less,"<",0,l});
                break;
            case '>':
                if(match('=')) out.push_back({TokType::GreaterEqual,">=",0,l});
                else out.push_back({TokType::Greater,">",0,l});
                break;
            case '&':
                if(match('&')) { out.push_back({TokType::AndAnd,"&&",0,l}); break; }
                throw std::runtime_error("Expected second '&'");
            case '|':
                if(match('|')) { out.push_back({TokType::OrOr,"||",0,l}); break; }
                throw std::runtime_error("Expected second '|'");
            case '/':
                out.push_back({TokType::Slash,"/",0,l});
                break;
            default:
                throw std::runtime_error("Unexpected character '"+std::string(1,c)+"'");
            }
        }
        return out;
    }
};

struct Expr { virtual ~Expr() = default; };
struct Stmt { virtual ~Stmt() = default; };
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct NumberExpr : Expr { double value; NumberExpr(double v):value(v){} };
struct BoolExpr : Expr { bool value; BoolExpr(bool v):value(v){} };
struct VarExpr : Expr { std::string name; VarExpr(std::string n):name(std::move(n)){} };
struct UnaryExpr : Expr { TokType op; ExprPtr expr; UnaryExpr(TokType o, ExprPtr e):op(o),expr(std::move(e)){} };
struct BinaryExpr : Expr { TokType op; ExprPtr left; ExprPtr right; BinaryExpr(TokType o, ExprPtr l, ExprPtr r):op(o),left(std::move(l)),right(std::move(r)){} };
struct CharCastExpr : Expr { ExprPtr expr; CharCastExpr(ExprPtr e):expr(std::move(e)){} };
struct StringCastExpr : Expr { ExprPtr expr; StringCastExpr(ExprPtr e):expr(std::move(e)){} };
struct StringLitExpr : Expr { std::string value; StringLitExpr(std::string v):value(std::move(v)){} };

struct JsonObjectLitExpr : Expr { std::vector<std::pair<ExprPtr,ExprPtr>> entries; };

struct HttpCallExpr : Expr {
    std::string method;
    ExprPtr url;
    std::vector<std::pair<ExprPtr,ExprPtr>> headers;
    ExprPtr bodyExpr; // null if no body{} block was given
};
struct LenExpr : Expr { std::string arrayName; LenExpr(std::string n):arrayName(std::move(n)){} };
struct ArrayAccessExpr : Expr { std::string arrayName; ExprPtr index; ArrayAccessExpr(std::string n, ExprPtr i):arrayName(std::move(n)),index(std::move(i)){} };
struct CallExpr : Expr { std::string name; std::vector<ExprPtr> args; };
struct MemberAccessExpr : Expr { ExprPtr base; std::string member; ExprPtr index; };

struct ThreadExpr : Expr { std::string fnName; std::vector<ExprPtr> args; };
struct JoinExpr : Expr { ExprPtr handleExpr; };
struct JoinAllExpr : Expr { std::string arrayName; };

enum class ArrayMethod { Push, Pop, Sort, Reverse, Contains, IndexOf, Accumulate };
struct ArrayMethodCallExpr : Expr { std::string arrayName; ArrayMethod method; ExprPtr arg; };

struct VarDeclStmt : Stmt { bool isArray = false; bool isEmptyArray = false; std::string name; ExprPtr value; std::vector<ExprPtr> arrayValues; };
struct AssignStmt : Stmt { std::string name; ExprPtr value; };
struct ArrayAssignStmt : Stmt { std::string arrayName; ExprPtr index; ExprPtr value; };
struct MemberAssignStmt : Stmt { std::string objectName; std::string member; ExprPtr index; ExprPtr value; };
struct PrintStmt : Stmt { bool printChar=false; bool printCharArray=false; ExprPtr expr; std::string arrayName; };
struct BlockStmt : Stmt { std::vector<StmtPtr> statements; };
struct IfStmt : Stmt { ExprPtr condition; std::shared_ptr<BlockStmt> thenBlock; std::shared_ptr<BlockStmt> elseBlock; };
struct WhileStmt : Stmt { ExprPtr condition; std::shared_ptr<BlockStmt> body; };
struct ForStmt : Stmt { StmtPtr init; ExprPtr condition; StmtPtr increment; std::shared_ptr<BlockStmt> body; };
struct ReturnStmt : Stmt { ExprPtr value; };
struct ExprStmt : Stmt { ExprPtr expr; };
struct ParallelStmt : Stmt { std::vector<std::shared_ptr<BlockStmt>> blocks; };

// --- HTTP server AST ---

// Server() { host = "0.0.0.0"; port = 8080; ... } -- a config object literal,
// parsed the same way struct-ctor-like blocks are: "key = expr;" lines.
// Recognized by the identifier "Server" (soft keyword, like "push"/"header").
struct ServerConfigExpr : Expr {
    std::vector<std::pair<std::string, ExprPtr>> entries;
};

// server.GET("/users/:id") { ...body... };
// Registered against the named server variable at "compile time" (i.e. when
// this statement executes, since CnR has no separate registration pass).
// `method` is the literal HTTP verb text (GET/POST/PUT/PATCH/DELETE).
struct RouteDeclStmt : Stmt {
    std::string serverName;
    std::string method;
    ExprPtr pathExpr; // usually a StringLitExpr
    std::shared_ptr<BlockStmt> body;
};

// server.start(); -- begins accepting connections (blocking call).
struct ServerStartStmt : Stmt {
    std::string serverName;
};

// A call to a builtin "object method" that isn't a plain field or array
// method -- e.g. response.header("K","V"), response.cookie("k","v",3600),
// response.redirect("/somewhere"). Distinguished from ArrayMethodCallExpr
// (which only knows about numeric arrays) and from ordinary function calls
// (which don't have a receiver object).
struct ObjectMethodCallExpr : Expr {
    std::string objectName; // "response" (or "request", though it has none yet)
    std::string methodName; // "header" / "cookie" / "redirect"
    std::vector<ExprPtr> args;
};

struct FunctionDecl {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
};

struct StructField {
    std::string name;
    bool isArray = false;
};

struct StructDecl {
    std::string name;
    std::vector<StructField> fields;
    std::vector<std::string> ctorParams;
    std::shared_ptr<BlockStmt> ctorBody;
};

struct Program {
    std::vector<StmtPtr> statements;
    std::unordered_map<std::string, FunctionDecl> functions;
    std::unordered_map<std::string, StructDecl> structs;
};

struct Parser {
    std::vector<Token> tokens;
    size_t pos = 0;
    Parser(std::vector<Token> t) : tokens(std::move(t)) {}

    Token& peek(int off = 0) {
        size_t p = pos + off;
        if (p >= tokens.size()) return tokens.back();
        return tokens[p];
    }
    bool check(TokType t) { return peek().type == t; }
    bool match(TokType t) { if (!check(t)) return false; pos++; return true; }
    Token advance() { return tokens[pos++]; }
    Token expect(TokType t,const std::string& msg) {
        if (!check(t)) {
            throw std::runtime_error("Parse error line " + std::to_string(peek().line) + ": expected " + msg);
        }
        return advance();
    }

    bool tryArrayMethodName(const std::string& text, ArrayMethod& out) {
        if(text=="push") { out=ArrayMethod::Push; return true; }
        if(text=="pop") { out=ArrayMethod::Pop; return true; }
        if(text=="sort") { out=ArrayMethod::Sort; return true; }
        if(text=="reverse") { out=ArrayMethod::Reverse; return true; }
        if(text=="contains") { out=ArrayMethod::Contains; return true; }
        if(text=="indexOf") { out=ArrayMethod::IndexOf; return true; }
        if(text=="accumulate") { out=ArrayMethod::Accumulate; return true; }
        return false;
    }
    bool arrayMethodTakesArg(ArrayMethod m) {
        return m==ArrayMethod::Push || m==ArrayMethod::Contains || m==ArrayMethod::IndexOf;
    }
    ExprPtr parseArrayMethodCall(const std::string& arrayName) {
        advance(); // '.'
        std::string methodText = advance().text; // method name ident
        ArrayMethod m = ArrayMethod::Push;
        tryArrayMethodName(methodText, m); // already validated by caller
        expect(TokType::LParen,"(");
        auto call = std::make_shared<ArrayMethodCallExpr>();
        call->arrayName = arrayName;
        call->method = m;
        if(arrayMethodTakesArg(m)) {
            call->arg = parseExpression();
        }
        expect(TokType::RParen,")");
        return call;
    }

    std::vector<ExprPtr> stringToArrayValues(const std::string& text) {
        std::vector<ExprPtr> values;
        values.reserve(text.size());
        for(unsigned char ch : text) values.push_back(std::make_shared<NumberExpr>((double)ch));
        return values;
    }

    void parseHttpOptionsBlock(std::shared_ptr<HttpCallExpr>& call) {
        expect(TokType::LBrace,"{");
        while(!check(TokType::RBrace)) {
            if(check(TokType::End))
                throw std::runtime_error("Unexpected end of file inside Http options block");
            if(match(TokType::HeaderKw)) {
                expect(TokType::LBrace,"{");
                while(!check(TokType::RBrace)) {
                    if(check(TokType::End))
                        throw std::runtime_error("Unexpected end of file inside header block");
                    ExprPtr key = parseExpression();
                    expect(TokType::Assign,"=");
                    ExprPtr val = parseExpression();
                    expect(TokType::Semicolon,";");
                    call->headers.push_back({key,val});
                }
                expect(TokType::RBrace,"}");
                continue;
            }
            if(match(TokType::BodyKw)) {
                expect(TokType::LBrace,"{");
                auto bodyObj = std::make_shared<JsonObjectLitExpr>();
                while(!check(TokType::RBrace)) {
                    if(check(TokType::End))
                        throw std::runtime_error("Unexpected end of file inside body block");
                    ExprPtr key = parseExpression();
                    expect(TokType::Assign,"=");
                    ExprPtr val = parseExpression();
                    expect(TokType::Semicolon,";");
                    bodyObj->entries.push_back({key,val});
                }
                expect(TokType::RBrace,"}");
                call->bodyExpr = bodyObj;
                continue;
            }
            throw std::runtime_error("Unexpected token in Http options block on line " + std::to_string(peek().line));
        }
        expect(TokType::RBrace,"}");
    }

    ExprPtr parseExpression() { return parseLogicalOr(); }
    ExprPtr parseLogicalOr() {
        auto left = parseLogicalAnd();
        while(match(TokType::OrOr)) {
            auto right = parseLogicalAnd();
            left = std::make_shared<BinaryExpr>(TokType::OrOr,left,right);
        }
        return left;
    }
    ExprPtr parseLogicalAnd() {
        auto left = parseEquality();
        while(match(TokType::AndAnd)) {
            auto right = parseEquality();
            left = std::make_shared<BinaryExpr>(TokType::AndAnd,left,right);
        }
        return left;
    }
    ExprPtr parseEquality() {
        auto left = parseComparison();
        while(true) {
            if(match(TokType::EqualEqual)) { auto right = parseComparison(); left = std::make_shared<BinaryExpr>(TokType::EqualEqual,left,right); continue; }
            if(match(TokType::BangEqual)) { auto right = parseComparison(); left = std::make_shared<BinaryExpr>(TokType::BangEqual,left,right); continue; }
            break;
        }
        return left;
    }
    ExprPtr parseComparison() {
        auto left = parseAddition();
        while(true) {
            if(match(TokType::Less)) { auto right = parseAddition(); left = std::make_shared<BinaryExpr>(TokType::Less,left,right); continue; }
            if(match(TokType::LessEqual)) { auto right = parseAddition(); left = std::make_shared<BinaryExpr>(TokType::LessEqual,left,right); continue; }
            if(match(TokType::Greater)) { auto right = parseAddition(); left = std::make_shared<BinaryExpr>(TokType::Greater,left,right); continue; }
            if(match(TokType::GreaterEqual)) { auto right = parseAddition(); left = std::make_shared<BinaryExpr>(TokType::GreaterEqual,left,right); continue; }
            break;
        }
        return left;
    }
    ExprPtr parseAddition() {
        auto left = parseMultiplication();
        while(true) {
            if(match(TokType::Plus)) { auto right = parseMultiplication(); left = std::make_shared<BinaryExpr>(TokType::Plus,left,right); continue; }
            if(match(TokType::Minus)) { auto right = parseMultiplication(); left = std::make_shared<BinaryExpr>(TokType::Minus,left,right); continue; }
            break;
        }
        return left;
    }
    ExprPtr parseMultiplication() {
        auto left = parseUnary();
        while(true) {
            if(match(TokType::Star)) { auto right = parseUnary(); left = std::make_shared<BinaryExpr>(TokType::Star,left,right); continue; }
            if(match(TokType::Slash)) { auto right = parseUnary(); left = std::make_shared<BinaryExpr>(TokType::Slash,left,right); continue; }
            if(match(TokType::Percent)) { auto right = parseUnary(); left = std::make_shared<BinaryExpr>(TokType::Percent,left,right); continue; }
            break;
        }
        return left;
    }
    ExprPtr parseUnary() {
        if(match(TokType::Minus)) return std::make_shared<UnaryExpr>(TokType::Minus, parseUnary());
        if(match(TokType::Bang)) return std::make_shared<UnaryExpr>(TokType::Bang, parseUnary());
        return parseCall();
    }
    ExprPtr parseCall() {
        auto expr = parsePrimary();
        while(check(TokType::Dot)) {
            if(peek(1).type == TokType::JoinKw && peek(2).type == TokType::LParen) {
                advance(); advance(); advance(); // . join (
                expect(TokType::RParen,")");
                auto j = std::make_shared<JoinExpr>();
                j->handleExpr = expr;
                expr = j;
                continue;
            }
            if(peek(1).type == TokType::JoinAllKw && peek(2).type == TokType::LParen) {
                if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
                    advance(); advance(); advance(); // . joinAll (
                    expect(TokType::RParen,")");
                    auto j = std::make_shared<JoinAllExpr>();
                    j->arrayName = v->name;
                    expr = j;
                    continue;
                }
                throw std::runtime_error("joinAll() must be called on an array name, line " + std::to_string(peek().line));
            }
            {
                ArrayMethod m;
                if(peek(1).type == TokType::Ident && tryArrayMethodName(peek(1).text, m) && peek(2).type == TokType::LParen) {
                    if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
                        expr = parseArrayMethodCall(v->name);
                        continue;
                    }
                    throw std::runtime_error("Array method '" + peek(1).text + "()' must be called on an array name, line " + std::to_string(peek().line));
                }
            }
            if(peek(1).type == TokType::Ident || peek(1).type == TokType::HeaderKw || peek(1).type == TokType::BodyKw) {
                advance(); // '.'
                auto ma = std::make_shared<MemberAccessExpr>();
                ma->base = expr;
                ma->member = advance().text; // "header"/"body" lex as keywords but are valid field names here (e.g. request.header[...], response.body)
                if(match(TokType::LBracket)) {
                    ma->index = parseExpression();
                    expect(TokType::RBracket,"]");
                }
                expr = ma;
                continue;
            }
            break;
        }
        return expr;
    }
    ExprPtr parsePrimary() {
        if(check(TokType::Number)) { double v = advance().number; return std::make_shared<NumberExpr>(v); }
        if(match(TokType::TrueKw)) return std::make_shared<BoolExpr>(true);
        if(match(TokType::FalseKw)) return std::make_shared<BoolExpr>(false);
        if(match(TokType::ThreadKw)) {
            expect(TokType::LParen,"(");
            auto t = std::make_shared<ThreadExpr>();
            t->fnName = expect(TokType::Ident,"function name").text;
            if(match(TokType::LParen)) {
                if(!check(TokType::RParen)) {
                    while(true) {
                        t->args.push_back(parseExpression());
                        if(match(TokType::Comma)) continue;
                        break;
                    }
                }
                expect(TokType::RParen,")");
            } else {
                while(match(TokType::Comma)) {
                    t->args.push_back(parseExpression());
                }
            }
            expect(TokType::RParen,")");
            return t;
        }
        if(check(TokType::String)) {
            return std::make_shared<StringLitExpr>(advance().text);
        }
        if(match(TokType::HttpKw)) {
            expect(TokType::Dot,".");
            std::string method = expect(TokType::Ident,"HTTP method (GET/POST/PUT/PATCH/DELETE)").text;
            expect(TokType::LParen,"(");
            auto call = std::make_shared<HttpCallExpr>();
            call->method = method;
            call->url = parseExpression();
            expect(TokType::RParen,")");
            if(check(TokType::LBrace)) parseHttpOptionsBlock(call);
            return call;
        }
        if(match(TokType::LParen)) {
            if(match(TokType::CharKw)) {
                if(match(TokType::LBracket)) {
                    expect(TokType::RBracket,"]");
                    expect(TokType::RParen,")");
                    auto id = expect(TokType::Ident,"identifier");
                    auto p = std::make_shared<ArrayAccessExpr>(id.text, nullptr);
                    return std::make_shared<CharCastExpr>(p);
                }
                expect(TokType::RParen,")");
                auto e = parseUnary();
                return std::make_shared<CharCastExpr>(e);
            }
            if(match(TokType::StringKw)) {
                expect(TokType::RParen,")");
                auto e = parseUnary();
                return std::make_shared<StringCastExpr>(e);
            }
            auto e = parseExpression();
            expect(TokType::RParen,")");
            return e;
        }
        if(check(TokType::LBrace)) {
            // { "key": expr, ... } or { key: expr, ... } -- a JSON-style object
            // literal usable anywhere an expression is expected (most commonly
            // in `return { ... };`). Keys may be string literals or bare
            // identifiers; values are full expressions, so this nests.
            advance(); // '{'
            auto obj = std::make_shared<JsonObjectLitExpr>();
            while(!check(TokType::RBrace)) {
                if(check(TokType::End))
                    throw std::runtime_error("Unexpected end of file inside object literal");
                ExprPtr key;
                if(check(TokType::String)) {
                    key = std::make_shared<StringLitExpr>(advance().text);
                } else if(check(TokType::Ident) || check(TokType::HeaderKw) || check(TokType::BodyKw)) {
                    key = std::make_shared<StringLitExpr>(advance().text);
                } else {
                    throw std::runtime_error("Expected object literal key (string or identifier) on line " + std::to_string(peek().line));
                }
                expect(TokType::Colon,":");
                ExprPtr val = parseExpression();
                obj->entries.push_back({key, val});
                if(match(TokType::Comma)) continue;
                break;
            }
            expect(TokType::RBrace,"}");
            return obj;
        }
        if(match(TokType::LenKw)) {
            expect(TokType::LParen,"(");
            auto id = expect(TokType::Ident,"identifier");
            expect(TokType::RParen,")");
            return std::make_shared<LenExpr>(id.text);
        }
        if(check(TokType::Ident) && peek().text == "Server" && peek(1).type == TokType::LParen) {
            advance(); // Server
            advance(); // (
            expect(TokType::RParen,")");
            auto cfg = std::make_shared<ServerConfigExpr>();
            expect(TokType::LBrace,"{");
            while(!check(TokType::RBrace)) {
                if(check(TokType::End))
                    throw std::runtime_error("Unexpected end of file inside Server() config block");
                std::string key = expect(TokType::Ident,"config key").text;
                expect(TokType::Assign,"=");
                ExprPtr val = parseExpression();
                expect(TokType::Semicolon,";");
                cfg->entries.push_back({key, val});
            }
            expect(TokType::RBrace,"}");
            return cfg;
        }
        if(check(TokType::Ident)) {
            std::string name = advance().text;
            if(match(TokType::LParen)) {
                auto call = std::make_shared<CallExpr>();
                call->name = name;
                if(!check(TokType::RParen)) {
                    while(true) {
                        call->args.push_back(parseExpression());
                        if(match(TokType::Comma)) continue;
                        break;
                    }
                }
                expect(TokType::RParen,")");
                return call;
            }
            if(match(TokType::LBracket)) {
                auto idx = parseExpression();
                expect(TokType::RBracket,"]");
                return std::make_shared<ArrayAccessExpr>(name, idx);
            }
            return std::make_shared<VarExpr>(name);
        }
        throw std::runtime_error("Unexpected token on line " + std::to_string(peek().line));
    }

    std::shared_ptr<BlockStmt> parseBlock() {
        expect(TokType::LBrace,"'{'");
        auto block = std::make_shared<BlockStmt>();
        while(!check(TokType::RBrace)) {
            if(check(TokType::End))
                throw std::runtime_error("Unexpected end of file: missing '}' to close block (opened near line " + std::to_string(peek().line) + ")");
            block->statements.push_back(parseStatement());
        }
        expect(TokType::RBrace,"'}'");
        return block;
    }

    StmtPtr parsePrint() {
        expect(TokType::Print,"print");
        expect(TokType::LParen,"(");
        auto stmt = std::make_shared<PrintStmt>();
        if(check(TokType::StringKw) && peek(1).type==TokType::Dot && peek(2).type==TokType::Ident) {
            advance(); // string
            advance(); // .
            std::string arrName = advance().text; // array name
            expect(TokType::RParen,")");
            expect(TokType::Semicolon,";");
            stmt->printCharArray = true;
            stmt->arrayName = arrName;
            return stmt;
        }
        if(check(TokType::LParen) && peek(1).type==TokType::CharKw && peek(2).type==TokType::LBracket) {
            advance(); advance();
            expect(TokType::LBracket,"[");
            expect(TokType::RBracket,"]");
            expect(TokType::RParen,")");
            stmt->printCharArray=true;
            stmt->arrayName= expect(TokType::Ident,"identifier").text;
            expect(TokType::RParen,")");
            expect(TokType::Semicolon,";");
            return stmt;
        }
        if(check(TokType::LParen) && peek(1).type==TokType::CharKw) {
            advance(); advance();
            expect(TokType::RParen,")");
            stmt->printChar=true;
            stmt->expr=parseExpression();
            expect(TokType::RParen,")");
            expect(TokType::Semicolon,";");
            return stmt;
        }
        stmt->expr=parseExpression();
        expect(TokType::RParen,")");
        expect(TokType::Semicolon,";");
        return stmt;
    }

    StmtPtr parseVarDecl(bool consumeSemicolon = true) {
        expect(TokType::Var,"var");
        auto stmt = std::make_shared<VarDeclStmt>();
        if(check(TokType::LBracket) && peek(1).type == TokType::RBracket) {
            advance(); advance(); // [ ]
            stmt->name = expect(TokType::Ident,"identifier").text;
            stmt->isArray = true;
            if(match(TokType::Assign)) {
                if(check(TokType::String)) {
                    stmt->arrayValues = stringToArrayValues(advance().text);
                } else {
                    expect(TokType::LBrace,"{");
                    while(true) {
                        stmt->arrayValues.push_back(parseExpression());
                        if(match(TokType::Comma)) continue;
                        break;
                    }
                    expect(TokType::RBrace,"}");
                }
            } else {
                stmt->isEmptyArray = true;
            }
            if(consumeSemicolon) expect(TokType::Semicolon,";");
            return stmt;
        }
        stmt->name= expect(TokType::Ident,"identifier").text;
        if(match(TokType::LBracket)) {
            expect(TokType::RBracket,"]");
            stmt->isArray=true;
            expect(TokType::Assign,"=");
            if(check(TokType::String)) {
                stmt->arrayValues = stringToArrayValues(advance().text);
                if(consumeSemicolon) expect(TokType::Semicolon,";");
                return stmt;
            }
            expect(TokType::LBrace,"{");
            while(true) {
                stmt->arrayValues.push_back(parseExpression());
                if(match(TokType::Comma)) continue;
                break;
            }
            expect(TokType::RBrace,"}");
            if(consumeSemicolon) expect(TokType::Semicolon,";");
            return stmt;
        }
        expect(TokType::Assign,"=");
        stmt->value=parseExpression();
        if(consumeSemicolon) expect(TokType::Semicolon,";");
        return stmt;
    }

    StmtPtr parseAssignmentStatement(bool consumeSemicolon = true) {
        auto id= expect(TokType::Ident,"identifier");
        if(check(TokType::LParen)) {
            advance();
            auto call = std::make_shared<CallExpr>();
            call->name = id.text;
            if(!check(TokType::RParen)) {
                while(true) {
                    call->args.push_back(parseExpression());
                    if(match(TokType::Comma)) continue;
                    break;
                }
            }
            expect(TokType::RParen,")");
            if(consumeSemicolon) expect(TokType::Semicolon,";");
            auto stmt = std::make_shared<ExprStmt>();
            stmt->expr = call;
            return stmt;
        }
        if(check(TokType::Dot)) {
            // server.GET("/path") { ... }; -- route declaration.
            // Recognized by an HTTP-verb identifier immediately followed by '(' and,
            // after the matching ')', a '{' -- this shape never collides with
            // ordinary member access (which has no block after the call) or
            // array methods (which aren't named GET/POST/etc).
            if(peek(1).type == TokType::Ident) {
                std::string maybeMethod = peek(1).text;
                static const std::vector<std::string> verbs = {"GET","POST","PUT","PATCH","DELETE"};
                bool isVerb = false;
                for(auto& v : verbs) if(v == maybeMethod) { isVerb = true; break; }
                if(isVerb && peek(2).type == TokType::LParen) {
                    advance(); // .
                    advance(); // METHOD
                    advance(); // (
                    auto route = std::make_shared<RouteDeclStmt>();
                    route->serverName = id.text;
                    route->method = maybeMethod;
                    route->pathExpr = parseExpression();
                    expect(TokType::RParen,")");
                    route->body = parseBlock();
                    if(consumeSemicolon) match(TokType::Semicolon); // trailing ';' after the block is optional but tolerated
                    return route;
                }
            }
            // server.start();
            if(peek(1).type == TokType::Ident && peek(1).text == "start" && peek(2).type == TokType::LParen) {
                advance(); advance(); advance(); // . start (
                expect(TokType::RParen,")");
                if(consumeSemicolon) expect(TokType::Semicolon,";");
                auto stmt = std::make_shared<ServerStartStmt>();
                stmt->serverName = id.text;
                return stmt;
            }
            // response.header(k,v) / response.cookie(k,v[,maxAge]) / response.redirect(url)
            // as bare statements (the common case: called for effect, result discarded).
            // Note: "header" lexes as the HeaderKw keyword token (reused from the
            // Http.GET(){header{...}} syntax), not a plain Ident, so it's checked
            // separately here.
            if(peek(1).type == TokType::Ident || peek(1).type == TokType::HeaderKw) {
                std::string maybeMethod = peek(1).text;
                static const std::vector<std::string> objMethods = {"header","cookie","redirect"};
                bool isObjMethod = false;
                for(auto& m : objMethods) if(m == maybeMethod) { isObjMethod = true; break; }
                if(isObjMethod && peek(2).type == TokType::LParen) {
                    advance(); advance(); advance(); // . method (
                    auto call = std::make_shared<ObjectMethodCallExpr>();
                    call->objectName = id.text;
                    call->methodName = maybeMethod;
                    if(!check(TokType::RParen)) {
                        while(true) {
                            call->args.push_back(parseExpression());
                            if(match(TokType::Comma)) continue;
                            break;
                        }
                    }
                    expect(TokType::RParen,")");
                    if(consumeSemicolon) expect(TokType::Semicolon,";");
                    auto stmt = std::make_shared<ExprStmt>();
                    stmt->expr = call;
                    return stmt;
                }
            }
            if(peek(1).type == TokType::JoinKw || peek(1).type == TokType::JoinAllKw) {
                auto base = std::make_shared<VarExpr>(id.text);
                ExprPtr fullExpr = base;
                advance(); // consume '.'
                if(match(TokType::JoinKw)) {
                    expect(TokType::LParen,"("); expect(TokType::RParen,")");
                    auto j = std::make_shared<JoinExpr>();
                    j->handleExpr = fullExpr;
                    fullExpr = j;
                } else {
                    advance(); // joinAll
                    expect(TokType::LParen,"("); expect(TokType::RParen,")");
                    auto j = std::make_shared<JoinAllExpr>();
                    j->arrayName = id.text;
                    fullExpr = j;
                }
                if(consumeSemicolon) expect(TokType::Semicolon,";");
                auto stmt = std::make_shared<ExprStmt>();
                stmt->expr = fullExpr;
                return stmt;
            }
            {
                ArrayMethod m;
                if(peek(1).type == TokType::Ident && tryArrayMethodName(peek(1).text, m) && peek(2).type == TokType::LParen) {
                    auto call = parseArrayMethodCall(id.text);
                    if(consumeSemicolon) expect(TokType::Semicolon,";");
                    auto stmt = std::make_shared<ExprStmt>();
                    stmt->expr = call;
                    return stmt;
                }
            }
            advance();
            auto stmt = std::make_shared<MemberAssignStmt>();
            stmt->objectName = id.text;
            stmt->member = expect(TokType::Ident,"member name").text;
            if(match(TokType::LBracket)) {
                stmt->index = parseExpression();
                expect(TokType::RBracket,"]");
            }
            expect(TokType::Assign,"=");
            stmt->value=parseExpression();
            if(consumeSemicolon) expect(TokType::Semicolon,";");
            return stmt;
        }
        if(match(TokType::LBracket)) {
            auto idx = parseExpression();
            expect(TokType::RBracket,"]");
            if(check(TokType::Dot) && peek(1).type == TokType::JoinKw) {
                advance(); advance(); // . join
                expect(TokType::LParen,"("); expect(TokType::RParen,")");
                if(consumeSemicolon) expect(TokType::Semicolon,";");
                auto j = std::make_shared<JoinExpr>();
                j->handleExpr = std::make_shared<ArrayAccessExpr>(id.text, idx);
                auto stmt = std::make_shared<ExprStmt>();
                stmt->expr = j;
                return stmt;
            }
            auto stmt= std::make_shared<ArrayAssignStmt>();
            stmt->arrayName=id.text;
            stmt->index=idx;
            expect(TokType::Assign,"=");
            stmt->value=parseExpression();
            if(consumeSemicolon) expect(TokType::Semicolon,";");
            return stmt;
        }
        auto stmt= std::make_shared<AssignStmt>();
        stmt->name=id.text;
        expect(TokType::Assign,"=");
        stmt->value=parseExpression();
        if(consumeSemicolon) expect(TokType::Semicolon,";");
        return stmt;
    }

    StmtPtr parseIf() {
        expect(TokType::If,"if");
        expect(TokType::LParen,"(");
        auto stmt = std::make_shared<IfStmt>();
        stmt->condition = parseExpression();
        expect(TokType::RParen,")");
        stmt->thenBlock = parseBlock();
        if(match(TokType::Else)) stmt->elseBlock = parseBlock();
        return stmt;
    }

    StmtPtr parseWhile() {
        expect(TokType::While,"while");
        expect(TokType::LParen,"(");
        auto stmt = std::make_shared<WhileStmt>();
        stmt->condition = parseExpression();
        expect(TokType::RParen,")");
        stmt->body = parseBlock();
        return stmt;
    }

    StmtPtr parseFor() {
        expect(TokType::For,"for");
        expect(TokType::LParen,"(");
        auto stmt = std::make_shared<ForStmt>();
        if(check(TokType::Var)) stmt->init = parseVarDecl(true);
        else stmt->init = parseAssignmentStatement(true);
        stmt->condition = parseExpression();
        expect(TokType::Semicolon,";");
        stmt->increment = parseAssignmentStatement(false);
        expect(TokType::RParen,")");
        stmt->body = parseBlock();
        return stmt;
    }

    StmtPtr parseReturn() {
        expect(TokType::Return,"return");
        auto stmt = std::make_shared<ReturnStmt>();
        if(!check(TokType::Semicolon)) stmt->value = parseExpression();
        expect(TokType::Semicolon,";");
        return stmt;
    }

    StmtPtr parseParallel() {
        expect(TokType::Parallel,"Parallel");
        auto stmt = std::make_shared<ParallelStmt>();
        stmt->blocks.push_back(parseBlock());
        while(check(TokType::LBrace)) stmt->blocks.push_back(parseBlock());
        if(stmt->blocks.size() < 2)
            throw std::runtime_error("'Parallel' expects at least two '{ }' blocks, line " + std::to_string(peek().line));
        return stmt;
    }

    StmtPtr parseStatement() {
        if(check(TokType::Var)) return parseVarDecl();
        if(check(TokType::Print)) return parsePrint();
        if(check(TokType::If)) return parseIf();
        if(check(TokType::While)) return parseWhile();
        if(check(TokType::For)) return parseFor();
        if(check(TokType::Return)) return parseReturn();
        if(check(TokType::Parallel)) return parseParallel();
        if(check(TokType::Ident)) return parseAssignmentStatement();
        throw std::runtime_error("Unexpected statement on line " + std::to_string(peek().line));
    }

    FunctionDecl parseFunctionDecl() {
        expect(TokType::Function,"function");
        FunctionDecl fn;
        fn.name = expect(TokType::Ident,"function name").text;
        expect(TokType::LParen,"(");
        if(!check(TokType::RParen)) {
            while(true) {
                expect(TokType::Var,"'var' before parameter name");
                fn.params.push_back(expect(TokType::Ident,"parameter name").text);
                if(match(TokType::Comma)) continue;
                break;
            }
        }
        expect(TokType::RParen,")");
        fn.body = parseBlock();
        return fn;
    }

    StructDecl parseStructDecl() {
        expect(TokType::StructKw,"Struct");
        StructDecl sd;
        sd.name = expect(TokType::Ident,"struct name").text;
        expect(TokType::LBrace,"{");
        bool ctorFound = false;
        while(!check(TokType::RBrace)) {
            if(check(TokType::End))
                throw std::runtime_error("Unexpected end of file inside struct '" + sd.name + "'");
            if(check(TokType::Var)) {
                advance();
                std::string fname = expect(TokType::Ident,"field name").text;
                bool isArr = false;
                if(match(TokType::LBracket)) { expect(TokType::RBracket,"]"); isArr = true; }
                expect(TokType::Semicolon,";");
                sd.fields.push_back({fname, isArr});
                continue;
            }
            if(check(TokType::Ident) && peek().text == sd.name) {
                advance();
                expect(TokType::LParen,"(");
                if(!check(TokType::RParen)) {
                    while(true) {
                        expect(TokType::Var,"'var' before parameter name");
                        sd.ctorParams.push_back(expect(TokType::Ident,"parameter name").text);
                        if(match(TokType::Comma)) continue;
                        break;
                    }
                }
                expect(TokType::RParen,")");
                sd.ctorBody = parseBlock();
                ctorFound = true;
                continue;
            }
            throw std::runtime_error("Unexpected token in struct '" + sd.name + "' on line " + std::to_string(peek().line));
        }
        expect(TokType::RBrace,"}");
        if(!ctorFound) sd.ctorBody = std::make_shared<BlockStmt>();
        return sd;
    }

    Program parseProgram() {
        Program prog;
        while(!check(TokType::End)) {
            if(check(TokType::Function)) {
                auto fn = parseFunctionDecl();
                prog.functions[fn.name] = fn;
                continue;
            }
            if(check(TokType::StructKw)) {
                auto sd = parseStructDecl();
                prog.structs[sd.name] = sd;
                continue;
            }
            prog.statements.push_back(parseStatement());
        }
        return prog;
    }
};

struct Value {
    bool isArray = false;
    bool isBool = false;
    bool isStruct = false;
    bool isThread = false;
    bool isString = false;
    bool isObject = false;
    bool isNull = false;
    double number = 0;
    bool boolean = false;
    std::string str;
    std::vector<double> array;
    std::string structType;
    std::shared_ptr<std::unordered_map<std::string, Value>> fields;
    std::shared_ptr<std::unordered_map<std::string, Value>> object;
    std::shared_ptr<std::vector<Value>> objectArray;
    bool isObjectArray = false;
    bool isServer = false;
    std::shared_ptr<struct ServerInstance> server;
    bool isLenientMap = false; // true for request.query/.params/.header/.cookie: missing key -> "" instead of throwing

    static Value makeString(std::string s) { Value v; v.isString = true; v.str = std::move(s); return v; }
    static Value makeNull() { Value v; v.isNull = true; return v; }
    static Value makeObject() { Value v; v.isObject = true; v.object = std::make_shared<std::unordered_map<std::string, Value>>(); return v; }
    static Value makeObjectArray() { Value v; v.isObject = true; v.isObjectArray = true; v.objectArray = std::make_shared<std::vector<Value>>(); return v; }
    static Value makeBool2(bool b) { Value v; v.isBool = true; v.boolean = b; return v; }
};

std::string valueToDisplayString(const Value& v) {
    if(v.isNull) return "null";
    if(v.isString) return v.str;
    if(v.isBool) return v.boolean ? "true" : "false";
    if(v.isObject) return v.isObjectArray ? "[array]" : "[object]";
    if(v.isStruct) return "[struct " + v.structType + "]";
    if(v.isArray) return "[array]";
    std::ostringstream oss;
    if(v.number == (long long)v.number) oss << (long long)v.number;
    else oss << v.number;
    return oss.str();
}

struct ReturnSignal { Value value; };

#include <thread>
#include <mutex>
#include <future>
#include <atomic>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <csignal>

// ============================================================================
// JSON support (parse + serialize) over the existing Value type.
// Value already has isObject/object/isObjectArray/objectArray/isNull/isString
// fields defined above specifically to support this.
// ============================================================================

struct JsonParser {
    const std::string& s;
    size_t pos = 0;
    JsonParser(const std::string& src) : s(src) {}

    char peek() { return pos < s.size() ? s[pos] : '\0'; }
    char advance() { return s[pos++]; }
    void skipWs() { while(pos < s.size() && std::isspace((unsigned char)s[pos])) pos++; }

    Value parse() {
        skipWs();
        Value v = parseValue();
        skipWs();
        return v;
    }

    Value parseValue() {
        skipWs();
        char c = peek();
        if(c == '{') return parseObject();
        if(c == '[') return parseArray();
        if(c == '"') return Value::makeString(parseString());
        if(c == 't' || c == 'f') return parseBool();
        if(c == 'n') return parseNull();
        if(c == '-' || std::isdigit((unsigned char)c)) return parseNumber();
        throw std::runtime_error("JSON parse error: unexpected character at position " + std::to_string(pos));
    }

    Value parseObject() {
        advance(); // {
        Value obj = Value::makeObject();
        skipWs();
        if(peek() == '}') { advance(); return obj; }
        while(true) {
            skipWs();
            if(peek() != '"') throw std::runtime_error("JSON parse error: expected string key at position " + std::to_string(pos));
            std::string key = parseString();
            skipWs();
            if(advance() != ':') throw std::runtime_error("JSON parse error: expected ':' at position " + std::to_string(pos));
            Value val = parseValue();
            (*obj.object)[key] = val;
            skipWs();
            char c = advance();
            if(c == ',') continue;
            if(c == '}') break;
            throw std::runtime_error("JSON parse error: expected ',' or '}' at position " + std::to_string(pos));
        }
        return obj;
    }

    Value parseArray() {
        advance(); // [
        Value arr = Value::makeObjectArray();
        skipWs();
        if(peek() == ']') { advance(); return arr; }
        while(true) {
            Value val = parseValue();
            arr.objectArray->push_back(val);
            skipWs();
            char c = advance();
            if(c == ',') continue;
            if(c == ']') break;
            throw std::runtime_error("JSON parse error: expected ',' or ']' at position " + std::to_string(pos));
        }
        return arr;
    }

    std::string parseString() {
        advance(); // opening quote
        std::string out;
        while(peek() != '"') {
            if(pos >= s.size()) throw std::runtime_error("JSON parse error: unterminated string");
            char c = advance();
            if(c == '\\') {
                char esc = advance();
                switch(esc) {
                    case '"': out += '"'; break;
                    case '\\': out += '\\'; break;
                    case '/': out += '/'; break;
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'u': {
                        std::string hex = s.substr(pos, 4);
                        pos += 4;
                        int code = std::stoi(hex, nullptr, 16);
                        if(code < 0x80) {
                            out += (char)code;
                        } else if(code < 0x800) {
                            out += (char)(0xC0 | (code >> 6));
                            out += (char)(0x80 | (code & 0x3F));
                        } else {
                            out += (char)(0xE0 | (code >> 12));
                            out += (char)(0x80 | ((code >> 6) & 0x3F));
                            out += (char)(0x80 | (code & 0x3F));
                        }
                        break;
                    }
                    default: out += esc; break;
                }
            } else {
                out += c;
            }
        }
        advance(); // closing quote
        return out;
    }

    Value parseBool() {
        if(s.compare(pos, 4, "true") == 0) { pos += 4; return Value::makeBool2(true); }
        if(s.compare(pos, 5, "false") == 0) { pos += 5; return Value::makeBool2(false); }
        throw std::runtime_error("JSON parse error: invalid literal at position " + std::to_string(pos));
    }

    Value parseNull() {
        if(s.compare(pos, 4, "null") == 0) { pos += 4; return Value::makeNull(); }
        throw std::runtime_error("JSON parse error: invalid literal at position " + std::to_string(pos));
    }

    Value parseNumber() {
        size_t start = pos;
        if(peek() == '-') advance();
        while(std::isdigit((unsigned char)peek())) advance();
        if(peek() == '.') {
            advance();
            while(std::isdigit((unsigned char)peek())) advance();
        }
        if(peek() == 'e' || peek() == 'E') {
            advance();
            if(peek() == '+' || peek() == '-') advance();
            while(std::isdigit((unsigned char)peek())) advance();
        }
        std::string numStr = s.substr(start, pos - start);
        Value v; v.number = std::stod(numStr); return v;
    }
};

Value parseJsonString(const std::string& src) {
    JsonParser p(src);
    return p.parse();
}

void jsonEscapeInto(std::ostringstream& oss, const std::string& s) {
    oss << '"';
    for(char c : s) {
        switch(c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\t': oss << "\\t"; break;
            case '\r': oss << "\\r"; break;
            default:
                if((unsigned char)c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned char)c);
                    oss << buf;
                } else {
                    oss << c;
                }
        }
    }
    oss << '"';
}

void serializeJsonInto(std::ostringstream& oss, const Value& v) {
    if(v.isNull) { oss << "null"; return; }
    if(v.isBool) { oss << (v.boolean ? "true" : "false"); return; }
    if(v.isString) { jsonEscapeInto(oss, v.str); return; }
    if(v.isStruct) {
        oss << '{';
        bool first = true;
        for(auto& kv : *v.fields) {
            if(!first) oss << ',';
            first = false;
            jsonEscapeInto(oss, kv.first);
            oss << ':';
            serializeJsonInto(oss, kv.second);
        }
        oss << '}';
        return;
    }
    if(v.isObject) {
        if(v.isObjectArray) {
            oss << '[';
            bool first = true;
            for(auto& item : *v.objectArray) {
                if(!first) oss << ',';
                first = false;
                serializeJsonInto(oss, item);
            }
            oss << ']';
        } else {
            oss << '{';
            bool first = true;
            for(auto& kv : *v.object) {
                if(!first) oss << ',';
                first = false;
                jsonEscapeInto(oss, kv.first);
                oss << ':';
                serializeJsonInto(oss, kv.second);
            }
            oss << '}';
        }
        return;
    }
    if(v.isArray) {
        oss << '[';
        bool first = true;
        for(double d : v.array) {
            if(!first) oss << ',';
            first = false;
            if(d == (long long)d) oss << (long long)d;
            else oss << d;
        }
        oss << ']';
        return;
    }
    if(v.number == (long long)v.number) oss << (long long)v.number;
    else oss << v.number;
}

std::string serializeJsonValue(const Value& v) {
    std::ostringstream oss;
    serializeJsonInto(oss, v);
    return oss.str();
}

// ============================================================================
// Minimal HTTP/1.1 client over raw POSIX sockets. No TLS: https:// URLs throw
// a clear error rather than silently degrading to plaintext.
// ============================================================================

struct HttpRawResponse {
    int status = 0;
    std::string statusText;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
};

struct ParsedUrl {
    std::string scheme;
    std::string host;
    std::string port;
    std::string path;
};

inline std::string httpToLower(std::string s) {
    for(auto& c : s) c = std::tolower((unsigned char)c);
    return s;
}

inline ParsedUrl parseHttpUrl(const std::string& url) {
    ParsedUrl out;
    size_t schemeEnd = url.find("://");
    if(schemeEnd == std::string::npos)
        throw std::runtime_error("Invalid URL (missing scheme, expected http://): " + url);
    out.scheme = url.substr(0, schemeEnd);
    size_t hostStart = schemeEnd + 3;
    size_t pathStart = url.find('/', hostStart);
    std::string hostport = (pathStart == std::string::npos)
        ? url.substr(hostStart)
        : url.substr(hostStart, pathStart - hostStart);
    out.path = (pathStart == std::string::npos) ? "/" : url.substr(pathStart);

    size_t colonPos = hostport.find(':');
    if(colonPos != std::string::npos) {
        out.host = hostport.substr(0, colonPos);
        out.port = hostport.substr(colonPos + 1);
    } else {
        out.host = hostport;
        out.port = (out.scheme == "https") ? "443" : "80";
    }
    return out;
}

inline int httpConnectTo(const std::string& host, const std::string& port) {
    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* res = nullptr;

    int rc = getaddrinfo(host.c_str(), port.c_str(), &hints, &res);
    if(rc != 0)
        throw std::runtime_error("DNS resolution failed for '" + host + "': " + gai_strerror(rc));

    int fd = -1;
    for(struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(fd < 0) continue;
        if(connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if(fd < 0)
        throw std::runtime_error("Could not connect to " + host + ":" + port);
    return fd;
}

inline void httpSendAll(int fd, const std::string& data) {
    size_t sent = 0;
    while(sent < data.size()) {
        ssize_t n = send(fd, data.data() + sent, data.size() - sent, 0);
        if(n <= 0) throw std::runtime_error("send() failed while writing HTTP request");
        sent += (size_t)n;
    }
}

inline std::string httpRecvAll(int fd) {
    std::string out;
    char buf[8192];
    while(true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if(n < 0) throw std::runtime_error("recv() failed while reading HTTP response");
        if(n == 0) break;
        out.append(buf, n);
    }
    return out;
}

inline std::string httpRecvBodyExact(int fd, std::string haveBody, size_t contentLength) {
    while(haveBody.size() < contentLength) {
        char buf[8192];
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if(n <= 0) break;
        haveBody.append(buf, n);
    }
    return haveBody;
}

inline std::string httpRecvChunkedBody(int fd, std::string buffered) {
    std::string result;
    while(true) {
        size_t lineEnd;
        while((lineEnd = buffered.find("\r\n")) == std::string::npos) {
            char buf[8192];
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if(n <= 0) return result;
            buffered.append(buf, n);
        }
        std::string sizeLine = buffered.substr(0, lineEnd);
        buffered.erase(0, lineEnd + 2);
        size_t chunkSize = std::stoul(sizeLine, nullptr, 16);
        if(chunkSize == 0) break;

        while(buffered.size() < chunkSize + 2) {
            char buf[8192];
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if(n <= 0) break;
            buffered.append(buf, n);
        }
        result += buffered.substr(0, chunkSize);
        buffered.erase(0, chunkSize + 2);
    }
    return result;
}

inline HttpRawResponse performHttpRequest(const std::string& method, const std::string& url,
                             const std::vector<std::pair<std::string,std::string>>& headers,
                             const std::string& body) {
    ParsedUrl u = parseHttpUrl(url);
    if(u.scheme == "https")
        throw std::runtime_error("HTTPS is not supported (no TLS library available). Use an http:// URL.");
    if(u.scheme != "http")
        throw std::runtime_error("Unsupported URL scheme '" + u.scheme + "' (only http:// is supported)");

    int fd = httpConnectTo(u.host, u.port);

    std::ostringstream req;
    req << method << " " << u.path << " HTTP/1.1\r\n";
    req << "Host: " << u.host << "\r\n";
    req << "Connection: close\r\n";
    bool hasContentType = false, hasUserAgent = false;
    for(auto& h : headers) {
        std::string lower = httpToLower(h.first);
        if(lower == "content-type") hasContentType = true;
        if(lower == "user-agent") hasUserAgent = true;
        req << h.first << ": " << h.second << "\r\n";
    }
    if(!hasUserAgent) req << "User-Agent: CnR-HttpClient/1.0\r\n";
    if(!body.empty()) {
        if(!hasContentType) req << "Content-Type: application/json\r\n";
        req << "Content-Length: " << body.size() << "\r\n";
    }
    req << "\r\n";
    req << body;

    try {
        httpSendAll(fd, req.str());

        std::string raw;
        char buf[8192];
        size_t headerEnd = std::string::npos;
        while(headerEnd == std::string::npos) {
            ssize_t n = recv(fd, buf, sizeof(buf), 0);
            if(n <= 0) break;
            raw.append(buf, n);
            headerEnd = raw.find("\r\n\r\n");
        }
        if(headerEnd == std::string::npos) {
            close(fd);
            throw std::runtime_error("Connection closed before HTTP headers were complete");
        }

        std::string headerBlock = raw.substr(0, headerEnd);
        std::string bodyStart = raw.substr(headerEnd + 4);

        HttpRawResponse resp;
        std::istringstream hs(headerBlock);
        std::string statusLine;
        std::getline(hs, statusLine);
        if(!statusLine.empty() && statusLine.back() == '\r') statusLine.pop_back();
        {
            std::istringstream sl(statusLine);
            std::string httpVer;
            sl >> httpVer >> resp.status;
            std::getline(sl, resp.statusText);
            if(!resp.statusText.empty() && resp.statusText[0] == ' ') resp.statusText.erase(0,1);
        }

        std::string line;
        while(std::getline(hs, line)) {
            if(!line.empty() && line.back() == '\r') line.pop_back();
            if(line.empty()) continue;
            size_t colon = line.find(':');
            if(colon == std::string::npos) continue;
            std::string key = line.substr(0, colon);
            size_t valStart = colon + 1;
            while(valStart < line.size() && line[valStart] == ' ') valStart++;
            std::string val = line.substr(valStart);
            resp.headers[key] = val;
        }

        std::string transferEncoding;
        std::string contentLengthStr;
        for(auto& kv : resp.headers) {
            if(httpToLower(kv.first) == "transfer-encoding") transferEncoding = httpToLower(kv.second);
            if(httpToLower(kv.first) == "content-length") contentLengthStr = kv.second;
        }

        if(transferEncoding.find("chunked") != std::string::npos) {
            resp.body = httpRecvChunkedBody(fd, bodyStart);
        } else if(!contentLengthStr.empty()) {
            size_t contentLength = std::stoul(contentLengthStr);
            resp.body = httpRecvBodyExact(fd, bodyStart, contentLength);
        } else {
            resp.body = bodyStart + httpRecvAll(fd);
        }

        close(fd);
        return resp;
    } catch(...) {
        close(fd);
        throw;
    }
}

struct ThreadRecord {
    std::thread worker;
    std::mutex mtx;
    bool finished = false;
    bool hasError = false;
    std::string errorMsg;
    Value result;
};

// ============================================================================
// HTTP server support: request parsing (method/path/query/headers/cookies/body),
// route matching against ":param" patterns, and the listening-socket loop.
// ============================================================================

struct ParsedServerRequest {
    std::string method;
    std::string fullPath;
    std::string path;
    std::unordered_map<std::string,std::string> query;
    std::unordered_map<std::string,std::string> headers;
    std::unordered_map<std::string,std::string> cookies;
    std::string body;
    std::string contentType;
    size_t contentLength = 0;
    std::string clientIp;
};

inline std::string serverUrlDecode(const std::string& s) {
    std::string out;
    for(size_t i=0;i<s.size();++i) {
        if(s[i]=='%' && i+2<s.size() && std::isxdigit((unsigned char)s[i+1]) && std::isxdigit((unsigned char)s[i+2])) {
            std::string hex = s.substr(i+1,2);
            out += (char)std::stoi(hex, nullptr, 16);
            i += 2;
        } else if(s[i]=='+') {
            out += ' ';
        } else {
            out += s[i];
        }
    }
    return out;
}

inline std::unordered_map<std::string,std::string> serverParseQueryString(const std::string& qs) {
    std::unordered_map<std::string,std::string> out;
    size_t pos = 0;
    while(pos < qs.size()) {
        size_t amp = qs.find('&', pos);
        std::string pair = (amp==std::string::npos) ? qs.substr(pos) : qs.substr(pos, amp-pos);
        size_t eq = pair.find('=');
        if(eq != std::string::npos) {
            out[serverUrlDecode(pair.substr(0,eq))] = serverUrlDecode(pair.substr(eq+1));
        } else if(!pair.empty()) {
            out[serverUrlDecode(pair)] = "";
        }
        if(amp == std::string::npos) break;
        pos = amp + 1;
    }
    return out;
}

inline std::unordered_map<std::string,std::string> serverParseCookieHeader(const std::string& val) {
    std::unordered_map<std::string,std::string> out;
    size_t pos = 0;
    while(pos < val.size()) {
        size_t semi = val.find(';', pos);
        std::string pair = (semi==std::string::npos) ? val.substr(pos) : val.substr(pos, semi-pos);
        size_t start = pair.find_first_not_of(' ');
        if(start != std::string::npos) pair = pair.substr(start);
        size_t eq = pair.find('=');
        if(eq != std::string::npos) out[pair.substr(0,eq)] = pair.substr(eq+1);
        if(semi == std::string::npos) break;
        pos = semi + 1;
    }
    return out;
}

inline ParsedServerRequest parseServerRequestHead(const std::string& head) {
    ParsedServerRequest req;
    std::istringstream hs(head);
    std::string requestLine;
    std::getline(hs, requestLine);
    if(!requestLine.empty() && requestLine.back()=='\r') requestLine.pop_back();

    std::istringstream rl(requestLine);
    std::string httpVer;
    rl >> req.method >> req.fullPath >> httpVer;

    size_t qpos = req.fullPath.find('?');
    if(qpos != std::string::npos) {
        req.path = serverUrlDecode(req.fullPath.substr(0, qpos));
        req.query = serverParseQueryString(req.fullPath.substr(qpos+1));
    } else {
        req.path = serverUrlDecode(req.fullPath);
    }

    std::string line;
    while(std::getline(hs, line)) {
        if(!line.empty() && line.back()=='\r') line.pop_back();
        if(line.empty()) continue;
        size_t colon = line.find(':');
        if(colon == std::string::npos) continue;
        std::string key = line.substr(0,colon);
        size_t vs = colon+1;
        while(vs<line.size() && line[vs]==' ') vs++;
        std::string val = line.substr(vs);
        req.headers[key] = val;
        std::string lowerKey = httpToLower(key);
        if(lowerKey == "content-type") req.contentType = val;
        if(lowerKey == "content-length") req.contentLength = std::stoul(val);
        if(lowerKey == "cookie") req.cookies = serverParseCookieHeader(val);
    }
    return req;
}

inline bool serverMatchRoute(const std::string& pattern, const std::string& path, std::unordered_map<std::string,std::string>& params) {
    std::vector<std::string> patternParts, pathParts;
    std::istringstream pss(pattern), pas(path);
    std::string seg;
    while(std::getline(pss, seg, '/')) if(!seg.empty()) patternParts.push_back(seg);
    while(std::getline(pas, seg, '/')) if(!seg.empty()) pathParts.push_back(seg);

    if(patternParts.size() != pathParts.size()) return false;
    for(size_t i=0;i<patternParts.size();++i) {
        if(!patternParts[i].empty() && patternParts[i][0]==':') {
            params[patternParts[i].substr(1)] = pathParts[i];
        } else if(patternParts[i] != pathParts[i]) {
            return false;
        }
    }
    return true;
}

// One registered route: method + path pattern + the CnR statement block to run.
struct RegisteredRoute {
    std::string method;
    std::string pathPattern;
    std::shared_ptr<BlockStmt> body;
};

// Runtime state for one `var server = Server() {...};` instance: its config
// values, registered routes (filled in by RouteDeclStmt execution), and the
// listening socket fd once start() is called. Held by shared_ptr so it can be
// captured into per-connection worker threads.
struct ServerInstance {
    std::unordered_map<std::string, Value> config;
    std::vector<RegisteredRoute> routes;
    int listenFd = -1;
    std::atomic<bool> running{false};

    std::string getStringConfig(const std::string& key, const std::string& def) const {
        auto it = config.find(key);
        if(it == config.end()) return def;
        return valueToDisplayString(it->second);
    }
    double getNumberConfig(const std::string& key, double def) const {
        auto it = config.find(key);
        if(it == config.end()) return def;
        if(it->second.isBool) return it->second.boolean ? 1.0 : 0.0;
        return it->second.number;
    }
    bool getBoolConfig(const std::string& key, bool def) const {
        auto it = config.find(key);
        if(it == config.end()) return def;
        if(it->second.isBool) return it->second.boolean;
        return it->second.number != 0;
    }
};

// Builds the `request` object exposed inside route bodies: request.method,
// request.path, request.ip, request.params.X, request.query.X,
// request.header["X"], request.cookie["X"], request.body, request.json,
// request.contentType, request.contentLength.
inline Value buildRequestValue(const ParsedServerRequest& req, const std::unordered_map<std::string,std::string>& params) {
    Value r = Value::makeObject();
    (*r.object)["method"] = Value::makeString(req.method);
    (*r.object)["path"] = Value::makeString(req.path);
    (*r.object)["ip"] = Value::makeString(req.clientIp);
    (*r.object)["contentType"] = Value::makeString(req.contentType);
    { Value cl; cl.number = (double)req.contentLength; (*r.object)["contentLength"] = cl; }
    (*r.object)["body"] = Value::makeString(req.body);

    Value paramsObj = Value::makeObject();
    paramsObj.isLenientMap = true;
    for(auto& kv : params) (*paramsObj.object)[kv.first] = Value::makeString(kv.second);
    (*r.object)["params"] = paramsObj;

    Value queryObj = Value::makeObject();
    queryObj.isLenientMap = true;
    for(auto& kv : req.query) (*queryObj.object)[kv.first] = Value::makeString(kv.second);
    (*r.object)["query"] = queryObj;

    Value headerObj = Value::makeObject();
    headerObj.isLenientMap = true;
    for(auto& kv : req.headers) (*headerObj.object)[kv.first] = Value::makeString(kv.second);
    (*r.object)["header"] = headerObj;

    Value cookieObj = Value::makeObject();
    cookieObj.isLenientMap = true;
    for(auto& kv : req.cookies) (*cookieObj.object)[kv.first] = Value::makeString(kv.second);
    (*r.object)["cookie"] = cookieObj;

    if(!req.body.empty()) {
        try {
            (*r.object)["json"] = parseJsonString(req.body);
        } catch(...) {
            (*r.object)["json"] = Value::makeNull();
        }
    } else {
        (*r.object)["json"] = Value::makeNull();
    }
    return r;
}

// Cookie the route body set via response.cookie(name, value[, maxAge]).
struct ServerCookie {
    std::string name;
    std::string value;
    bool hasMaxAge = false;
    long maxAge = 0;
};

// Mutable response state built up by a route body's `response.status = ...;`,
// `response.header(...)`, `response.cookie(...)`, `response.redirect(...)`
// calls, plus whatever the route body `return`s as the body.
struct ServerResponseState {
    int status = 200;
    std::vector<std::pair<std::string,std::string>> headers;
    std::vector<ServerCookie> cookies;
    std::string redirectTo;
    bool hasRedirect = false;
};

// Turns the CnR return value (string, object/JSON, or number) into a raw
// HTTP/1.1 response, applying whatever status/headers/cookies were staged
// on the `response` object during route execution.
inline std::string buildRawHttpResponse(const ServerResponseState& rs, const Value& returned, bool hadReturn, const std::string& serverName) {
    std::ostringstream out;

    int status = rs.hasRedirect ? 302 : rs.status;
    std::ostringstream body;
    bool jsonBody = false;
    if(rs.hasRedirect) {
        // Body is irrelevant for a redirect; browsers follow Location.
    } else if(hadReturn) {
        if(returned.isString) {
            body << returned.str;
        } else if(returned.isObject || returned.isStruct) {
            body << serializeJsonValue(returned);
            jsonBody = true;
        } else if(returned.isNull) {
            // no body
        } else {
            body << valueToDisplayString(returned);
        }
    }

    static const std::unordered_map<int,std::string> reasonPhrases = {
        {200,"OK"},{201,"Created"},{202,"Accepted"},{204,"No Content"},
        {301,"Moved Permanently"},{302,"Found"},{304,"Not Modified"},
        {400,"Bad Request"},{401,"Unauthorized"},{403,"Forbidden"},{404,"Not Found"},
        {405,"Method Not Allowed"},{409,"Conflict"},{422,"Unprocessable Entity"},
        {500,"Internal Server Error"},{502,"Bad Gateway"},{503,"Service Unavailable"}
    };
    std::string reason = "OK";
    { auto it = reasonPhrases.find(status); if(it != reasonPhrases.end()) reason = it->second; }

    out << "HTTP/1.1 " << status << " " << reason << "\r\n";

    bool hasContentType = false;
    for(auto& h : rs.headers) {
        if(httpToLower(h.first) == "content-type") hasContentType = true;
        out << h.first << ": " << h.second << "\r\n";
    }
    if(!hasContentType && !rs.hasRedirect) {
        out << "Content-Type: " << (jsonBody ? "application/json" : "text/plain") << "\r\n";
    }
    if(rs.hasRedirect) {
        out << "Location: " << rs.redirectTo << "\r\n";
    }
    for(auto& c : rs.cookies) {
        out << "Set-Cookie: " << c.name << "=" << c.value;
        if(c.hasMaxAge) out << "; Max-Age=" << c.maxAge;
        out << "; Path=/\r\n";
    }
    if(!serverName.empty()) out << "Server: " << serverName << "\r\n";
    out << "Connection: close\r\n";
    out << "Content-Length: " << body.str().size() << "\r\n";
    out << "\r\n";
    out << body.str();
    return out.str();
}

class Interpreter {
public:
    Interpreter(std::unordered_map<std::string, FunctionDecl> fns,
                std::unordered_map<std::string, StructDecl> strs)
        : functions(std::move(fns)), structs(std::move(strs)) {
        scopes.push_back({});
    }

    Interpreter(const Interpreter& parent, bool /*forThread*/)
        : functions(parent.functions), structs(parent.structs), threadRegistry(parent.threadRegistry) {
        scopes.push_back({});
    }

    std::vector<std::unordered_map<std::string, Value>> scopes;
    std::unordered_map<std::string, FunctionDecl> functions;
    std::unordered_map<std::string, StructDecl> structs;
    Value* currentSelf = nullptr;
    ServerResponseState* currentResponseState = nullptr; // set while executing a route body

    std::shared_ptr<std::vector<std::shared_ptr<ThreadRecord>>> threadRegistry =
        std::make_shared<std::vector<std::shared_ptr<ThreadRecord>>>();
    std::shared_ptr<std::mutex> registryMtx = std::make_shared<std::mutex>();

Value& lookupVar(const std::string& name) {
    for(auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto found = it->find(name);
        if(found != it->end()) return found->second;
    }
    throw std::runtime_error("Undefined variable '" + name + "'");
}

Value& resolveVar(const std::string& name) {
    if(currentSelf) {
        auto it = currentSelf->fields->find(name);
        if(it != currentSelf->fields->end()) return it->second;
    }
    return lookupVar(name);
}

double evalNumber(const ExprPtr& expr)
{
    if(auto n = std::dynamic_pointer_cast<NumberExpr>(expr)) return n->value;

    if(auto bo = std::dynamic_pointer_cast<BoolExpr>(expr)) return bo->value ? 1.0 : 0.0;

    if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
        Value& val = resolveVar(v->name);
        if(val.isArray)
            throw std::runtime_error("Cannot use array '" + v->name + "' as a number");
        if(val.isStruct)
            throw std::runtime_error("Cannot use struct '" + v->name + "' as a number");
        if(val.isBool) return val.boolean ? 1.0 : 0.0;
        return val.number;
    }

    if(auto u = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        if(u->op==TokType::Bang) return evalBool(u->expr) ? 0.0 : 1.0;
        double x = evalNumber(u->expr);
        if(u->op==TokType::Minus) return -x;
        return x;
    }

    if(auto cc = std::dynamic_pointer_cast<CharCastExpr>(expr)) {
        return evalNumber(cc->expr);
    }

    if(auto b = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        if(b->op==TokType::AndAnd) return (evalBool(b->left) && evalBool(b->right)) ? 1.0 : 0.0;
        if(b->op==TokType::OrOr) return (evalBool(b->left) || evalBool(b->right)) ? 1.0 : 0.0;

        double L = evalNumber(b->left);
        double R = evalNumber(b->right);
        switch(b->op) {
        case TokType::Plus: return L+R;
        case TokType::Minus: return L-R;
        case TokType::Star: return L*R;
        case TokType::Slash:
            if(R==0) throw std::runtime_error("Division by zero");
            return L/R;
        case TokType::Percent:
            if((int)R==0) throw std::runtime_error("Modulo by zero");
            return (int)L%(int)R;
        case TokType::Less: return L<R ? 1.0 : 0.0;
        case TokType::Greater: return L>R ? 1.0 : 0.0;
        case TokType::LessEqual: return L<=R ? 1.0 : 0.0;
        case TokType::GreaterEqual: return L>=R ? 1.0 : 0.0;
        case TokType::EqualEqual: return L==R ? 1.0 : 0.0;
        case TokType::BangEqual: return L!=R ? 1.0 : 0.0;
        default: break;
        }
    }

    if(auto a = std::dynamic_pointer_cast<ArrayAccessExpr>(expr)) {
        if(!a->index)
            throw std::runtime_error("Array '" + a->arrayName + "' used without an index in this context");
        Value& val = resolveVar(a->arrayName);
        if(!val.isArray)
            throw std::runtime_error("'" + a->arrayName + "' is not an array");
        int index = (int)evalNumber(a->index);
        if(index < 0 || index >= (int)val.array.size())
            throw std::runtime_error("Array index out of bounds for '" + a->arrayName + "': " + std::to_string(index));
        return val.array[index];
    }

    if(auto l = std::dynamic_pointer_cast<LenExpr>(expr)) {
        Value& val = resolveVar(l->arrayName);
        if(!val.isArray)
            throw std::runtime_error("'" + l->arrayName + "' is not an array");
        return (double)val.array.size();
    }

    if(auto m = std::dynamic_pointer_cast<MemberAccessExpr>(expr)) {
        Value v = evalMemberAccess(m);
        if(v.isArray) throw std::runtime_error("Cannot use array field '" + m->member + "' as a number");
        if(v.isStruct) throw std::runtime_error("Cannot use struct field '" + m->member + "' as a number");
        if(v.isObject) throw std::runtime_error("Cannot use object field '" + m->member + "' as a number");
        if(v.isString) throw std::runtime_error("Cannot use string field '" + m->member + "' as a number (use (string) cast or print it directly)");
        if(v.isNull) throw std::runtime_error("Field '" + m->member + "' is null");
        return v.isBool ? (v.boolean ? 1.0 : 0.0) : v.number;
    }

    if(auto c = std::dynamic_pointer_cast<CallExpr>(expr)) {
        Value v = callCallable(c->name, c->args);
        if(v.isArray) throw std::runtime_error("Cannot use array result of '" + c->name + "' as a number");
        if(v.isStruct) throw std::runtime_error("Cannot use struct result of '" + c->name + "' as a number");
        return v.isBool ? (v.boolean ? 1.0 : 0.0) : v.number;
    }

    if(auto t = std::dynamic_pointer_cast<ThreadExpr>(expr)) {
        Value v = spawnThread(t);
        return v.number;
    }

    if(auto j = std::dynamic_pointer_cast<JoinExpr>(expr)) {
        Value v = joinThread(j);
        if(v.isArray) throw std::runtime_error("Cannot use array result of join() as a number");
        if(v.isStruct) throw std::runtime_error("Cannot use struct result of join() as a number");
        return v.isBool ? (v.boolean ? 1.0 : 0.0) : v.number;
    }

    if(auto am = std::dynamic_pointer_cast<ArrayMethodCallExpr>(expr)) {
        return callArrayMethod(am);
    }

    throw std::runtime_error("Cannot evaluate numeric expression.");
}
bool evalBool(const ExprPtr& expr)
{
    if(auto b = std::dynamic_pointer_cast<BoolExpr>(expr)) return b->value;

    if(auto u = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        if(u->op==TokType::Bang) return !evalBool(u->expr);
    }

    if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
        Value& val = resolveVar(v->name);
        if(val.isBool) return val.boolean;
        return val.number != 0;
    }

    if(auto op = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        switch(op->op) {
        case TokType::AndAnd: return evalBool(op->left) && evalBool(op->right);
        case TokType::OrOr: return evalBool(op->left) || evalBool(op->right);
        case TokType::Less:
        case TokType::Greater:
        case TokType::LessEqual:
        case TokType::GreaterEqual:
        case TokType::EqualEqual:
        case TokType::BangEqual: {
            double L = evalNumber(op->left);
            double R = evalNumber(op->right);
            switch(op->op) {
            case TokType::Less: return L<R;
            case TokType::Greater: return L>R;
            case TokType::LessEqual: return L<=R;
            case TokType::GreaterEqual: return L>=R;
            case TokType::EqualEqual: return L==R;
            case TokType::BangEqual: return L!=R;
            default: break;
            }
        }
        default: break;
        }
    }

    return evalNumber(expr)!=0;
}

Value evalToValue(const ExprPtr& expr) {
    if(auto c = std::dynamic_pointer_cast<CallExpr>(expr)) return callCallable(c->name, c->args);
    if(auto m = std::dynamic_pointer_cast<MemberAccessExpr>(expr)) return evalMemberAccess(m);
    if(auto b = std::dynamic_pointer_cast<BoolExpr>(expr)) { Value v; v.isBool=true; v.boolean=b->value; return v; }
    if(auto t = std::dynamic_pointer_cast<ThreadExpr>(expr)) return spawnThread(t);
    if(auto j = std::dynamic_pointer_cast<JoinExpr>(expr)) return joinThread(j);
    if(auto ja = std::dynamic_pointer_cast<JoinAllExpr>(expr)) return joinAllThreads(ja);
    if(auto am = std::dynamic_pointer_cast<ArrayMethodCallExpr>(expr)) { Value v; v.number = callArrayMethod(am); return v; }
    if(auto sl = std::dynamic_pointer_cast<StringLitExpr>(expr)) return Value::makeString(sl->value);
    if(auto sc = std::dynamic_pointer_cast<StringCastExpr>(expr)) return Value::makeString(valueToDisplayString(evalToValue(sc->expr)));
    if(auto jo = std::dynamic_pointer_cast<JsonObjectLitExpr>(expr)) {
        Value obj = Value::makeObject();
        for(auto& kv : jo->entries) {
            std::string key = valueToDisplayString(evalToValue(kv.first));
            (*obj.object)[key] = evalToValue(kv.second);
        }
        return obj;
    }
    if(auto hc = std::dynamic_pointer_cast<HttpCallExpr>(expr)) return performHttpCall(hc);
    if(auto sc = std::dynamic_pointer_cast<ServerConfigExpr>(expr)) {
        Value v;
        v.isServer = true;
        v.server = std::make_shared<ServerInstance>();
        for(auto& kv : sc->entries) v.server->config[kv.first] = evalToValue(kv.second);
        return v;
    }
    if(auto omc = std::dynamic_pointer_cast<ObjectMethodCallExpr>(expr)) return callObjectMethod(omc);
    if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
        Value& val = resolveVar(v->name);
        return val;
    }
    Value v; v.number = evalNumber(expr); return v;
}

double callArrayMethod(const std::shared_ptr<ArrayMethodCallExpr>& am) {
    Value& val = resolveVar(am->arrayName);
    if(!val.isArray)
        throw std::runtime_error("'" + am->arrayName + "' is not an array");
    switch(am->method) {
    case ArrayMethod::Push: {
        double x = evalNumber(am->arg);
        val.array.push_back(x);
        return (double)val.array.size();
    }
    case ArrayMethod::Pop: {
        if(val.array.empty())
            throw std::runtime_error("Cannot pop() from empty array '" + am->arrayName + "'");
        double back = val.array.back();
        val.array.pop_back();
        return back;
    }
    case ArrayMethod::Sort: {
        std::sort(val.array.begin(), val.array.end());
        return (double)val.array.size();
    }
    case ArrayMethod::Reverse: {
        std::reverse(val.array.begin(), val.array.end());
        return (double)val.array.size();
    }
    case ArrayMethod::Contains: {
        double target = evalNumber(am->arg);
        for(double x : val.array) if(x==target) return 1.0;
        return 0.0;
    }
    case ArrayMethod::IndexOf: {
        double target = evalNumber(am->arg);
        for(size_t i=0;i<val.array.size();++i) if(val.array[i]==target) return (double)i;
        return -1.0;
    }
    case ArrayMethod::Accumulate: {
        double sum = 0;
        for(double x : val.array) sum += x;
        return sum;
    }
    }
    throw std::runtime_error("Unknown array method.");
}

// response.header(name, value) / response.cookie(name, value[, maxAge]) /
// response.redirect(url) -- only meaningful while executing a route body
// (currentResponseState is set by runServerRoute for the duration).
Value callObjectMethod(const std::shared_ptr<ObjectMethodCallExpr>& omc) {
    if(!currentResponseState)
        throw std::runtime_error("'" + omc->objectName + "." + omc->methodName + "()' can only be called inside a route handler");

    if(omc->methodName == "header") {
        if(omc->args.size() != 2)
            throw std::runtime_error("response.header() expects 2 arguments (name, value)");
        std::string name = valueToDisplayString(evalToValue(omc->args[0]));
        std::string val = valueToDisplayString(evalToValue(omc->args[1]));
        currentResponseState->headers.push_back({name, val});
        return Value::makeNull();
    }
    if(omc->methodName == "cookie") {
        if(omc->args.size() < 2 || omc->args.size() > 3)
            throw std::runtime_error("response.cookie() expects 2 or 3 arguments (name, value[, maxAge])");
        ServerCookie c;
        c.name = valueToDisplayString(evalToValue(omc->args[0]));
        c.value = valueToDisplayString(evalToValue(omc->args[1]));
        if(omc->args.size() == 3) {
            c.hasMaxAge = true;
            c.maxAge = (long)evalNumber(omc->args[2]);
        }
        currentResponseState->cookies.push_back(c);
        return Value::makeNull();
    }
    if(omc->methodName == "redirect") {
        if(omc->args.size() != 1)
            throw std::runtime_error("response.redirect() expects 1 argument (url)");
        currentResponseState->redirectTo = valueToDisplayString(evalToValue(omc->args[0]));
        currentResponseState->hasRedirect = true;
        return Value::makeNull();
    }
    throw std::runtime_error("Unknown response method '" + omc->methodName + "'");
}

// Http.METHOD(url) { header {...} body {...} } -- evaluates the URL/header/body
// expressions in the current scope, performs the real HTTP request over a
// POSIX socket, and returns a Value object with fields:
//   .status  (number)      .statusText (string)      .body (string, raw)
//   .json    (object, only meaningfully usable if the body was valid JSON --
//             parsing is attempted eagerly and stored; if parsing fails, .json
//             is a Value holding a string of the parse error instead)
//   .header["Name"] is not modeled as an indexable object here; instead each
//             response header is exposed as a field on .headers (object).
Value performHttpCall(const std::shared_ptr<HttpCallExpr>& hc) {
    std::string method = hc->method;
    for(auto& c : method) c = std::toupper((unsigned char)c);
    if(method != "GET" && method != "POST" && method != "PUT" && method != "PATCH" && method != "DELETE")
        throw std::runtime_error("Http: unsupported method '" + hc->method + "' (expected GET/POST/PUT/PATCH/DELETE)");

    std::string url = valueToDisplayString(evalToValue(hc->url));

    std::vector<std::pair<std::string,std::string>> headers;
    for(auto& kv : hc->headers) {
        std::string key = valueToDisplayString(evalToValue(kv.first));
        std::string val = valueToDisplayString(evalToValue(kv.second));
        headers.push_back({key, val});
    }

    std::string bodyStr;
    if(hc->bodyExpr) {
        Value bodyVal = evalToValue(hc->bodyExpr);
        bodyStr = serializeJsonValue(bodyVal);
    }

    HttpRawResponse resp = performHttpRequest(method, url, headers, bodyStr);

    Value result = Value::makeObject();
    Value statusVal; statusVal.number = (double)resp.status;
    (*result.object)["status"] = statusVal;
    (*result.object)["statusText"] = Value::makeString(resp.statusText);
    (*result.object)["body"] = Value::makeString(resp.body);

    Value headersObj = Value::makeObject();
    for(auto& kv : resp.headers) (*headersObj.object)[kv.first] = Value::makeString(kv.second);
    (*result.object)["headers"] = headersObj;

    // Best-effort JSON parse of the body for `.json` access; on failure,
    // `.json` becomes null rather than throwing, since not every response is JSON.
    try {
        if(!resp.body.empty()) {
            Value parsed = parseJsonString(resp.body);
            (*result.object)["json"] = parsed;
        } else {
            (*result.object)["json"] = Value::makeNull();
        }
    } catch(...) {
        (*result.object)["json"] = Value::makeNull();
    }

    return result;
}

Value evalMemberAccess(const std::shared_ptr<MemberAccessExpr>& m) {
    Value obj = evalToValue(m->base);

    if(obj.isStruct) {
        auto it = obj.fields->find(m->member);
        if(it == obj.fields->end())
            throw std::runtime_error("Struct '" + obj.structType + "' has no field '" + m->member + "'");
        Value field = it->second;
        if(m->index) {
            if(!field.isArray)
                throw std::runtime_error("Field '" + m->member + "' is not an array");
            int idx = (int)evalNumber(m->index);
            if(idx < 0 || idx >= (int)field.array.size())
                throw std::runtime_error("Array index out of bounds for field '" + m->member + "': " + std::to_string(idx));
            Value r; r.number = field.array[idx];
            return r;
        }
        return field;
    }

    if(obj.isObject) {
        if(obj.isObjectArray)
            throw std::runtime_error("Cannot access member '" + m->member + "' on a JSON array (index it with [i] instead)");
        auto it = obj.object->find(m->member);
        if(it == obj.object->end()) {
            if(obj.isLenientMap) return Value::makeString(""); // e.g. request.query.missingKey -> ""
            throw std::runtime_error("Object has no field '" + m->member + "'");
        }
        Value field = it->second;
        if(m->index) {
            if(field.isObject && field.isObjectArray) {
                int idx = (int)evalNumber(m->index);
                if(idx < 0 || idx >= (int)field.objectArray->size())
                    throw std::runtime_error("Array index out of bounds for field '" + m->member + "': " + std::to_string(idx));
                return (*field.objectArray)[idx];
            }
            if(field.isObject) {
                // String-keyed lookup, e.g. request.header["Authorization"] or
                // request.cookie["session"] -- field is itself a map, and the
                // index expression is a string key rather than a numeric position.
                Value keyVal = evalToValue(m->index);
                std::string key = valueToDisplayString(keyVal);
                auto fit = field.object->find(key);
                if(fit == field.object->end()) return Value::makeString(""); // absent header/cookie -> empty string
                return fit->second;
            }
            if(field.isArray) {
                int idx = (int)evalNumber(m->index);
                if(idx < 0 || idx >= (int)field.array.size())
                    throw std::runtime_error("Array index out of bounds for field '" + m->member + "': " + std::to_string(idx));
                Value r; r.number = field.array[idx];
                return r;
            }
            throw std::runtime_error("Field '" + m->member + "' is not indexable");
        }
        return field;
    }

    throw std::runtime_error("Cannot access member '" + m->member + "': base value is not a struct or object");
}

Value spawnThread(const std::shared_ptr<ThreadExpr>& t) {
    auto fIt = functions.find(t->fnName);
    if(fIt == functions.end())
        throw std::runtime_error("thread(): undefined function '" + t->fnName + "'");
    const FunctionDecl& fn = fIt->second;
    if(t->args.size() != fn.params.size())
        throw std::runtime_error("thread(): function '" + t->fnName + "' expects " +
                                  std::to_string(fn.params.size()) + " argument(s), got " +
                                  std::to_string(t->args.size()));

    std::vector<Value> argVals;
    argVals.reserve(t->args.size());
    for(auto& a : t->args) argVals.push_back(evalToValue(a));

    auto record = std::make_shared<ThreadRecord>();
    size_t handleId;
    {
        std::lock_guard<std::mutex> lock(*registryMtx);
        threadRegistry->push_back(record);
        handleId = threadRegistry->size() - 1;
    }

    FunctionDecl fnCopy = fn;
    std::string fnName = t->fnName;
    auto functionsCopy = functions;
    auto structsCopy = structs;
    auto reg = threadRegistry;
    auto regMtx = registryMtx;

    record->worker = std::thread([record, fnCopy, functionsCopy, structsCopy, reg, regMtx, argVals]() mutable {
        Interpreter worker(functionsCopy, structsCopy);
        worker.threadRegistry = reg;
        worker.registryMtx = regMtx;
        Value result;
        bool errored = false;
        std::string errMsg;
        try {
            result = worker.callFunction(fnCopy, const_cast<std::vector<Value>&>(argVals));
        } catch(ReturnSignal& r) {
            result = r.value;
        } catch(const std::exception& e) {
            errored = true;
            errMsg = e.what();
        } catch(...) {
            errored = true;
            errMsg = "unknown error in thread";
        }
        std::lock_guard<std::mutex> lock(record->mtx);
        record->finished = true;
        record->hasError = errored;
        record->errorMsg = errMsg;
        record->result = result;
    });

    Value handle;
    handle.isThread = true;
    handle.number = (double)handleId;
    return handle;
}

size_t resolveThreadHandle(const ExprPtr& handleExpr) {
    double idNum;
    if(std::dynamic_pointer_cast<ArrayAccessExpr>(handleExpr)) {
        idNum = evalNumber(handleExpr);
    } else {
        Value v = evalToValue(handleExpr);
        if(!v.isThread)
            throw std::runtime_error("join() called on a value that is not a thread handle");
        idNum = v.number;
    }
    size_t id = (size_t)idNum;
    std::lock_guard<std::mutex> lock(*registryMtx);
    if(id >= threadRegistry->size())
        throw std::runtime_error("join() called with an invalid thread handle");
    return id;
}

Value joinThread(const std::shared_ptr<JoinExpr>& j) {
    size_t id = resolveThreadHandle(j->handleExpr);
    std::shared_ptr<ThreadRecord> record;
    {
        std::lock_guard<std::mutex> lock(*registryMtx);
        record = (*threadRegistry)[id];
    }
    if(record->worker.joinable()) record->worker.join();
    std::lock_guard<std::mutex> lock(record->mtx);
    if(record->hasError)
        throw std::runtime_error("Thread error: " + record->errorMsg);
    return record->result;
}

Value joinAllThreads(const std::shared_ptr<JoinAllExpr>& ja) {
    Value& arr = resolveVar(ja->arrayName);
    if(!arr.isArray)
        throw std::runtime_error("joinAll() called on '" + ja->arrayName + "', which is not an array");
    Value results;
    results.isArray = true;
    for(double idVal : arr.array) {
        size_t id = (size_t)idVal;
        std::shared_ptr<ThreadRecord> record;
        {
            std::lock_guard<std::mutex> lock(*registryMtx);
            if(id >= threadRegistry->size())
                throw std::runtime_error("joinAll(): invalid thread handle in '" + ja->arrayName + "'");
            record = (*threadRegistry)[id];
        }
        if(record->worker.joinable()) record->worker.join();
        std::lock_guard<std::mutex> lock(record->mtx);
        if(record->hasError)
            throw std::runtime_error("Thread error: " + record->errorMsg);
        double num = record->result.isBool ? (record->result.boolean ? 1.0 : 0.0) : record->result.number;
        results.array.push_back(num);
    }
    return results;
}

Value callCallable(const std::string& name, const std::vector<ExprPtr>& argExprs) {
    std::vector<Value> argVals;
    argVals.reserve(argExprs.size());
    for(auto& a : argExprs) argVals.push_back(evalToValue(a));

    auto sIt = structs.find(name);
    if(sIt != structs.end()) return instantiateStruct(sIt->second, argVals);

    auto fIt = functions.find(name);
    if(fIt != functions.end()) return callFunction(fIt->second, argVals);

    throw std::runtime_error("Undefined function or struct '" + name + "'");
}

Value callFunction(const FunctionDecl& fn, std::vector<Value>& args) {
    if(args.size() != fn.params.size())
        throw std::runtime_error("Function '" + fn.name + "' expects " + std::to_string(fn.params.size()) +
                                  " argument(s), got " + std::to_string(args.size()));
    scopes.push_back({});
    for(size_t i=0;i<fn.params.size();++i) scopes.back()[fn.params[i]] = args[i];

    Value* prevSelf = currentSelf;
    currentSelf = nullptr;
    Value result;
    try {
        executeBlock(fn.body);
    } catch(ReturnSignal& r) {
        result = r.value;
        scopes.pop_back();
        currentSelf = prevSelf;
        return result;
    } catch(...) {
        scopes.pop_back();
        currentSelf = prevSelf;
        throw;
    }
    scopes.pop_back();
    currentSelf = prevSelf;
    return result;
}

Value instantiateStruct(const StructDecl& sd, std::vector<Value>& args) {
    if(args.size() != sd.ctorParams.size())
        throw std::runtime_error("Struct '" + sd.name + "' constructor expects " + std::to_string(sd.ctorParams.size()) +
                                  " argument(s), got " + std::to_string(args.size()));
    Value instance;
    instance.isStruct = true;
    instance.structType = sd.name;
    instance.fields = std::make_shared<std::unordered_map<std::string, Value>>();
    for(auto& f : sd.fields) {
        Value fv;
        fv.isArray = f.isArray;
        (*instance.fields)[f.name] = fv;
    }

    scopes.push_back({});
    for(size_t i=0;i<sd.ctorParams.size();++i) scopes.back()[sd.ctorParams[i]] = args[i];
    Value* prevSelf = currentSelf;
    currentSelf = &instance;
    try {
        executeBlock(sd.ctorBody);
    } catch(ReturnSignal&) {
    } catch(...) {
        scopes.pop_back();
        currentSelf = prevSelf;
        throw;
    }
    scopes.pop_back();
    currentSelf = prevSelf;
    return instance;
}

void executeBlock(const std::shared_ptr<BlockStmt>& block) {
    for(auto& stmt : block->statements) execute(stmt);
}

// Executes one matched route's body in a fresh scope with `request` and
// `response` bound as ordinary object variables. Returns the raw HTTP
// response bytes to write back to the client socket.
std::string runServerRoute(const RegisteredRoute& route, const ParsedServerRequest& req,
                            const std::unordered_map<std::string,std::string>& params,
                            const std::string& serverName) {
    ServerResponseState rs;
    ServerResponseState* prevRs = currentResponseState;
    currentResponseState = &rs;

    scopes.push_back({});
    scopes.back()["request"] = buildRequestValue(req, params);
    scopes.back()["response"] = Value::makeObject();
    (*scopes.back()["response"].object)["status"] = [](){ Value v; v.number = 200; return v; }();

    Value returned;
    bool hadReturn = false;
    try {
        executeBlock(route.body);
    } catch(ReturnSignal& r) {
        returned = r.value;
        hadReturn = true;
    } catch(const std::exception& e) {
        scopes.pop_back();
        currentResponseState = prevRs;
        ServerResponseState errRs;
        errRs.status = 500;
        Value errBody = Value::makeObject();
        (*errBody.object)["error"] = Value::makeString(e.what());
        return buildRawHttpResponse(errRs, errBody, true, serverName);
    }

    // response.status = N; is stored as a plain field on the `response`
    // object; pull it out into the response state before serializing.
    Value& responseVal = scopes.back()["response"];
    if(responseVal.isObject) {
        auto it = responseVal.object->find("status");
        if(it != responseVal.object->end() && !it->second.isNull) {
            rs.status = it->second.isBool ? (it->second.boolean ? 1 : 0) : (int)it->second.number;
        }
    }

    scopes.pop_back();
    currentResponseState = prevRs;
    return buildRawHttpResponse(rs, returned, hadReturn, serverName);
}

// Handles a single accepted client connection: reads the request head (and
// body, if Content-Length/chunked), matches it against the server's
// registered routes, runs the matching route body, and writes the response.
void handleServerConnection(int clientFd, std::shared_ptr<ServerInstance> server, std::string clientIp) {
    std::string buffered;
    char buf[8192];
    size_t headerEnd = std::string::npos;
    while(headerEnd == std::string::npos) {
        ssize_t n = recv(clientFd, buf, sizeof(buf), 0);
        if(n <= 0) { close(clientFd); return; }
        buffered.append(buf, n);
        headerEnd = buffered.find("\r\n\r\n");
        if(buffered.size() > 1024*1024 && headerEnd == std::string::npos) {
            close(clientFd); // headers too large, bail out
            return;
        }
    }

    std::string headBlock = buffered.substr(0, headerEnd);
    std::string bodyStart = buffered.substr(headerEnd + 4);

    ParsedServerRequest req;
    try {
        req = parseServerRequestHead(headBlock);
    } catch(...) {
        close(clientFd);
        return;
    }
    req.clientIp = clientIp;

    if(req.contentLength > 0) {
        req.body = httpRecvBodyExact(clientFd, bodyStart, req.contentLength);
    } else {
        req.body = bodyStart;
    }

    const RegisteredRoute* matched = nullptr;
    std::unordered_map<std::string,std::string> params;
    bool pathExistsForOtherMethod = false;
    for(auto& route : server->routes) {
        std::unordered_map<std::string,std::string> p;
        if(serverMatchRoute(route.pathPattern, req.path, p)) {
            if(route.method == req.method) {
                matched = &route;
                params = p;
                break;
            }
            pathExistsForOtherMethod = true;
        }
    }

    std::string responseBytes;
    std::string serverName = server->getStringConfig("serverName", "");
    if(matched) {
        responseBytes = runServerRoute(*matched, req, params, serverName);
    } else {
        ServerResponseState rs;
        rs.status = pathExistsForOtherMethod ? 405 : 404;
        Value body = Value::makeObject();
        (*body.object)["error"] = Value::makeString(pathExistsForOtherMethod ? "Method Not Allowed" : "Not Found");
        responseBytes = buildRawHttpResponse(rs, body, true, serverName);
    }

    size_t sent = 0;
    while(sent < responseBytes.size()) {
        ssize_t n = send(clientFd, responseBytes.data() + sent, responseBytes.size() - sent, 0);
        if(n <= 0) break;
        sent += (size_t)n;
    }
    close(clientFd);
}

// server.start(); -- binds, listens, and accepts connections in a loop.
// Each connection is dispatched to its own std::thread (bounded informally
// by the `threads`/`parallel` config, though std::thread is used per-request
// here rather than a fixed-size pool for simplicity). Blocking call: never
// returns under normal operation.
void runServerLoop(std::shared_ptr<ServerInstance> server) {
    std::string host = server->getStringConfig("host", "0.0.0.0");
    int port = (int)server->getNumberConfig("port", 8080);
    int backlog = (int)server->getNumberConfig("backlog", 128);
    bool logRequests = server->getBoolConfig("logRequests", false);
    std::string serverName = server->getStringConfig("serverName", "CnR");

    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo* res = nullptr;
    std::string portStr = std::to_string(port);
    int rc = getaddrinfo(host == "0.0.0.0" ? nullptr : host.c_str(), portStr.c_str(), &hints, &res);
    if(rc != 0)
        throw std::runtime_error("Server: failed to resolve bind address: " + std::string(gai_strerror(rc)));

    int listenFd = -1;
    for(struct addrinfo* p = res; p != nullptr; p = p->ai_next) {
        listenFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(listenFd < 0) continue;
        int opt = 1;
        setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        if(bind(listenFd, p->ai_addr, p->ai_addrlen) == 0) break;
        close(listenFd);
        listenFd = -1;
    }
    freeaddrinfo(res);
    if(listenFd < 0)
        throw std::runtime_error("Server: could not bind to " + host + ":" + portStr);

    if(listen(listenFd, backlog) != 0) {
        close(listenFd);
        throw std::runtime_error("Server: listen() failed");
    }

    server->listenFd = listenFd;
    server->running = true;

    if(logRequests) {
        std::cout << "[" << serverName << "] listening on " << host << ":" << port << std::endl;
    }

    while(server->running) {
        struct sockaddr_storage clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);
        if(clientFd < 0) {
            if(!server->running) break;
            continue;
        }

        char ipBuf[INET6_ADDRSTRLEN] = {0};
        std::string clientIp;
        if(clientAddr.ss_family == AF_INET) {
            auto* s4 = (struct sockaddr_in*)&clientAddr;
            inet_ntop(AF_INET, &s4->sin_addr, ipBuf, sizeof(ipBuf));
            clientIp = ipBuf;
        } else if(clientAddr.ss_family == AF_INET6) {
            auto* s6 = (struct sockaddr_in6*)&clientAddr;
            inet_ntop(AF_INET6, &s6->sin6_addr, ipBuf, sizeof(ipBuf));
            clientIp = ipBuf;
        }

        auto functionsCopy = functions;
        auto structsCopy = structs;

        // Each connection gets its own Interpreter (fresh scopes), sharing
        // only the function/struct declarations and the server's route table.
        std::thread worker([clientFd, server, clientIp, functionsCopy, structsCopy]() mutable {
            Interpreter connInterp(functionsCopy, structsCopy);
            try {
                connInterp.handleServerConnection(clientFd, server, clientIp);
            } catch(...) {
                close(clientFd);
            }
        });
        worker.detach();
    }
    close(listenFd);
}

void execute(const StmtPtr& stmt)
{
    if(auto s = std::dynamic_pointer_cast<ParallelStmt>(stmt)) {
        auto functionsCopy = functions;
        auto structsCopy = structs;
        auto reg = threadRegistry;
        auto regMtx = registryMtx;

        std::vector<std::thread> workers;
        auto errors = std::make_shared<std::vector<std::string>>();
        auto errMtx = std::make_shared<std::mutex>();

        for(auto& block : s->blocks) {
            workers.emplace_back([block, functionsCopy, structsCopy, reg, regMtx, errors, errMtx]() {
                Interpreter worker(functionsCopy, structsCopy);
                worker.threadRegistry = reg;
                worker.registryMtx = regMtx;
                try {
                    worker.executeBlock(block);
                } catch(ReturnSignal&) {
                } catch(const std::exception& e) {
                    std::lock_guard<std::mutex> lock(*errMtx);
                    errors->push_back(e.what());
                } catch(...) {
                    std::lock_guard<std::mutex> lock(*errMtx);
                    errors->push_back("unknown error in Parallel block");
                }
            });
        }
        for(auto& w : workers) w.join();
        if(!errors->empty())
            throw std::runtime_error("Parallel block error: " + (*errors)[0]);
        return;
    }
    if(auto s = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        Value value;
        value.isArray = s->isArray;
        if(s->isArray) {
            for(auto& e : s->arrayValues) value.array.push_back(evalNumber(e));
        } else {
            value = evalToValue(s->value);
        }
        scopes.back()[s->name]=value;
        return;
    }
    if(auto s = std::dynamic_pointer_cast<AssignStmt>(stmt)) {
        Value& val = resolveVar(s->name);
        if(val.isArray)
            throw std::runtime_error("Cannot assign a number to array '" + s->name + "'");
        Value newVal = evalToValue(s->value);
        val = newVal;
        return;
    }
    if(auto s = std::dynamic_pointer_cast<ArrayAssignStmt>(stmt)) {
        Value& val = resolveVar(s->arrayName);
        if(!val.isArray)
            throw std::runtime_error("'" + s->arrayName + "' is not an array");
        int index = (int)evalNumber(s->index);
        if(index < 0 || index >= (int)val.array.size())
            throw std::runtime_error("Array index out of bounds for '" + s->arrayName + "': " + std::to_string(index));
        val.array[index] = evalNumber(s->value);
        return;
    }
    if(auto s = std::dynamic_pointer_cast<MemberAssignStmt>(stmt)) {
        Value& obj = resolveVar(s->objectName);
        if(obj.isObject && !obj.isObjectArray) {
            // Dynamic object field assignment (e.g. response.status = 200;
            // response.headersToSend["X"] = "Y";). Creates the field if absent,
            // matching object-literal semantics elsewhere in the language.
            Value& field = (*obj.object)[s->member];
            if(s->index) {
                if(!field.isArray)
                    throw std::runtime_error("Field '" + s->member + "' is not an array");
                int idx = (int)evalNumber(s->index);
                if(idx < 0 || idx >= (int)field.array.size())
                    throw std::runtime_error("Array index out of bounds for field '" + s->member + "': " + std::to_string(idx));
                field.array[idx] = evalNumber(s->value);
            } else {
                field = evalToValue(s->value);
            }
            return;
        }
        if(!obj.isStruct)
            throw std::runtime_error("'" + s->objectName + "' is not a struct instance or object");
        auto it = obj.fields->find(s->member);
        if(it == obj.fields->end())
            throw std::runtime_error("Struct '" + obj.structType + "' has no field '" + s->member + "'");
        Value& field = it->second;
        if(s->index) {
            if(!field.isArray)
                throw std::runtime_error("Field '" + s->member + "' is not an array");
            int idx = (int)evalNumber(s->index);
            if(idx < 0 || idx >= (int)field.array.size())
                throw std::runtime_error("Array index out of bounds for field '" + s->member + "': " + std::to_string(idx));
            field.array[idx] = evalNumber(s->value);
        } else {
            if(field.isArray)
                throw std::runtime_error("Cannot assign a number to array field '" + s->member + "'");
            Value newVal = evalToValue(s->value);
            field = newVal;
        }
        return;
    }
    if(auto s = std::dynamic_pointer_cast<PrintStmt>(stmt)) {
        if(s->printCharArray) {
            auto& arr = resolveVar(s->arrayName).array;
            for(double c : arr) std::cout << (char)c;
            std::cout << '\n';
            return;
        }
        if(s->printChar) { std::cout << (char)evalNumber(s->expr); return; }
        Value v = evalToValue(s->expr);
        std::cout << valueToDisplayString(v) << '\n';
        return;
    }
    if(auto s = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        if(evalBool(s->condition)) executeBlock(s->thenBlock);
        else if(s->elseBlock) executeBlock(s->elseBlock);
        return;
    }
    if(auto s = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        while(evalBool(s->condition)) executeBlock(s->body);
        return;
    }
    if(auto s = std::dynamic_pointer_cast<ForStmt>(stmt)) {
        execute(s->init);
        while(evalBool(s->condition)) { executeBlock(s->body); execute(s->increment); }
        return;
    }
    if(auto s = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
        ReturnSignal sig;
        if(s->value) sig.value = evalToValue(s->value);
        throw sig;
    }
    if(auto s = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        evalToValue(s->expr);
        return;
    }
    if(auto s = std::dynamic_pointer_cast<RouteDeclStmt>(stmt)) {
        Value& serverVal = resolveVar(s->serverName);
        if(!serverVal.isServer)
            throw std::runtime_error("'" + s->serverName + "' is not a Server instance");
        RegisteredRoute route;
        route.method = s->method;
        route.pathPattern = valueToDisplayString(evalToValue(s->pathExpr));
        route.body = s->body;
        serverVal.server->routes.push_back(route);
        return;
    }
    if(auto s = std::dynamic_pointer_cast<ServerStartStmt>(stmt)) {
        Value& serverVal = resolveVar(s->serverName);
        if(!serverVal.isServer)
            throw std::runtime_error("'" + s->serverName + "' is not a Server instance");
        runServerLoop(serverVal.server);
        return;
    }
    throw std::runtime_error("Unknown statement.");
}
};

void runProgram(const Program& program)
{
    Interpreter interpreter(program.functions, program.structs);
    try {
        for(auto& stmt : program.statements) interpreter.execute(stmt);
    } catch(ReturnSignal&) {
    }
}

std::string loadFile(const std::string& path)
{
    std::ifstream in(path);
    if(!in) throw std::runtime_error("Cannot open file: " + path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

int main(int argc,char** argv)
{
    std::signal(SIGPIPE, SIG_IGN);
    if(argc!=2) { std::cout << "Usage:\n" << "    CnR program.CnR\n"; return 1; }
    try {
        std::string source = loadFile(argv[1]);
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto program = parser.parseProgram();
        runProgram(program);
    }
    catch(const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}