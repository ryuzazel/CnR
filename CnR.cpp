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
#include <array>
#include <cstdint>
#include <cstring>
#include <random>
#include <cstdio>

enum class TokType {
    Number, Ident, String,
    Var, Print, If, Else, While, For, TrueKw, FalseKw, CharKw, LenKw, StringKw,
    Function, Return, StructKw,
    Parallel, ThreadKw, JoinKw, JoinAllKw,
    HttpKw, HeaderKw, BodyKw,
    NodesKw, OnFailKw, RetryKw, FailKw,
    TryKw, CatchKw, ThrowKw,
    DataKw, TableKw,
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
        if (s == "Nodes") return {TokType::NodesKw,s,0,startLine};
        if (s == "OnFail") return {TokType::OnFailKw,s,0,startLine};
        if (s == "Retry") return {TokType::RetryKw,s,0,startLine};
        if (s == "Fail") return {TokType::FailKw,s,0,startLine};
        if (s == "try") return {TokType::TryKw,s,0,startLine};
        if (s == "catch") return {TokType::CatchKw,s,0,startLine};
        if (s == "Throw") return {TokType::ThrowKw,s,0,startLine};
        if (s == "Data") return {TokType::DataKw,s,0,startLine};
        if (s == "table") return {TokType::TableKw,s,0,startLine};
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

// --- DAG / Nodes workflow ---

// Fail(); -- only meaningful inside a Nodes{} node body. Aborts the current
// attempt of the enclosing node (counts as a failed attempt; retried if the
// node's OnFail(Retry=N) budget allows).
struct FailExpr : Expr {};

// One node inside a `Nodes Name { ... }` statement:
//   NodeName(dep1, dep2, ...) -> OnFail(Retry = N) { body }
// `dependsOn` holds the names of sibling nodes (within the same Nodes
// statement) that must succeed before this node is eligible to run.
// `maxRetries` defaults to 0 (no OnFail clause = fail is final immediately).
struct NodeDecl {
    std::string name;
    std::vector<std::string> dependsOn;
    int maxRetries = 0;
    std::shared_ptr<BlockStmt> body;
};

// `Nodes MyWorkflow { NodeDecl* }` -- a statement (not a declaration): when
// execution reaches it, the whole DAG runs to completion (nodes at the same
// dependency depth run concurrently as threads) before the next statement
// after the Nodes block executes. `workflowName` is a label only (used in
// error messages), not a callable identifier.
struct NodesStmt : Stmt {
    std::string workflowName;
    std::vector<NodeDecl> nodes;
};

// Throw("message"); -- raises a catchable runtime error carrying a string
// message. Distinct from Fail(), which stays specific to Nodes{} node
// bodies and always marks the enclosing node attempt as failed even when
// caught by a try/catch inside that same body.
struct ThrowExpr : Expr { ExprPtr messageExpr; };

// try { ... } catch(var e) { ... } -- generic exception handling. Catches:
//   - Throw("msg") raised anywhere inside the try block
//   - ordinary runtime errors (std::runtime_error) raised inside the try block
//   - Fail() raised inside the try block (the catch body still runs, but
//     Fail() additionally marks the enclosing Nodes node attempt as failed
//     for retry purposes -- see CnrThrowSignal/NodeFailSignal handling in
//     the interpreter)
// Does NOT catch ReturnSignal (a `return;` inside try still returns from
// the enclosing function normally).
// `catchVarName` is always present syntactically (catch(var e)) and is
// bound to the error message as a string inside catchBlock.
struct TryCatchStmt : Stmt {
    std::shared_ptr<BlockStmt> tryBlock;
    std::string catchVarName;
    std::shared_ptr<BlockStmt> catchBlock;
};

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

// Data1.encode("key")                -> dataName="Data1", tableName="",       method="encode"
// Data1.userList.push(1,"Test")      -> dataName="Data1", tableName="userList", method="push"
// Data1.userList.find("Test")        -> ... method="find"
// Data1.userList.delete(0)           -> ... method="delete"
// Data1.userList.insert(0,"name",v)  -> ... method="insert"
// Data1.userList.save() / .load() / .count()
struct DbMethodCallExpr : Expr {
    std::string dataName;
    std::string tableName; // empty when the call targets the Data object itself (e.g. encode)
    std::string method;
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

// ----------------------------------------------------------------------------
// Native database ("Data") support.
//
//   Data Name("file.cnrdb"){
//       Struct Users{ var id; var name; Users(var i, var n){ id=i; name=n; } }
//       table Users userList;
//   }
//
// A TableDecl ("table StructType varName;") declares one persistent record
// table typed to a struct defined in the same Data block. A DataDecl bundles
// its own private struct namespace with one or more table declarations and
// the backing file name. At runtime a Data instance is a Value with
// isDatabase=true (see below) holding one in-memory table per TableDecl.
// ----------------------------------------------------------------------------
struct TableDecl {
    std::string structType;
    std::string varName;
    std::string primaryKeyField; // empty if no primary key declared
};

struct DataDecl {
    std::string name;
    std::string fileNameLiteral; // raw string literal given in Data Name("file")
    std::unordered_map<std::string, StructDecl> structs; // structs declared inside this Data block
    std::vector<TableDecl> tables;
};

struct Program {
    std::vector<StmtPtr> statements;
    std::unordered_map<std::string, FunctionDecl> functions;
    std::unordered_map<std::string, StructDecl> structs;
    std::unordered_map<std::string, DataDecl> datas;
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
    bool isDbMethodName(const std::string& text) {
        static const std::vector<std::string> names = {
            "encode","push","find","delete","insert","save","load","count",
            "findWhere","updateWhere","deleteWhere",
            "findById","updateById","deleteById",
            "orderBy","get","getById"
        };
        for(auto& n : names) if(n==text) return true;
        return false;
    }
    // Parses `id.method(args)` (targets the Data object itself, e.g. .encode())
    // when tableName is empty, or `id.tableName.method(args)` (targets a table,
    // e.g. .push()/.find()/.delete()/.insert()/.save()/.load()/.count()).
    // Caller has already confirmed the shape matches via isDbMethodName lookahead.
    ExprPtr parseDbMethodCall(const std::string& dataName, const std::string& tableName) {
        advance(); // '.'
        std::string methodText = advance().text; // method name ident
        expect(TokType::LParen,"(");
        auto call = std::make_shared<DbMethodCallExpr>();
        call->dataName = dataName;
        call->tableName = tableName;
        call->method = methodText;
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
            // Data1.encode("key")            -- id . method (
            if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
                if(peek(1).type == TokType::Ident && isDbMethodName(peek(1).text) && peek(2).type == TokType::LParen) {
                    expr = parseDbMethodCall(v->name, "");
                    continue;
                }
                // Data1.userList.push(...)   -- id . tableName . method (
                if(peek(1).type == TokType::Ident && peek(2).type == TokType::Dot &&
                   peek(3).type == TokType::Ident && isDbMethodName(peek(3).text) && peek(4).type == TokType::LParen) {
                    std::string tableName = peek(1).text;
                    advance(); advance(); // '.' tableName
                    expr = parseDbMethodCall(v->name, tableName);
                    continue;
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
        if(match(TokType::FailKw)) {
            expect(TokType::LParen,"(");
            expect(TokType::RParen,")");
            return std::make_shared<FailExpr>();
        }
        if(match(TokType::ThrowKw)) {
            expect(TokType::LParen,"(");
            auto t = std::make_shared<ThrowExpr>();
            t->messageExpr = parseExpression();
            expect(TokType::RParen,")");
            return t;
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
            // Data1.encode("key");  /  Data1.userList.push(...);  etc as bare statements
            if(peek(1).type == TokType::Ident && isDbMethodName(peek(1).text) && peek(2).type == TokType::LParen) {
                auto call = parseDbMethodCall(id.text, "");
                if(consumeSemicolon) expect(TokType::Semicolon,";");
                auto stmt = std::make_shared<ExprStmt>();
                stmt->expr = call;
                return stmt;
            }
            if(peek(1).type == TokType::Ident && peek(2).type == TokType::Dot &&
               peek(3).type == TokType::Ident && isDbMethodName(peek(3).text) && peek(4).type == TokType::LParen) {
                std::string tableName = peek(1).text;
                advance(); advance(); // '.' tableName
                auto call = parseDbMethodCall(id.text, tableName);
                if(consumeSemicolon) expect(TokType::Semicolon,";");
                auto stmt = std::make_shared<ExprStmt>();
                stmt->expr = call;
                return stmt;
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

    NodeDecl parseNodeDecl() {
        NodeDecl node;
        node.name = expect(TokType::Ident,"node name").text;
        expect(TokType::LParen,"(");
        if(!check(TokType::RParen)) {
            while(true) {
                node.dependsOn.push_back(expect(TokType::Ident,"dependency node name").text);
                if(match(TokType::Comma)) continue;
                break;
            }
        }
        expect(TokType::RParen,")");
        if(match(TokType::Minus)) {
            // matched '-' of '->'; consume the '>' greedily since the lexer
            // tokenizes '-' and '>' separately (no dedicated Arrow token)
            expect(TokType::Greater,">");
            expect(TokType::OnFailKw,"OnFail");
            expect(TokType::LParen,"(");
            expect(TokType::RetryKw,"Retry");
            expect(TokType::Assign,"=");
            auto n = expect(TokType::Number,"retry count");
            node.maxRetries = (int)n.number;
            expect(TokType::RParen,")");
        }
        node.body = parseBlock();
        return node;
    }

    StmtPtr parseNodes() {
        expect(TokType::NodesKw,"Nodes");
        auto stmt = std::make_shared<NodesStmt>();
        stmt->workflowName = expect(TokType::Ident,"workflow name").text;
        expect(TokType::LBrace,"{");
        while(!check(TokType::RBrace)) {
            if(check(TokType::End))
                throw std::runtime_error("Unexpected end of file inside Nodes '" + stmt->workflowName + "'");
            stmt->nodes.push_back(parseNodeDecl());
            match(TokType::Semicolon); // trailing ';' after a node block is optional but tolerated
        }
        expect(TokType::RBrace,"}");
        // Validate dependency names refer to nodes declared in this same workflow.
        for(auto& n : stmt->nodes) {
            for(auto& dep : n.dependsOn) {
                bool found = false;
                for(auto& other : stmt->nodes) if(other.name == dep) { found = true; break; }
                if(!found)
                    throw std::runtime_error("Nodes '" + stmt->workflowName + "': node '" + n.name +
                                              "' depends on undeclared node '" + dep + "'");
            }
        }
        return stmt;
    }

    StmtPtr parseTryCatch() {
        expect(TokType::TryKw,"try");
        auto stmt = std::make_shared<TryCatchStmt>();
        stmt->tryBlock = parseBlock();
        expect(TokType::CatchKw,"catch");
        expect(TokType::LParen,"(");
        expect(TokType::Var,"'var' before catch variable name");
        stmt->catchVarName = expect(TokType::Ident,"catch variable name").text;
        expect(TokType::RParen,")");
        stmt->catchBlock = parseBlock();
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
        if(check(TokType::NodesKw)) return parseNodes();
        if(check(TokType::TryKw)) return parseTryCatch();
        if(check(TokType::FailKw)) {
            auto stmt = std::make_shared<ExprStmt>();
            stmt->expr = parseExpression();
            expect(TokType::Semicolon,";");
            return stmt;
        }
        if(check(TokType::ThrowKw)) {
            auto stmt = std::make_shared<ExprStmt>();
            stmt->expr = parseExpression();
            expect(TokType::Semicolon,";");
            return stmt;
        }
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

    DataDecl parseDataDecl() {
        expect(TokType::DataKw,"Data");
        DataDecl dd;
        dd.name = expect(TokType::Ident,"database name").text;
        expect(TokType::LParen,"(");
        dd.fileNameLiteral = expect(TokType::String,"database file name string").text;
        expect(TokType::RParen,")");
        expect(TokType::LBrace,"{");
        while(!check(TokType::RBrace)) {
            if(check(TokType::End))
                throw std::runtime_error("Unexpected end of file inside Data '" + dd.name + "'");
            if(check(TokType::StructKw)) {
                auto sd = parseStructDecl();
                dd.structs[sd.name] = sd;
                continue;
            }
            if(check(TokType::TableKw)) {
                advance();
                TableDecl td;
                td.structType = expect(TokType::Ident,"struct type name").text;
                td.varName = expect(TokType::Ident,"table variable name").text;
                if(check(TokType::Ident) && peek().text == "primaryKey") {
                    advance();
                    td.primaryKeyField = expect(TokType::Ident,"primary key field name").text;
                }
                expect(TokType::Semicolon,";");
                dd.tables.push_back(td);
                continue;
            }
            throw std::runtime_error("Unexpected token in Data '" + dd.name + "' on line " + std::to_string(peek().line) +
                                      " (expected 'Struct' or 'table')");
        }
        expect(TokType::RBrace,"}");
        for(auto& td : dd.tables) {
            auto sit = dd.structs.find(td.structType);
            if(sit == dd.structs.end())
                throw std::runtime_error("Data '" + dd.name + "': table '" + td.varName + "' refers to undefined struct '" + td.structType + "'");
            if(!td.primaryKeyField.empty()) {
                bool found = false;
                for(auto& f : sit->second.fields) if(f.name == td.primaryKeyField) { found = true; break; }
                if(!found)
                    throw std::runtime_error("Data '" + dd.name + "': table '" + td.varName + "' primaryKey '" + td.primaryKeyField +
                                              "' is not a field of struct '" + td.structType + "'");
            }
        }
        return dd;
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
            if(check(TokType::DataKw)) {
                auto dd = parseDataDecl();
                prog.datas[dd.name] = dd;
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
    bool isDatabase = false;
    std::shared_ptr<struct DatabaseInstance> database;

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
    if(v.isDatabase) return "[database]";
    if(v.isArray) return "[array]";
    std::ostringstream oss;
    if(v.number == (long long)v.number) oss << (long long)v.number;
    else oss << v.number;
    return oss.str();
}

// Generic "==" for two Values, used by BinaryExpr EqualEqual/BangEqual so
// that `==`/`!=` work correctly on the operand kinds this language actually
// has, instead of silently falling back to a numeric compare (which used to
// make two different strings, or two different arrays, look "equal" just
// because their raw .number field defaulted to 0 on both sides):
//   - arrays: same length and every element numerically equal, in order
//   - strings: same character content
//   - null: equal only to another null
//   - everything else (numbers, bools): numeric compare, bools as 0/1
// Mismatched kinds (e.g. an array vs a string) are never equal.
bool valuesEqual(const Value& a, const Value& b) {
    if(a.isArray || b.isArray) {
        if(!a.isArray || !b.isArray) return false;
        if(a.array.size() != b.array.size()) return false;
        for(size_t i=0;i<a.array.size();++i) {
            if(a.array[i] != b.array[i]) return false;
        }
        return true;
    }
    if(a.isString || b.isString) {
        if(!a.isString || !b.isString) return false;
        return a.str == b.str;
    }
    if(a.isNull || b.isNull) {
        return a.isNull && b.isNull;
    }
    double av = a.isBool ? (a.boolean ? 1.0 : 0.0) : a.number;
    double bv = b.isBool ? (b.boolean ? 1.0 : 0.0) : b.number;
    return av == bv;
}

struct ReturnSignal { Value value; };

// Thrown by Fail() to abort the current attempt of the enclosing Nodes{}
// node body. Caught by the per-node retry loop in runNodesWorkflow(); if
// Fail() is somehow reached outside a node body, it propagates up like any
// other uncaught error (reported as "Fail() called outside of a Nodes node").
struct NodeFailSignal {};

// Thrown by Throw("msg"). Carries a string message, caught by the nearest
// enclosing try/catch (see TryCatchStmt execution). Unlike NodeFailSignal,
// catching this has no special side effect on Nodes{} retry bookkeeping.
struct CnrThrowSignal { std::string message; };

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

// ============================================================================
// Minimal self-contained SHA-256 + AES-256-CBC, used to give Data.encode(key)
// real AES-256 encryption at rest for .cnrdb files without any external
// crypto library (the sandboxed build environment can't fetch one). SHA-256
// derives a 32-byte key from the user-supplied passphrase; AES-256-CBC with
// PKCS#7 padding and a random IV (stored in the file header) encrypts the
// serialized table bytes.
// ============================================================================
namespace cnrcrypto {

// ---- SHA-256 -----------------------------------------------------------
struct Sha256 {
    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

    static std::array<uint8_t,32> hash(const std::vector<uint8_t>& msg) {
        static const uint32_t k[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
        uint32_t h[8] = {
            0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
            0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19
        };
        std::vector<uint8_t> data = msg;
        uint64_t bitLen = (uint64_t)msg.size() * 8;
        data.push_back(0x80);
        while (data.size() % 64 != 56) data.push_back(0x00);
        for (int i = 7; i >= 0; --i) data.push_back((uint8_t)(bitLen >> (i*8)));

        for (size_t chunk = 0; chunk < data.size(); chunk += 64) {
            uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = (data[chunk+i*4] << 24) | (data[chunk+i*4+1] << 16) |
                       (data[chunk+i*4+2] << 8) | (data[chunk+i*4+3]);
            }
            for (int i = 16; i < 64; ++i) {
                uint32_t s0 = rotr(w[i-15],7) ^ rotr(w[i-15],18) ^ (w[i-15] >> 3);
                uint32_t s1 = rotr(w[i-2],17) ^ rotr(w[i-2],19) ^ (w[i-2] >> 10);
                w[i] = w[i-16] + s0 + w[i-7] + s1;
            }
            uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
            for (int i = 0; i < 64; ++i) {
                uint32_t S1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
                uint32_t ch = (e & f) ^ ((~e) & g);
                uint32_t temp1 = hh + S1 + ch + k[i] + w[i];
                uint32_t S0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
                uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                uint32_t temp2 = S0 + maj;
                hh=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
            }
            h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
        }
        std::array<uint8_t,32> out;
        for (int i = 0; i < 8; ++i) {
            out[i*4]   = (uint8_t)(h[i] >> 24);
            out[i*4+1] = (uint8_t)(h[i] >> 16);
            out[i*4+2] = (uint8_t)(h[i] >> 8);
            out[i*4+3] = (uint8_t)(h[i]);
        }
        return out;
    }
};

inline std::array<uint8_t,32> deriveKey(const std::string& passphrase) {
    std::vector<uint8_t> bytes(passphrase.begin(), passphrase.end());
    return Sha256::hash(bytes);
}

// ---- AES-256 (ECB core, used to build CBC) ------------------------------
class Aes256 {
public:
    explicit Aes256(const std::array<uint8_t,32>& key) { keyExpansion(key); }

    void encryptBlock(const uint8_t in[16], uint8_t out[16]) const {
        uint8_t state[16];
        std::memcpy(state, in, 16);
        addRoundKey(state, 0);
        for (int round = 1; round < 14; ++round) {
            subBytes(state);
            shiftRows(state);
            mixColumns(state);
            addRoundKey(state, round);
        }
        subBytes(state);
        shiftRows(state);
        addRoundKey(state, 14);
        std::memcpy(out, state, 16);
    }

    void decryptBlock(const uint8_t in[16], uint8_t out[16]) const {
        uint8_t state[16];
        std::memcpy(state, in, 16);
        addRoundKey(state, 14);
        for (int round = 13; round >= 1; --round) {
            invShiftRows(state);
            invSubBytes(state);
            addRoundKey(state, round);
            invMixColumns(state);
        }
        invShiftRows(state);
        invSubBytes(state);
        addRoundKey(state, 0);
        std::memcpy(out, state, 16);
    }

private:
    uint32_t roundKeys[60]; // 15 round keys * 4 words for AES-256 (Nr=14)

    static const uint8_t* sbox() {
        static const uint8_t s[256] = {
            0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
            0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
            0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
            0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
            0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
            0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
            0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
            0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
            0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
            0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
            0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
            0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
            0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
            0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
            0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
            0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
        };
        return s;
    }
    static const uint8_t* invSbox() {
        static uint8_t inv[256]; static bool init=false;
        if(!init) { const uint8_t* s=sbox(); for(int i=0;i<256;i++) inv[s[i]]=(uint8_t)i; init=true; }
        return inv;
    }
    static uint8_t xtime(uint8_t x) { return (uint8_t)((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00)); }
    static uint8_t gmul(uint8_t a, uint8_t b) {
        uint8_t p = 0;
        for (int i = 0; i < 8; ++i) {
            if (b & 1) p ^= a;
            a = xtime(a);
            b >>= 1;
        }
        return p;
    }

    void keyExpansion(const std::array<uint8_t,32>& key) {
        const int Nk = 8, Nr = 14, Nb = 4;
        uint8_t* rk = reinterpret_cast<uint8_t*>(roundKeys);
        std::memcpy(rk, key.data(), 32);
        uint8_t temp[4];
        static const uint8_t rcon[15] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36,0x6c,0xd8,0xab,0x4d};
        for (int i = Nk; i < Nb*(Nr+1); ++i) {
            std::memcpy(temp, rk + (i-1)*4, 4);
            if (i % Nk == 0) {
                uint8_t t = temp[0]; temp[0]=temp[1]; temp[1]=temp[2]; temp[2]=temp[3]; temp[3]=t;
                for (int j = 0; j < 4; ++j) temp[j] = sbox()[temp[j]];
                temp[0] ^= rcon[i/Nk - 1];
            } else if (i % Nk == 4) {
                for (int j = 0; j < 4; ++j) temp[j] = sbox()[temp[j]];
            }
            for (int j = 0; j < 4; ++j) rk[i*4+j] = rk[(i-Nk)*4+j] ^ temp[j];
        }
    }

    void addRoundKey(uint8_t state[16], int round) const {
        const uint8_t* rk = reinterpret_cast<const uint8_t*>(roundKeys) + round*16;
        for (int i = 0; i < 16; ++i) state[i] ^= rk[i];
    }
    static void subBytes(uint8_t state[16]) { for (int i=0;i<16;i++) state[i]=sbox()[state[i]]; }
    static void invSubBytes(uint8_t state[16]) { for (int i=0;i<16;i++) state[i]=invSbox()[state[i]]; }
    static void shiftRows(uint8_t s[16]) {
        uint8_t t;
        t=s[1]; s[1]=s[5]; s[5]=s[9]; s[9]=s[13]; s[13]=t;
        t=s[2]; s[2]=s[10]; s[10]=t; t=s[6]; s[6]=s[14]; s[14]=t;
        t=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=s[3]; s[3]=t;
    }
    static void invShiftRows(uint8_t s[16]) {
        uint8_t t;
        t=s[13]; s[13]=s[9]; s[9]=s[5]; s[5]=s[1]; s[1]=t;
        t=s[2]; s[2]=s[10]; s[10]=t; t=s[6]; s[6]=s[14]; s[14]=t;
        t=s[3]; s[3]=s[7]; s[7]=s[11]; s[11]=s[15]; s[15]=t;
    }
    static void mixColumns(uint8_t s[16]) {
        for (int c = 0; c < 4; ++c) {
            uint8_t a0=s[c*4],a1=s[c*4+1],a2=s[c*4+2],a3=s[c*4+3];
            s[c*4]   = (uint8_t)(gmul(a0,2) ^ gmul(a1,3) ^ a2 ^ a3);
            s[c*4+1] = (uint8_t)(a0 ^ gmul(a1,2) ^ gmul(a2,3) ^ a3);
            s[c*4+2] = (uint8_t)(a0 ^ a1 ^ gmul(a2,2) ^ gmul(a3,3));
            s[c*4+3] = (uint8_t)(gmul(a0,3) ^ a1 ^ a2 ^ gmul(a3,2));
        }
    }
    static void invMixColumns(uint8_t s[16]) {
        for (int c = 0; c < 4; ++c) {
            uint8_t a0=s[c*4],a1=s[c*4+1],a2=s[c*4+2],a3=s[c*4+3];
            s[c*4]   = (uint8_t)(gmul(a0,14) ^ gmul(a1,11) ^ gmul(a2,13) ^ gmul(a3,9));
            s[c*4+1] = (uint8_t)(gmul(a0,9)  ^ gmul(a1,14) ^ gmul(a2,11) ^ gmul(a3,13));
            s[c*4+2] = (uint8_t)(gmul(a0,13) ^ gmul(a1,9)  ^ gmul(a2,14) ^ gmul(a3,11));
            s[c*4+3] = (uint8_t)(gmul(a0,11) ^ gmul(a1,13) ^ gmul(a2,9)  ^ gmul(a3,14));
        }
    }
};

// PKCS#7-padded AES-256-CBC encrypt/decrypt over a byte buffer.
inline std::vector<uint8_t> aesCbcEncrypt(const std::vector<uint8_t>& plain, const std::array<uint8_t,32>& key, const uint8_t iv[16]) {
    Aes256 aes(key);
    std::vector<uint8_t> data = plain;
    uint8_t pad = (uint8_t)(16 - (data.size() % 16));
    for (int i = 0; i < pad; ++i) data.push_back(pad);

    std::vector<uint8_t> out(data.size());
    uint8_t prev[16]; std::memcpy(prev, iv, 16);
    for (size_t off = 0; off < data.size(); off += 16) {
        uint8_t block[16];
        for (int i = 0; i < 16; ++i) block[i] = data[off+i] ^ prev[i];
        uint8_t enc[16];
        aes.encryptBlock(block, enc);
        std::memcpy(&out[off], enc, 16);
        std::memcpy(prev, enc, 16);
    }
    return out;
}

inline std::vector<uint8_t> aesCbcDecrypt(const std::vector<uint8_t>& cipher, const std::array<uint8_t,32>& key, const uint8_t iv[16]) {
    if (cipher.empty() || cipher.size() % 16 != 0)
        throw std::runtime_error("Corrupt encrypted database: ciphertext length is not a multiple of 16");
    Aes256 aes(key);
    std::vector<uint8_t> out(cipher.size());
    uint8_t prev[16]; std::memcpy(prev, iv, 16);
    for (size_t off = 0; off < cipher.size(); off += 16) {
        uint8_t dec[16];
        aes.decryptBlock(&cipher[off], dec);
        for (int i = 0; i < 16; ++i) out[off+i] = dec[i] ^ prev[i];
        std::memcpy(prev, &cipher[off], 16);
    }
    uint8_t pad = out.empty() ? 0 : out.back();
    if (pad == 0 || pad > 16 || (size_t)pad > out.size())
        throw std::runtime_error("Failed to decrypt database: wrong key or corrupt file");
    out.resize(out.size() - pad);
    return out;
}

} // namespace cnrcrypto

// ============================================================================
// DatabaseInstance: runtime state for one `Data Name("file"){...}` instance.
// Holds one in-memory record table per `table Struct name;` declaration, the
// backing .cnrdb file path, and the optional AES-256 key set by .encode().
// Serialization format (before optional encryption):
//   magic "CNRD" (4 bytes) + version byte (1)
//   for each table, in declaration order:
//     tableName length (u32) + tableName bytes
//     recordCount (u32)
//     for each record:
//       fieldCount (u32)
//       for each field: fieldName length(u32)+bytes, then a 1-byte type tag
//         (0=number, 1=string) followed by either 8 bytes double or
//         length(u32)+bytes.
// If a key was set via .encode(), the whole payload above is AES-256-CBC
// encrypted (PKCS#7 padded) and the file on disk is instead:
//   magic "CNRE" (4 bytes) + version byte (1) + 16-byte IV + ciphertext.
// ============================================================================
struct DbRecordTable {
    std::string structType;
    std::string primaryKeyField; // empty if this table has no primary key
    std::vector<Value> records;  // each is a struct-instance Value (isStruct=true)
    // Maps primary-key value (as its display string) -> index into records,
    // kept in sync by every mutating operation. Lets findById/updateById/
    // deleteById run in O(1) instead of scanning every record.
    std::unordered_map<std::string, size_t> pkIndex;

    void rebuildPkIndex() {
        pkIndex.clear();
        if(primaryKeyField.empty()) return;
        for(size_t i=0;i<records.size();++i) {
            auto it = records[i].fields->find(primaryKeyField);
            if(it != records[i].fields->end())
                pkIndex[valueToDisplayString(it->second)] = i;
        }
    }
};

struct DatabaseInstance {
    std::string name;
    std::string filePath;
    std::unordered_map<std::string, DbRecordTable> tables; // keyed by table var name
    std::vector<std::string> tableOrder; // preserves declaration order for stable file layout
    bool hasKey = false;
    std::array<uint8_t,32> key{};
    // Guards every read/write against this instance's tables and its file on
    // disk. CnR programs can run Data operations from multiple threads via
    // Parallel{}/thread(), so without this two pushes racing could corrupt
    // the pkIndex or interleave writes to the .cnrdb file.
    std::shared_ptr<std::mutex> mtx = std::make_shared<std::mutex>();

    DbRecordTable& table(const std::string& n) {
        auto it = tables.find(n);
        if (it == tables.end())
            throw std::runtime_error("Data instance '" + name + "' has no table '" + n + "'");
        return it->second;
    }
};

class Interpreter {
public:
    Interpreter(std::unordered_map<std::string, FunctionDecl> fns,
                std::unordered_map<std::string, StructDecl> strs)
        : functions(std::move(fns)), structs(std::move(strs)) {
        scopes.push_back({});
    }

    Interpreter(std::unordered_map<std::string, FunctionDecl> fns,
                std::unordered_map<std::string, StructDecl> strs,
                std::unordered_map<std::string, DataDecl> das)
        : functions(std::move(fns)), structs(std::move(strs)) {
        scopes.push_back({});
        initDatabases(das);
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

    // Set to true whenever a Fail() is caught by a try/catch (see execute()'s
    // TryCatchStmt case). Unlike a generic Throw()/runtime error, catching
    // Fail() must still mark the enclosing Nodes{} node attempt as failed for
    // retry purposes -- this flag is how that side effect survives being
    // caught. Checked (and cleared) by runNodeWithRetries() after each
    // attempt completes, whether the attempt's top-level exception was caught
    // there or bubbled all the way up.
    bool pendingNodeFailFromCatch = false;

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
        if(b->op==TokType::EqualEqual || b->op==TokType::BangEqual) {
            Value L = evalToValue(b->left);
            Value R = evalToValue(b->right);
            bool eq = valuesEqual(L, R);
            return (b->op==TokType::EqualEqual ? eq : !eq) ? 1.0 : 0.0;
        }

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
        case TokType::EqualEqual:
        case TokType::BangEqual: {
            Value L = evalToValue(op->left);
            Value R = evalToValue(op->right);
            bool eq = valuesEqual(L, R);
            return op->op==TokType::EqualEqual ? eq : !eq;
        }
        case TokType::Less:
        case TokType::Greater:
        case TokType::LessEqual:
        case TokType::GreaterEqual: {
            double L = evalNumber(op->left);
            double R = evalNumber(op->right);
            switch(op->op) {
            case TokType::Less: return L<R;
            case TokType::Greater: return L>R;
            case TokType::LessEqual: return L<=R;
            case TokType::GreaterEqual: return L>=R;
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
    if(auto dm = std::dynamic_pointer_cast<DbMethodCallExpr>(expr)) return callDbMethod(dm);
    if(std::dynamic_pointer_cast<FailExpr>(expr)) throw NodeFailSignal{};
    if(auto th = std::dynamic_pointer_cast<ThrowExpr>(expr)) {
        std::string msg = valueToDisplayString(evalToValue(th->messageExpr));
        throw CnrThrowSignal{msg};
    }
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

// ----------------------------------------------------------------------------
// Database ("Data") support
// ----------------------------------------------------------------------------

// Merges each Data block's locally-declared structs into the global struct
// table, builds one DatabaseInstance per Data decl, tries to load any
// existing unencrypted .cnrdb file for it from disk, and binds it as a
// global variable under the Data block's name.
void initDatabases(const std::unordered_map<std::string, DataDecl>& das) {
    for(auto& kv : das) {
        const DataDecl& dd = kv.second;
        for(auto& s : dd.structs) {
            if(structs.count(s.first) && structs.at(s.first).name != s.second.name)
                throw std::runtime_error("Struct name '" + s.first + "' declared inside Data '" + dd.name + "' conflicts with an existing struct");
            structs[s.first] = s.second;
        }

        auto db = std::make_shared<DatabaseInstance>();
        db->name = dd.name;
        db->filePath = dd.fileNameLiteral;
        if(db->filePath.size() < 7 || db->filePath.substr(db->filePath.size()-7) != ".cnrdb")
            db->filePath += ".cnrdb";
        for(auto& td : dd.tables) {
            DbRecordTable t;
            t.structType = td.structType;
            t.primaryKeyField = td.primaryKeyField;
            db->tables[td.varName] = t;
            db->tableOrder.push_back(td.varName);
        }

        // Best-effort initial load: only succeeds automatically for
        // unencrypted files. An encrypted (.cnrdb produced after .encode())
        // file requires the program to call .encode(key) again before
        // .load()/auto-load can decrypt it -- see loadDatabaseFile().
        try {
            loadDatabaseFile(*db);
        } catch(...) {
            // No existing file yet, or it's encrypted and no key has been
            // set on this instance -- that's fine, it just starts empty.
        }

        Value v;
        v.isDatabase = true;
        v.database = db;
        scopes.back()[dd.name] = v;
    }
}

// ---- binary (de)serialization of one DatabaseInstance's tables ----------

static void writeU32(std::vector<uint8_t>& out, uint32_t x) {
    out.push_back((uint8_t)(x>>24)); out.push_back((uint8_t)(x>>16));
    out.push_back((uint8_t)(x>>8));  out.push_back((uint8_t)(x));
}
static uint32_t readU32(const std::vector<uint8_t>& in, size_t& pos) {
    if(pos+4 > in.size()) throw std::runtime_error("Corrupt database file (truncated)");
    uint32_t x = (uint32_t(in[pos])<<24)|(uint32_t(in[pos+1])<<16)|(uint32_t(in[pos+2])<<8)|uint32_t(in[pos+3]);
    pos += 4;
    return x;
}
static void writeStr(std::vector<uint8_t>& out, const std::string& s) {
    writeU32(out, (uint32_t)s.size());
    out.insert(out.end(), s.begin(), s.end());
}
static std::string readStr(const std::vector<uint8_t>& in, size_t& pos) {
    uint32_t len = readU32(in, pos);
    if(pos+len > in.size()) throw std::runtime_error("Corrupt database file (truncated string)");
    std::string s(in.begin()+pos, in.begin()+pos+len);
    pos += len;
    return s;
}

std::vector<uint8_t> serializeDatabase(const DatabaseInstance& db) {
    std::vector<uint8_t> out;
    out.push_back('C'); out.push_back('N'); out.push_back('R'); out.push_back('D');
    out.push_back(1); // version
    writeU32(out, (uint32_t)db.tableOrder.size());
    for(auto& tname : db.tableOrder) {
        const DbRecordTable& t = db.tables.at(tname);
        writeStr(out, tname);
        writeStr(out, t.structType);
        writeU32(out, (uint32_t)t.records.size());
        for(auto& rec : t.records) {
            const auto& fields = *rec.fields;
            writeU32(out, (uint32_t)fields.size());
            for(auto& fkv : fields) {
                writeStr(out, fkv.first);
                const Value& fv = fkv.second;
                if(fv.isString) {
                    out.push_back(1);
                    writeStr(out, fv.str);
                } else if(fv.isBool) {
                    out.push_back(2);
                    out.push_back(fv.boolean ? 1 : 0);
                } else if(fv.isArray) {
                    out.push_back(3);
                    writeU32(out, (uint32_t)fv.array.size());
                    for(double d : fv.array) {
                        uint64_t bits; std::memcpy(&bits, &d, 8);
                        for(int i=7;i>=0;--i) out.push_back((uint8_t)(bits >> (i*8)));
                    }
                } else {
                    out.push_back(0);
                    double d = fv.number;
                    uint64_t bits; std::memcpy(&bits, &d, 8);
                    for(int i=7;i>=0;--i) out.push_back((uint8_t)(bits >> (i*8)));
                }
            }
        }
    }
    return out;
}

void deserializeDatabase(DatabaseInstance& db, const std::vector<uint8_t>& in) {
    if(in.size() < 5 || in[0]!='C'||in[1]!='N'||in[2]!='R'||in[3]!='D')
        throw std::runtime_error("Not a valid .cnrdb file");
    size_t pos = 5; // skip magic + version
    uint32_t tableCount = readU32(in, pos);
    for(uint32_t i=0;i<tableCount;++i) {
        std::string tname = readStr(in, pos);
        std::string structType = readStr(in, pos);
        uint32_t recCount = readU32(in, pos);
        DbRecordTable t;
        t.structType = structType;
        {
            auto existing = db.tables.find(tname);
            if(existing != db.tables.end()) t.primaryKeyField = existing->second.primaryKeyField;
        }
        for(uint32_t r=0;r<recCount;++r) {
            Value rec;
            rec.isStruct = true;
            rec.structType = structType;
            rec.fields = std::make_shared<std::unordered_map<std::string, Value>>();
            uint32_t fcount = readU32(in, pos);
            for(uint32_t f=0; f<fcount; ++f) {
                std::string fname = readStr(in, pos);
                if(pos >= in.size()) throw std::runtime_error("Corrupt database file (truncated field)");
                uint8_t tag = in[pos++];
                Value fv;
                if(tag==1) {
                    fv.isString = true;
                    fv.str = readStr(in, pos);
                } else if(tag==2) {
                    if(pos >= in.size()) throw std::runtime_error("Corrupt database file (truncated bool)");
                    fv.isBool = true;
                    fv.boolean = in[pos++] != 0;
                } else if(tag==3) {
                    fv.isArray = true;
                    uint32_t count = readU32(in, pos);
                    fv.array.reserve(count);
                    for(uint32_t e=0; e<count; ++e) {
                        if(pos+8 > in.size()) throw std::runtime_error("Corrupt database file (truncated array element)");
                        uint64_t bits = 0;
                        for(int i=0;i<8;++i) bits = (bits<<8) | in[pos++];
                        double d; std::memcpy(&d, &bits, 8);
                        fv.array.push_back(d);
                    }
                } else {
                    if(pos+8 > in.size()) throw std::runtime_error("Corrupt database file (truncated number)");
                    uint64_t bits = 0;
                    for(int i=0;i<8;++i) bits = (bits<<8) | in[pos++];
                    double d; std::memcpy(&d, &bits, 8);
                    fv.number = d;
                }
                (*rec.fields)[fname] = fv;
            }
            t.records.push_back(rec);
        }
        t.rebuildPkIndex();
        db.tables[tname] = t;
        if(std::find(db.tableOrder.begin(), db.tableOrder.end(), tname) == db.tableOrder.end())
            db.tableOrder.push_back(tname);
    }
}

void saveDatabaseFile(DatabaseInstance& db) {
    std::vector<uint8_t> payload = serializeDatabase(db);
    std::string tmpPath = db.filePath + ".tmp";
    {
        std::ofstream out(tmpPath, std::ios::binary | std::ios::trunc);
        if(!out) throw std::runtime_error("Cannot write database file '" + db.filePath + "'");
        if(db.hasKey) {
            uint8_t iv[16];
            std::random_device rd;
            for(int i=0;i<16;++i) iv[i] = (uint8_t)(rd() & 0xFF);
            std::vector<uint8_t> cipher = cnrcrypto::aesCbcEncrypt(payload, db.key, iv);
            out.put('C'); out.put('N'); out.put('R'); out.put('E'); out.put((char)1);
            out.write((const char*)iv, 16);
            out.write((const char*)cipher.data(), (std::streamsize)cipher.size());
        } else {
            out.write((const char*)payload.data(), (std::streamsize)payload.size());
        }
        if(!out) throw std::runtime_error("Failed writing database file '" + db.filePath + "'");
    }
    // Atomic on POSIX: rename() replaces the destination in a single syscall,
    // so any other process/thread opening db.filePath always sees either the
    // old complete file or the new complete file, never a half-written one.
    if(std::rename(tmpPath.c_str(), db.filePath.c_str()) != 0)
        throw std::runtime_error("Failed to finalize database file '" + db.filePath + "' (rename from temp file failed)");
}

void loadDatabaseFile(DatabaseInstance& db) {
    std::ifstream in(db.filePath, std::ios::binary);
    if(!in) return; // no file yet -- fine, tables start empty
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if(raw.empty()) return;
    if(raw.size() >= 5 && raw[0]=='C'&&raw[1]=='N'&&raw[2]=='R'&&raw[3]=='E') {
        if(!db.hasKey)
            throw std::runtime_error("Database file '" + db.filePath + "' is encrypted; call .encode(\"key\") with the correct key before .load()");
        if(raw.size() < 21) throw std::runtime_error("Corrupt encrypted database file '" + db.filePath + "'");
        uint8_t iv[16];
        std::memcpy(iv, raw.data()+5, 16);
        std::vector<uint8_t> cipher(raw.begin()+21, raw.end());
        std::vector<uint8_t> payload = cnrcrypto::aesCbcDecrypt(cipher, db.key, iv);
        deserializeDatabase(db, payload);
    } else {
        deserializeDatabase(db, raw);
    }
}

// ---- Data1.encode(...) / Data1.table.push/find/delete/insert/save/load/count(...) ----

Value callDbMethod(const std::shared_ptr<DbMethodCallExpr>& dm) {
    Value& dbVal = resolveVar(dm->dataName);
    if(!dbVal.isDatabase)
        throw std::runtime_error("'" + dm->dataName + "' is not a Data instance");
    DatabaseInstance& db = *dbVal.database;
    // Every Data/table operation on this instance is serialized through one
    // mutex, so concurrent Parallel{}/thread() blocks pushing/deleting on the
    // same Data instance can't race on pkIndex or interleave writes to the
    // .cnrdb file. Evaluating argument expressions can itself call back into
    // the interpreter (e.g. another expression touching the same or a
    // different Data instance), so the lock is scoped as narrowly as
    // possible around each operation's actual table mutation + save.
    std::lock_guard<std::mutex> lock(*db.mtx);

    if(dm->tableName.empty()) {
        // Methods on the Data object itself.
        if(dm->method == "encode") {
            if(dm->args.size() != 1)
                throw std::runtime_error("Data.encode() expects 1 argument (the key)");
            std::string keyStr = valueToDisplayString(evalToValue(dm->args[0]));
            db.key = cnrcrypto::deriveKey(keyStr);
            db.hasKey = true;
            // Per spec: setting a key makes decode automatic from here on.
            // If the file on disk is already encrypted, try to load it now
            // with the freshly-set key so existing data becomes visible.
            loadDatabaseFile(db);
            for(auto& tname : db.tableOrder) db.tables[tname].rebuildPkIndex();
            saveDatabaseFile(db); // re-persist (encrypted) immediately
            return Value::makeNull();
        }
        if(dm->method == "save") { saveDatabaseFile(db); return Value::makeNull(); }
        if(dm->method == "load") {
            loadDatabaseFile(db);
            for(auto& tname : db.tableOrder) db.tables[tname].rebuildPkIndex();
            return Value::makeNull();
        }
        throw std::runtime_error("Unknown Data method '" + dm->method + "'");
    }

    DbRecordTable& table = db.table(dm->tableName);
    auto sIt = structs.find(table.structType);
    if(sIt == structs.end())
        throw std::runtime_error("Table '" + dm->tableName + "' has unknown struct type '" + table.structType + "'");

    // Looks up the field named table.primaryKeyField on a struct-instance
    // record and returns its display-string value. Throws if the table has
    // no primary key configured, since callers only reach here after
    // checking !table.primaryKeyField.empty().
    auto pkOf = [&](const Value& rec) -> std::string {
        auto it = rec.fields->find(table.primaryKeyField);
        if(it == rec.fields->end())
            throw std::runtime_error("Record is missing primary key field '" + table.primaryKeyField + "'");
        return valueToDisplayString(it->second);
    };

    if(dm->method == "push") {
        std::vector<Value> args;
        args.reserve(dm->args.size());
        for(auto& a : dm->args) args.push_back(evalToValue(a));
        Value rec = instantiateStruct(sIt->second, args);
        if(!table.primaryKeyField.empty()) {
            std::string pk = pkOf(rec);
            if(table.pkIndex.count(pk))
                throw std::runtime_error("table.push(): duplicate primary key '" + pk + "' for field '" + table.primaryKeyField + "' in table '" + dm->tableName + "'");
            table.pkIndex[pk] = table.records.size();
        }
        table.records.push_back(rec);
        saveDatabaseFile(db);
        Value ret; ret.number = (double)table.records.size(); return ret;
    }
    if(dm->method == "find") {
        if(dm->args.size() != 1)
            throw std::runtime_error("table.find() expects 1 argument");
        std::string needle = valueToDisplayString(evalToValue(dm->args[0]));
        Value result; result.isArray = true;
        for(size_t i=0;i<table.records.size();++i) {
            bool matched = false;
            for(auto& fkv : *table.records[i].fields) {
                std::string fieldStr = valueToDisplayString(fkv.second);
                if(fieldStr.find(needle) != std::string::npos) { matched = true; break; }
            }
            if(matched) result.array.push_back((double)i);
        }
        return result;
    }
    // findWhere(fieldName, value) -- exact match on one specific field only,
    // unlike find() which substring-matches across every field. Returns the
    // indices of matching records.
    if(dm->method == "findWhere") {
        if(dm->args.size() != 2)
            throw std::runtime_error("table.findWhere() expects 2 arguments (fieldName, value)");
        std::string fieldName = valueToDisplayString(evalToValue(dm->args[0]));
        std::string target = valueToDisplayString(evalToValue(dm->args[1]));
        Value result; result.isArray = true;
        for(size_t i=0;i<table.records.size();++i) {
            auto it = table.records[i].fields->find(fieldName);
            if(it == table.records[i].fields->end()) continue;
            if(valueToDisplayString(it->second) == target) result.array.push_back((double)i);
        }
        return result;
    }
    if(dm->method == "findById") {
        if(table.primaryKeyField.empty())
            throw std::runtime_error("table.findById(): table '" + dm->tableName + "' has no primaryKey declared");
        if(dm->args.size() != 1)
            throw std::runtime_error("table.findById() expects 1 argument (the primary key value)");
        std::string pk = valueToDisplayString(evalToValue(dm->args[0]));
        auto it = table.pkIndex.find(pk);
        if(it == table.pkIndex.end()) { Value v; v.number = -1.0; return v; }
        Value v; v.number = (double)it->second; return v;
    }
    // get(index) -- returns the full record (a struct Value, so its fields
    // are readable via ordinary dot access, e.g. UsersDB.users.get(0).name)
    // at a given row index, as returned by find()/findWhere()/orderBy().
    if(dm->method == "get") {
        if(dm->args.size() != 1)
            throw std::runtime_error("table.get() expects 1 argument (the index)");
        int idx = (int)evalNumber(dm->args[0]);
        if(idx < 0 || idx >= (int)table.records.size())
            throw std::runtime_error("table.get(): index out of bounds: " + std::to_string(idx));
        return table.records[idx];
    }
    // getById(pk) -- returns the full record (struct Value) whose primary
    // key matches, or null if no such record exists.
    if(dm->method == "getById") {
        if(table.primaryKeyField.empty())
            throw std::runtime_error("table.getById(): table '" + dm->tableName + "' has no primaryKey declared");
        if(dm->args.size() != 1)
            throw std::runtime_error("table.getById() expects 1 argument (the primary key value)");
        std::string pk = valueToDisplayString(evalToValue(dm->args[0]));
        auto it = table.pkIndex.find(pk);
        if(it == table.pkIndex.end()) return Value::makeNull();
        return table.records[it->second];
    }
    if(dm->method == "delete") {
        if(dm->args.size() != 1)
            throw std::runtime_error("table.delete() expects 1 argument (the index)");
        int idx = (int)evalNumber(dm->args[0]);
        if(idx < 0 || idx >= (int)table.records.size())
            throw std::runtime_error("table.delete(): index out of bounds: " + std::to_string(idx));
        table.records.erase(table.records.begin()+idx);
        table.rebuildPkIndex();
        saveDatabaseFile(db);
        return Value::makeNull();
    }
    // deleteWhere(fieldName, value) -- deletes every record whose field
    // matches exactly. Removes in a single pass (back-to-front) so earlier
    // indices stay valid while erasing.
    if(dm->method == "deleteWhere") {
        if(dm->args.size() != 2)
            throw std::runtime_error("table.deleteWhere() expects 2 arguments (fieldName, value)");
        std::string fieldName = valueToDisplayString(evalToValue(dm->args[0]));
        std::string target = valueToDisplayString(evalToValue(dm->args[1]));
        int deleted = 0;
        for(int i=(int)table.records.size()-1; i>=0; --i) {
            auto it = table.records[i].fields->find(fieldName);
            if(it == table.records[i].fields->end()) continue;
            if(valueToDisplayString(it->second) == target) {
                table.records.erase(table.records.begin()+i);
                deleted++;
            }
        }
        if(deleted > 0) { table.rebuildPkIndex(); saveDatabaseFile(db); }
        Value v; v.number = (double)deleted; return v;
    }
    if(dm->method == "deleteById") {
        if(table.primaryKeyField.empty())
            throw std::runtime_error("table.deleteById(): table '" + dm->tableName + "' has no primaryKey declared");
        if(dm->args.size() != 1)
            throw std::runtime_error("table.deleteById() expects 1 argument (the primary key value)");
        std::string pk = valueToDisplayString(evalToValue(dm->args[0]));
        auto it = table.pkIndex.find(pk);
        if(it == table.pkIndex.end())
            throw std::runtime_error("table.deleteById(): no record with " + table.primaryKeyField + "='" + pk + "'");
        table.records.erase(table.records.begin() + it->second);
        table.rebuildPkIndex();
        saveDatabaseFile(db);
        return Value::makeNull();
    }
    if(dm->method == "insert") {
        if(dm->args.size() != 3)
            throw std::runtime_error("table.insert() expects 3 arguments (index, fieldName, newValue)");
        int idx = (int)evalNumber(dm->args[0]);
        if(idx < 0 || idx >= (int)table.records.size())
            throw std::runtime_error("table.insert(): index out of bounds: " + std::to_string(idx));
        std::string fieldName = valueToDisplayString(evalToValue(dm->args[1]));
        Value newVal = evalToValue(dm->args[2]);
        auto& fields = *table.records[idx].fields;
        auto fit = fields.find(fieldName);
        if(fit == fields.end())
            throw std::runtime_error("table.insert(): struct '" + table.structType + "' has no field '" + fieldName + "'");
        if(!table.primaryKeyField.empty() && fieldName == table.primaryKeyField)
            throw std::runtime_error("table.insert(): cannot modify primary key field '" + fieldName + "' via insert(); use updateById() on a non-key field instead");
        fit->second = newVal;
        saveDatabaseFile(db);
        return Value::makeNull();
    }
    // updateWhere(fieldName, matchValue, targetField, newValue) -- updates
    // targetField on every record whose fieldName equals matchValue.
    if(dm->method == "updateWhere") {
        if(dm->args.size() != 4)
            throw std::runtime_error("table.updateWhere() expects 4 arguments (fieldName, matchValue, targetField, newValue)");
        std::string fieldName = valueToDisplayString(evalToValue(dm->args[0]));
        std::string matchValue = valueToDisplayString(evalToValue(dm->args[1]));
        std::string targetField = valueToDisplayString(evalToValue(dm->args[2]));
        Value newVal = evalToValue(dm->args[3]);
        if(!table.primaryKeyField.empty() && targetField == table.primaryKeyField)
            throw std::runtime_error("table.updateWhere(): cannot modify primary key field '" + targetField + "'");
        int updated = 0;
        for(auto& rec : table.records) {
            auto it = rec.fields->find(fieldName);
            if(it == rec.fields->end()) continue;
            if(valueToDisplayString(it->second) != matchValue) continue;
            auto tit = rec.fields->find(targetField);
            if(tit == rec.fields->end())
                throw std::runtime_error("table.updateWhere(): struct '" + table.structType + "' has no field '" + targetField + "'");
            tit->second = newVal;
            updated++;
        }
        if(updated > 0) saveDatabaseFile(db);
        Value v; v.number = (double)updated; return v;
    }
    if(dm->method == "updateById") {
        if(table.primaryKeyField.empty())
            throw std::runtime_error("table.updateById(): table '" + dm->tableName + "' has no primaryKey declared");
        if(dm->args.size() != 3)
            throw std::runtime_error("table.updateById() expects 3 arguments (idValue, fieldName, newValue)");
        std::string pk = valueToDisplayString(evalToValue(dm->args[0]));
        std::string fieldName = valueToDisplayString(evalToValue(dm->args[1]));
        Value newVal = evalToValue(dm->args[2]);
        if(fieldName == table.primaryKeyField)
            throw std::runtime_error("table.updateById(): cannot modify primary key field '" + fieldName + "'");
        auto it = table.pkIndex.find(pk);
        if(it == table.pkIndex.end())
            throw std::runtime_error("table.updateById(): no record with " + table.primaryKeyField + "='" + pk + "'");
        auto& fields = *table.records[it->second].fields;
        auto fit = fields.find(fieldName);
        if(fit == fields.end())
            throw std::runtime_error("table.updateById(): struct '" + table.structType + "' has no field '" + fieldName + "'");
        fit->second = newVal;
        saveDatabaseFile(db);
        return Value::makeNull();
    }
    // orderBy(fieldName, "asc"|"desc") -- returns record indices sorted by
    // that field's value (numeric fields sort numerically, everything else
    // sorts as text).
    if(dm->method == "orderBy") {
        if(dm->args.size() < 1 || dm->args.size() > 2)
            throw std::runtime_error("table.orderBy() expects 1 or 2 arguments (fieldName[, \"asc\"|\"desc\"])");
        std::string fieldName = valueToDisplayString(evalToValue(dm->args[0]));
        bool descending = false;
        if(dm->args.size() == 2) {
            std::string dir = valueToDisplayString(evalToValue(dm->args[1]));
            if(dir == "desc") descending = true;
            else if(dir != "asc")
                throw std::runtime_error("table.orderBy(): direction must be \"asc\" or \"desc\", got '" + dir + "'");
        }
        std::vector<size_t> idx(table.records.size());
        for(size_t i=0;i<idx.size();++i) idx[i]=i;
        std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
            auto ia = table.records[a].fields->find(fieldName);
            auto ib = table.records[b].fields->find(fieldName);
            if(ia == table.records[a].fields->end() || ib == table.records[b].fields->end())
                throw std::runtime_error("table.orderBy(): struct '" + table.structType + "' has no field '" + fieldName + "'");
            const Value& va = ia->second; const Value& vb = ib->second;
            bool lt;
            if(!va.isString && !vb.isString) lt = va.number < vb.number;
            else lt = valueToDisplayString(va) < valueToDisplayString(vb);
            return descending ? !lt && !(valueToDisplayString(va)==valueToDisplayString(vb)) : lt;
        });
        Value result; result.isArray = true;
        for(size_t i : idx) result.array.push_back((double)i);
        return result;
    }
    if(dm->method == "save") { saveDatabaseFile(db); return Value::makeNull(); }
    if(dm->method == "load") { loadDatabaseFile(db); table.rebuildPkIndex(); return Value::makeNull(); }
    if(dm->method == "count") { Value v; v.number = (double)table.records.size(); return v; }
    throw std::runtime_error("Unknown table method '" + dm->method + "'");
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

enum class NodeStatus { Pending, Success, Failed, Skipped };

// Runs one node's body up to (maxRetries + 1) times. Fail() inside the body
// aborts that attempt (caught as NodeFailSignal) and counts as a failed
// attempt; any other uncaught exception is also treated as a failed attempt
// (so a runtime error inside a node doesn't crash the whole workflow thread --
// it's just recorded as that node failing, same as an explicit Fail()).
// Returns true if the node ultimately succeeded.
bool runNodeWithRetries(const NodeDecl& node, std::string& outErrorMsg) {
    int attempts = node.maxRetries + 1;
    for(int attempt = 0; attempt < attempts; ++attempt) {
        // Fresh scope per attempt (and per node): matches function-call
        // scoping semantics -- locals declared with `var` inside the node
        // body don't leak into sibling nodes or outer code, but reads/writes
        // of names not declared locally fall through to outer/global scopes
        // via the normal scope-chain lookup in lookupVar/resolveVar.
        scopes.push_back({});
        pendingNodeFailFromCatch = false;
        try {
            executeBlock(node.body);
            scopes.pop_back();
            if(pendingNodeFailFromCatch) {
                // Fail() was raised and caught by a try/catch inside the body
                // (so the body ran to completion normally), but Fail() still
                // marks this attempt as failed for retry purposes.
                pendingNodeFailFromCatch = false;
                outErrorMsg = "Fail() called (caught internally)";
                continue;
            }
            return true;
        } catch(NodeFailSignal&) {
            scopes.pop_back();
            outErrorMsg = "Fail() called";
        } catch(ReturnSignal&) {
            // A bare `return;` inside a node body ends that attempt successfully
            // without propagating out of the workflow (Nodes bodies aren't functions).
            scopes.pop_back();
            return true;
        } catch(CnrThrowSignal& t) {
            scopes.pop_back();
            outErrorMsg = t.message;
        } catch(const std::exception& e) {
            scopes.pop_back();
            outErrorMsg = e.what();
        } catch(...) {
            scopes.pop_back();
            outErrorMsg = "unknown error in node";
        }
    }
    return false;
}

// Executes a whole `Nodes Name { ... }` statement to completion: computes
// dependency waves, runs each wave's nodes concurrently as threads (sharing
// this Interpreter's live scope stack for outer-variable access -- no
// synchronization beyond what Parallel{} already provides, by design), and
// skips any node whose dependencies didn't all succeed.
void runNodesWorkflow(const std::shared_ptr<NodesStmt>& s) {
    std::unordered_map<std::string, NodeStatus> status;
    std::unordered_map<std::string, std::string> errorMsgs;
    for(auto& n : s->nodes) status[n.name] = NodeStatus::Pending;

    std::mutex statusMtx;

    // Compute dependency waves via repeated topological "ready" passes.
    // Cycle detection: if a full pass makes no progress, remaining nodes
    // form a cycle (or depend on one), which is a program error.
    std::vector<std::vector<const NodeDecl*>> waves;
    std::vector<const NodeDecl*> remaining;
    for(auto& n : s->nodes) remaining.push_back(&n);

    while(!remaining.empty()) {
        std::vector<const NodeDecl*> ready;
        std::vector<const NodeDecl*> stillRemaining;
        for(auto* n : remaining) {
            bool allDepsScheduled = true;
            for(auto& dep : n->dependsOn) {
                bool scheduled = false;
                for(auto& wave : waves) for(auto* wn : wave) if(wn->name == dep) scheduled = true;
                if(!scheduled) { allDepsScheduled = false; break; }
            }
            if(allDepsScheduled) ready.push_back(n);
            else stillRemaining.push_back(n);
        }
        if(ready.empty())
            throw std::runtime_error("Nodes '" + s->workflowName + "': circular dependency detected among remaining nodes");
        waves.push_back(ready);
        remaining = stillRemaining;
    }

    for(auto& wave : waves) {
        std::vector<std::thread> workers;
        for(auto* nodePtr : wave) {
            const NodeDecl& node = *nodePtr;

            // Determine eligibility up front (dependency success check),
            // outside the thread, so Skipped nodes don't spawn a thread at all.
            bool eligible = true;
            for(auto& dep : node.dependsOn) {
                std::lock_guard<std::mutex> lock(statusMtx);
                if(status[dep] != NodeStatus::Success) { eligible = false; break; }
            }
            if(!eligible) {
                std::lock_guard<std::mutex> lock(statusMtx);
                status[node.name] = NodeStatus::Skipped;
                continue;
            }

            workers.emplace_back([this, &node, &status, &statusMtx, &errorMsgs]() {
                std::string errMsg;
                bool ok = runNodeWithRetries(node, errMsg);
                std::lock_guard<std::mutex> lock(statusMtx);
                status[node.name] = ok ? NodeStatus::Success : NodeStatus::Failed;
                if(!ok) errorMsgs[node.name] = errMsg;
            });
        }
        for(auto& w : workers) w.join();
    }
}

void execute(const StmtPtr& stmt)
{
    if(auto s = std::dynamic_pointer_cast<TryCatchStmt>(stmt)) {
        // ReturnSignal is intentionally NOT caught here -- `return;` inside a
        // try block still returns from the enclosing function normally.
        try {
            executeBlock(s->tryBlock);
        } catch(ReturnSignal&) {
            throw;
        } catch(NodeFailSignal&) {
            // Fail() caught by a try/catch inside a Nodes node body: run the
            // catch block as normal, but remember that Fail() happened so the
            // enclosing node attempt still gets marked failed for retries.
            pendingNodeFailFromCatch = true;
            scopes.push_back({});
            scopes.back()[s->catchVarName] = Value::makeString("Fail() called");
            executeBlock(s->catchBlock);
            scopes.pop_back();
        } catch(CnrThrowSignal& t) {
            scopes.push_back({});
            scopes.back()[s->catchVarName] = Value::makeString(t.message);
            executeBlock(s->catchBlock);
            scopes.pop_back();
        } catch(const std::exception& e) {
            scopes.push_back({});
            scopes.back()[s->catchVarName] = Value::makeString(e.what());
            executeBlock(s->catchBlock);
            scopes.pop_back();
        }
        return;
    }
    if(auto s = std::dynamic_pointer_cast<NodesStmt>(stmt)) {
        runNodesWorkflow(s);
        return;
    }
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
    Interpreter interpreter(program.functions, program.structs, program.datas);
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