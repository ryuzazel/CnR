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
    Var, Print, If, Else, While, For, TrueKw, FalseKw, CharKw, LenKw,
    Function, Return, StructKw,
    Parallel, ThreadKw, JoinKw, JoinAllKw,
    Plus, Minus, Star, Slash, Percent,
    Assign,
    EqualEqual, BangEqual, Less, LessEqual, Greater, GreaterEqual,
    AndAnd, OrOr, Bang,
    Semicolon, Comma, Dot,
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
    // Reads a double-quoted string literal, e.g. "String".
    // Supports the escapes \n \t \" \\ ; the opening quote is consumed here
    // (mirrors identifier()/number(), which are also called before the
    // caller's peek() has been advanced past the first character).
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
struct LenExpr : Expr { std::string arrayName; LenExpr(std::string n):arrayName(std::move(n)){} };
struct ArrayAccessExpr : Expr { std::string arrayName; ExprPtr index; ArrayAccessExpr(std::string n, ExprPtr i):arrayName(std::move(n)),index(std::move(i)){} };
struct CallExpr : Expr { std::string name; std::vector<ExprPtr> args; };
struct MemberAccessExpr : Expr { std::string objectName; std::string member; ExprPtr index; };

// --- Threading AST nodes ---
// thread(fnName, args...) — spawns fnName(args...) on a new OS thread, evaluates to a thread handle.
struct ThreadExpr : Expr { std::string fnName; std::vector<ExprPtr> args; };
// expr.join() where expr names a single thread-handle variable (or array element access via ArrayAccessExpr)
struct JoinExpr : Expr { ExprPtr handleExpr; };
// arrayName.joinAll() where arrayName is an array of thread handles
struct JoinAllExpr : Expr { std::string arrayName; };

enum class ArrayMethod { Push, Pop, Sort, Reverse, Contains, IndexOf, Accumulate };
// arrayName.push(x) / .pop() / .sort() / .reverse() / .contains(x) / .indexOf(x) / .accumulate()
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
// Parallel { } { } { } — runs each block body on its own OS thread, joins all before continuing.
struct ParallelStmt : Stmt { std::vector<std::shared_ptr<BlockStmt>> blocks; };

// --- Function & struct declarations (top-level metadata, not executed statements) ---
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

    // Recognizes array-method names (push, pop, sort, reverse, contains, indexOf, accumulate).
    // These aren't reserved keywords -- just Ident tokens matched by text -- so they don't
    // collide with variables or functions named e.g. "sort".
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
    // Parses '.methodName(args)' assuming '.' has NOT yet been consumed and
    // peek(1) is already confirmed to be an Ident naming a known array method.
    // Consumes through the closing ')'.
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

    // Builds the arrayValues list for a string literal token (already consumed),
    // one NumberExpr per character holding its ASCII code. This is how
    // 'var s[] = "abc";' desugars into 'var s[] = {97,98,99};' -- strings are
    // just sugar over the existing numeric-array machinery, nothing else
    // downstream needs to know a string was ever involved.
    std::vector<ExprPtr> stringToArrayValues(const std::string& text) {
        std::vector<ExprPtr> values;
        values.reserve(text.size());
        for(unsigned char ch : text) values.push_back(std::make_shared<NumberExpr>((double)ch));
        return values;
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
        // handles primary + optional call/member/index suffixes
        auto expr = parsePrimary();
        // allow .join()/.joinAll() and array-method suffixes directly on the
        // primary expression, e.g. threads[0].join(), threads.joinAll(), arr.push(5)
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
                // thread(somar(10)) style — args go inside the inner parens
                if(!check(TokType::RParen)) {
                    while(true) {
                        t->args.push_back(parseExpression());
                        if(match(TokType::Comma)) continue;
                        break;
                    }
                }
                expect(TokType::RParen,")");
            } else {
                // thread(somar, 10) style — args are comma-separated after the name
                while(match(TokType::Comma)) {
                    t->args.push_back(parseExpression());
                }
            }
            expect(TokType::RParen,")");
            return t;
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
            auto e = parseExpression();
            expect(TokType::RParen,")");
            return e;
        }
        if(match(TokType::LenKw)) {
            expect(TokType::LParen,"(");
            auto id = expect(TokType::Ident,"identifier");
            expect(TokType::RParen,")");
            return std::make_shared<LenExpr>(id.text);
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
            if(check(TokType::Dot)) {
                // .join() / .joinAll() / array-methods are handled by parseCall's suffix loop, not here.
                ArrayMethod amCheck;
                if(peek(1).type == TokType::JoinKw || peek(1).type == TokType::JoinAllKw ||
                   (peek(1).type == TokType::Ident && tryArrayMethodName(peek(1).text, amCheck) && peek(2).type == TokType::LParen)) {
                    return std::make_shared<VarExpr>(name);
                }
                advance();
                auto m = std::make_shared<MemberAccessExpr>();
                m->objectName = name;
                m->member = expect(TokType::Ident,"member name").text;
                if(match(TokType::LBracket)) {
                    m->index = parseExpression();
                    expect(TokType::RBracket,"]");
                }
                return m;
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
        // print(string.name) -- prints array 'name' as a string (its doubles
        // read as ASCII codes), exactly like print((char[])name) below, just
        // with friendlier syntax. "string" is not a reserved keyword, it's
        // matched by text here, so it never collides with a variable/function
        // that happens to be named "string" anywhere else in the language.
        if(check(TokType::Ident) && peek().text=="string" && peek(1).type==TokType::Dot && peek(2).type==TokType::Ident) {
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
        // var[] name;                -- empty, growable array (filled later via .push())
        // var[] name = "String";     -- growable array pre-filled with a string's ASCII codes
        // var[] name = { 1, 2, 3 };  -- growable array pre-filled with numeric values
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
            // var name[] = "String";     -- same string sugar as above
            // var name[] = { 1, 2, 3 };  -- plain numeric array literal
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
            // Could be threads[i].join()/.joinAll(), an array method call like
            // arr.push(5), or a struct member assignment.
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
            // threads[0].join() as a bare statement
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
    bool isThread = false;   // true if this Value holds a thread handle id (stored in `number`)
    double number = 0;
    bool boolean = false;
    std::vector<double> array;
    std::string structType;
    std::shared_ptr<std::unordered_map<std::string, Value>> fields;
};

// Thrown by 'return' statements; carries the returned value up to the call site.
struct ReturnSignal { Value value; };

#include <thread>
#include <mutex>
#include <future>

// A single spawned thread's bookkeeping: the std::thread itself plus a place
// to stash its result (or an exception) once it finishes, protected by a mutex
// since join() may be called from the main thread once the worker completes.
struct ThreadRecord {
    std::thread worker;
    std::mutex mtx;
    bool finished = false;
    bool hasError = false;
    std::string errorMsg;
    Value result;
};

class Interpreter {
public:
    Interpreter(std::unordered_map<std::string, FunctionDecl> fns,
                std::unordered_map<std::string, StructDecl> strs)
        : functions(std::move(fns)), structs(std::move(strs)) {
        scopes.push_back({});
    }

    // Interpreters spawned for threads share the read-only function/struct tables
    // and the same thread registry as their parent, but get their own scope stack.
    Interpreter(const Interpreter& parent, bool /*forThread*/)
        : functions(parent.functions), structs(parent.structs), threadRegistry(parent.threadRegistry) {
        scopes.push_back({});
    }

    std::vector<std::unordered_map<std::string, Value>> scopes;
    std::unordered_map<std::string, FunctionDecl> functions;
    std::unordered_map<std::string, StructDecl> structs;
    Value* currentSelf = nullptr; // non-null while executing a struct constructor

    // Shared thread registry: index = thread handle id. Shared via shared_ptr so
    // that spawned sub-interpreters and the parent all see the same table.
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

// Resolves a bare name: inside a constructor, struct fields take priority
// over outer scopes so 'id = 5;' targets the instance being built.
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
        // Logical operators short-circuit and produce 0/1
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
        return v.number; // thread handle id
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

// General expression evaluator: returns a full Value (used where the result
// might be a struct instance or the outcome of a function call).
Value evalToValue(const ExprPtr& expr) {
    if(auto c = std::dynamic_pointer_cast<CallExpr>(expr)) return callCallable(c->name, c->args);
    if(auto m = std::dynamic_pointer_cast<MemberAccessExpr>(expr)) return evalMemberAccess(m);
    if(auto b = std::dynamic_pointer_cast<BoolExpr>(expr)) { Value v; v.isBool=true; v.boolean=b->value; return v; }
    if(auto t = std::dynamic_pointer_cast<ThreadExpr>(expr)) return spawnThread(t);
    if(auto j = std::dynamic_pointer_cast<JoinExpr>(expr)) return joinThread(j);
    if(auto ja = std::dynamic_pointer_cast<JoinAllExpr>(expr)) return joinAllThreads(ja);
    if(auto am = std::dynamic_pointer_cast<ArrayMethodCallExpr>(expr)) { Value v; v.number = callArrayMethod(am); return v; }
    if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
        Value& val = resolveVar(v->name);
        return val; // copy (arrays/struct fields copied by value, consistent with rest of language)
    }
    Value v; v.number = evalNumber(expr); return v;
}

// Executes an array method (.push/.pop/.sort/.reverse/.contains/.indexOf/.accumulate)
// directly against the target array's std::vector<double>, and returns a numeric
// result: push -> new length, pop -> removed value, sort/reverse -> new length,
// contains -> 1/0, indexOf -> index or -1, accumulate -> sum of all elements.
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

Value evalMemberAccess(const std::shared_ptr<MemberAccessExpr>& m) {
    Value& obj = resolveVar(m->objectName);
    if(!obj.isStruct)
        throw std::runtime_error("'" + m->objectName + "' is not a struct instance");
    auto it = obj.fields->find(m->member);
    if(it == obj.fields->end())
        throw std::runtime_error("Struct '" + obj.structType + "' has no field '" + m->member + "'");
    Value& field = it->second;
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

// --- Threading support ---

// Spawns fnName(args...) on a new OS thread. Each thread gets its own
// Interpreter with a fresh scope stack; it only shares the read-only
// functions/structs tables and the thread registry with the parent.
// Returns a Value holding the new thread's handle id (an index into threadRegistry).
Value spawnThread(const std::shared_ptr<ThreadExpr>& t) {
    auto fIt = functions.find(t->fnName);
    if(fIt == functions.end())
        throw std::runtime_error("thread(): undefined function '" + t->fnName + "'");
    const FunctionDecl& fn = fIt->second;
    if(t->args.size() != fn.params.size())
        throw std::runtime_error("thread(): function '" + t->fnName + "' expects " +
                                  std::to_string(fn.params.size()) + " argument(s), got " +
                                  std::to_string(t->args.size()));

    // Evaluate arguments in the *calling* thread's scope, before handing off,
    // since ExprPtr args may reference variables in the spawning interpreter.
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

    // Copy what the worker needs by value; functions/structs tables copied
    // (they're read-only after parsing, so a copy per thread is simplest and safest).
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

// Resolves an expression that should evaluate to a thread handle id.
// Thread handles are represented as plain numeric ids; when stored inside a
// 'var threads[] = { thread(...), ... }' array literal they lose the isThread
// flag (arrays only hold raw doubles), so array-element access is trusted
// here rather than requiring isThread — the registry bounds check below still
// catches genuinely bogus values.
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

// threads.joinAll() — joins every handle stored in the named array, in order,
// waiting for all to finish. Returns an array Value of their numeric results
// (struct/array-returning threads aren't representable in a numeric array,
// so joinAll() is meant for numeric-returning worker functions).
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
    currentSelf = nullptr; // a function body is not "inside" a struct instance
    Value result; // default: number 0, used when the function is void
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
        // 'return;' with no value simply ends the constructor early; ignore.
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

// Runs a Parallel block's statements on a new OS thread using a fresh
// Interpreter that shares only functions/structs/threadRegistry with this one.
// Any error inside is captured and rethrown on the calling thread after all
// blocks finish, so one failing block doesn't hide others' errors silently.
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
                    // a bare 'return;' inside a Parallel block just ends that block
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
        if(!obj.isStruct)
            throw std::runtime_error("'" + s->objectName + "' is not a struct instance");
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
        std::cout << evalNumber(s->expr) << '\n';
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
        evalToValue(s->expr); // result discarded (e.g. a void function call)
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
        // a top-level 'return;' simply ends the program
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