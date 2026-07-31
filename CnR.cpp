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
#include <cmath>

enum class TokType {
    Number, Ident, String,
    Var, Print, If, Else, While, For, TrueKw, FalseKw, CharKw, LenKw, StringKw,
    Function, Return, StructKw,
    Parallel, ThreadKw, JoinKw, JoinAllKw,
    HttpKw, HeaderKw, BodyKw,
    NodesKw, OnFailKw, RetryKw, FailKw,
    TryKw, CatchKw, ThrowKw,
    DataKw, TableKw,
    IntKw, LongKw, FloatKw, BigIntKw, BigFloatKw,
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
        if (s == "Int") return {TokType::IntKw,s,0,startLine};
        if (s == "long") return {TokType::LongKw,s,0,startLine};
        if (s == "Float") return {TokType::FloatKw,s,0,startLine};
        if (s == "BigInt") return {TokType::BigIntKw,s,0,startLine};
        if (s == "BigFloat" || s == "bigDouble") return {TokType::BigFloatKw,s,0,startLine};
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

// Tag enums used for O(1) AST dispatch (see ExprKind/StmtKind below). Every
// concrete Expr/Stmt subclass sets its `kind` in its base-class constructor
// call, once, at construction time. Hot dispatch functions (evalNumber,
// evalBool, evalToValue, execute, ...) then `switch` on this tag and
// `static_cast` to the already-known concrete type instead of trying a long
// chain of std::dynamic_pointer_cast<T> (which is RTTI-based and, per
// profiling, was the single largest CPU cost in the interpreter). The
// static_cast is safe precisely because `kind` is set once at construction
// and never changes, so it always matches the object's real dynamic type.
enum class ExprKind {
    Number, Bool, Var, Unary, Binary, CharCast, StringCast, IntCast, LongCast,
    FloatCast, BigIntCast, BigFloatCast, StringLit, JsonObjectLit, HttpCall,
    Len, ArrayAccess, TensorAccess, Call, MemberAccess, Thread, Join, JoinAll,
    ArrayMethodCall, Fail, Throw, ServerConfig, ObjectMethodCall, DbMethodCall
};
enum class StmtKind {
    VarDecl, Assign, ArrayAssign, MemberAssign, Print, Block, If, While, For,
    Return, ExprS, Parallel, Nodes, TryCatch, RouteDecl, ServerStart
};

struct Expr { ExprKind kind; explicit Expr(ExprKind k):kind(k){} virtual ~Expr() = default; };
struct Stmt { StmtKind kind; explicit Stmt(StmtKind k):kind(k){} virtual ~Stmt() = default; };
using ExprPtr = std::shared_ptr<Expr>;
using StmtPtr = std::shared_ptr<Stmt>;

struct NumberExpr : Expr { double value; std::string text; NumberExpr(double v):Expr(ExprKind::Number),value(v){} NumberExpr(double v, std::string t):Expr(ExprKind::Number),value(v),text(std::move(t)){} };
struct BoolExpr : Expr { bool value; BoolExpr(bool v):Expr(ExprKind::Bool),value(v){} };
struct VarExpr : Expr { std::string name; VarExpr(std::string n):Expr(ExprKind::Var),name(std::move(n)){} };
struct UnaryExpr : Expr { TokType op; ExprPtr expr; UnaryExpr(TokType o, ExprPtr e):Expr(ExprKind::Unary),op(o),expr(std::move(e)){} };
struct BinaryExpr : Expr { TokType op; ExprPtr left; ExprPtr right; BinaryExpr(TokType o, ExprPtr l, ExprPtr r):Expr(ExprKind::Binary),op(o),left(std::move(l)),right(std::move(r)){} };
struct CharCastExpr : Expr { ExprPtr expr; CharCastExpr(ExprPtr e):Expr(ExprKind::CharCast),expr(std::move(e)){} };
struct StringCastExpr : Expr { ExprPtr expr; StringCastExpr(ExprPtr e):Expr(ExprKind::StringCast),expr(std::move(e)){} };
struct IntCastExpr : Expr { ExprPtr expr; IntCastExpr(ExprPtr e):Expr(ExprKind::IntCast),expr(std::move(e)){} };
struct LongCastExpr : Expr { ExprPtr expr; LongCastExpr(ExprPtr e):Expr(ExprKind::LongCast),expr(std::move(e)){} };
struct FloatCastExpr : Expr { ExprPtr expr; FloatCastExpr(ExprPtr e):Expr(ExprKind::FloatCast),expr(std::move(e)){} };
struct BigIntCastExpr : Expr { ExprPtr expr; BigIntCastExpr(ExprPtr e):Expr(ExprKind::BigIntCast),expr(std::move(e)){} };
// precisionExpr == nullptr means "use the default 22-digit precision", i.e.
// a plain (BigFloat)expr cast. (BigFloat(N))expr instead carries a
// precision expression (N), evaluated at cast time, giving that value up
// to 1000 exact fractional digits.
struct BigFloatCastExpr : Expr { ExprPtr expr; ExprPtr precisionExpr; BigFloatCastExpr(ExprPtr e, ExprPtr p=nullptr):Expr(ExprKind::BigFloatCast),expr(std::move(e)),precisionExpr(std::move(p)){} };
struct StringLitExpr : Expr { std::string value; StringLitExpr(std::string v):Expr(ExprKind::StringLit),value(std::move(v)){} };

struct JsonObjectLitExpr : Expr { std::vector<std::pair<ExprPtr,ExprPtr>> entries; JsonObjectLitExpr():Expr(ExprKind::JsonObjectLit){} };

struct HttpCallExpr : Expr {
    std::string method;
    ExprPtr url;
    std::vector<std::pair<ExprPtr,ExprPtr>> headers;
    ExprPtr bodyExpr; // null if no body{} block was given
    HttpCallExpr():Expr(ExprKind::HttpCall){}
};
struct LenExpr : Expr { std::string arrayName; LenExpr(std::string n):Expr(ExprKind::Len),arrayName(std::move(n)){} };
struct ArrayAccessExpr : Expr { std::string arrayName; ExprPtr index; ArrayAccessExpr(std::string n, ExprPtr i):Expr(ExprKind::ArrayAccess),arrayName(std::move(n)),index(std::move(i)){} };
// Chained index access for Matrix/Tensor values, e.g. m[1][0] or t[0][1][2].
// A full index (indices.size()==dims.size()) returns the scalar element; a
// partial index returns the sub-slice at that index (still a Matrix/Tensor
// Value with one fewer dimension).
struct TensorAccessExpr : Expr { std::string arrayName; std::vector<ExprPtr> indices; TensorAccessExpr():Expr(ExprKind::TensorAccess){} };
// Recursive literal for var[]...[] = {{...},{...}} initializers. A "leaf"
// node (isLeaf==true) holds flat scalar expressions (the innermost row);
// otherwise children holds one nested literal per element at this depth.
struct NestedArrayLitExpr {
    bool isLeaf = false;
    std::vector<ExprPtr> leafValues;
    std::vector<std::shared_ptr<NestedArrayLitExpr>> children;
};
struct CallExpr : Expr { std::string name; std::vector<ExprPtr> args; CallExpr():Expr(ExprKind::Call){} };
struct MemberAccessExpr : Expr { ExprPtr base; std::string member; ExprPtr index; MemberAccessExpr():Expr(ExprKind::MemberAccess){} };

struct ThreadExpr : Expr { std::string fnName; std::vector<ExprPtr> args; ThreadExpr():Expr(ExprKind::Thread){} };
struct JoinExpr : Expr { ExprPtr handleExpr; JoinExpr():Expr(ExprKind::Join){} };
struct JoinAllExpr : Expr { std::string arrayName; JoinAllExpr():Expr(ExprKind::JoinAll){} };

enum class ArrayMethod { Push, Pop, Sort, Reverse, Contains, IndexOf, Accumulate };
struct ArrayMethodCallExpr : Expr { std::string arrayName; ArrayMethod method; ExprPtr arg; ArrayMethodCallExpr():Expr(ExprKind::ArrayMethodCall){} };

struct VarDeclStmt : Stmt {
    bool isArray = false; bool isEmptyArray = false; std::string name; ExprPtr value; std::vector<ExprPtr> arrayValues;
    // tensorRank>0 means this was declared with 2+ bracket pairs, e.g.
    // var[][] m = {...}; (rank 2, a Matrix) or var[][][] t = {...}; (rank 3,
    // a Tensor). rank==1 (plain var[] arr = {...};) keeps using arrayValues
    // above unchanged. nestedInit holds the {{...},{...}} literal tree.
    int tensorRank = 0;
    std::shared_ptr<NestedArrayLitExpr> nestedInit;
    VarDeclStmt():Stmt(StmtKind::VarDecl){}
};
struct AssignStmt : Stmt { std::string name; ExprPtr value; AssignStmt():Stmt(StmtKind::Assign){} };
// index is used for plain 1-D arrays (arr[i] = v;), kept for backward
// compatibility. indices (2+ entries) is used for Matrix/Tensor chained
// assignment, e.g. m[1][0] = v; or t[0][1][2] = v;
struct ArrayAssignStmt : Stmt { std::string arrayName; ExprPtr index; ExprPtr value; std::vector<ExprPtr> indices; ArrayAssignStmt():Stmt(StmtKind::ArrayAssign){} };
struct MemberAssignStmt : Stmt { std::string objectName; std::string member; ExprPtr index; ExprPtr value; MemberAssignStmt():Stmt(StmtKind::MemberAssign){} };
struct PrintStmt : Stmt { bool printChar=false; bool printCharArray=false; ExprPtr expr; std::string arrayName; PrintStmt():Stmt(StmtKind::Print){} };
struct BlockStmt : Stmt { std::vector<StmtPtr> statements; BlockStmt():Stmt(StmtKind::Block){} };
struct IfStmt : Stmt { ExprPtr condition; std::shared_ptr<BlockStmt> thenBlock; std::shared_ptr<BlockStmt> elseBlock; IfStmt():Stmt(StmtKind::If){} };
struct WhileStmt : Stmt { ExprPtr condition; std::shared_ptr<BlockStmt> body; WhileStmt():Stmt(StmtKind::While){} };
struct ForStmt : Stmt { StmtPtr init; ExprPtr condition; StmtPtr increment; std::shared_ptr<BlockStmt> body; ForStmt():Stmt(StmtKind::For){} };
struct ReturnStmt : Stmt { ExprPtr value; ReturnStmt():Stmt(StmtKind::Return){} };
struct ExprStmt : Stmt { ExprPtr expr; ExprStmt():Stmt(StmtKind::ExprS){} };
struct ParallelStmt : Stmt { std::vector<std::shared_ptr<BlockStmt>> blocks; ParallelStmt():Stmt(StmtKind::Parallel){} };

// --- DAG / Nodes workflow ---

// Fail(); -- only meaningful inside a Nodes{} node body. Aborts the current
// attempt of the enclosing node (counts as a failed attempt; retried if the
// node's OnFail(Retry=N) budget allows).
struct FailExpr : Expr { FailExpr():Expr(ExprKind::Fail){} };

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
    NodesStmt():Stmt(StmtKind::Nodes){}
};

// Throw("message"); -- raises a catchable runtime error carrying a string
// message. Distinct from Fail(), which stays specific to Nodes{} node
// bodies and always marks the enclosing node attempt as failed even when
// caught by a try/catch inside that same body.
struct ThrowExpr : Expr { ExprPtr messageExpr; ThrowExpr():Expr(ExprKind::Throw){} };

// try { ... } catch(var e) { ... } -- generic exception handling. Catches:
//   - Throw("msg") raised anywhere inside the try block
//   - ordinary runtime errors (std::runtime_error) raised inside the try block
//   - Fail() raised inside the try block (the catch body still runs, but
//     Fail() additionally marks the enclosing Nodes node attempt as failed
//     for retry purposes -- see CnrThrowSignal/NodeFailSignal handling in
//     the interpreter)
// A `return;` inside try still returns from the enclosing function
// normally, propagated via ExecResult rather than an exception.
// `catchVarName` is always present syntactically (catch(var e)) and is
// bound to the error message as a string inside catchBlock.
struct TryCatchStmt : Stmt {
    std::shared_ptr<BlockStmt> tryBlock;
    std::string catchVarName;
    std::shared_ptr<BlockStmt> catchBlock;
    TryCatchStmt():Stmt(StmtKind::TryCatch){}
};

// --- HTTP server AST ---

// Server() { host = "0.0.0.0"; port = 8080; ... } -- a config object literal,
// parsed the same way struct-ctor-like blocks are: "key = expr;" lines.
// Recognized by the identifier "Server" (soft keyword, like "push"/"header").
struct ServerConfigExpr : Expr {
    std::vector<std::pair<std::string, ExprPtr>> entries;
    ServerConfigExpr():Expr(ExprKind::ServerConfig){}
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
    RouteDeclStmt():Stmt(StmtKind::RouteDecl){}
};

// server.start(); -- begins accepting connections (blocking call).
struct ServerStartStmt : Stmt {
    std::string serverName;
    ServerStartStmt():Stmt(StmtKind::ServerStart){}
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
    ObjectMethodCallExpr():Expr(ExprKind::ObjectMethodCall){}
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
    DbMethodCallExpr():Expr(ExprKind::DbMethodCall){}
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
        if(check(TokType::Number)) { Token t = advance(); return std::make_shared<NumberExpr>(t.number, t.text); }
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
            if(match(TokType::IntKw)) {
                expect(TokType::RParen,")");
                return std::make_shared<IntCastExpr>(parseUnary());
            }
            if(match(TokType::LongKw)) {
                expect(TokType::RParen,")");
                return std::make_shared<LongCastExpr>(parseUnary());
            }
            if(match(TokType::FloatKw)) {
                expect(TokType::RParen,")");
                return std::make_shared<FloatCastExpr>(parseUnary());
            }
            if(match(TokType::BigIntKw)) {
                expect(TokType::RParen,")");
                return std::make_shared<BigIntCastExpr>(parseUnary());
            }
            if(match(TokType::BigFloatKw)) {
                ExprPtr precisionExpr;
                if(match(TokType::LParen)) {
                    precisionExpr = parseExpression();
                    expect(TokType::RParen,")");
                }
                expect(TokType::RParen,")");
                return std::make_shared<BigFloatCastExpr>(parseUnary(), precisionExpr);
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
        // BigFloat(N) used standalone (not as a (BigFloat(N))expr cast) --
        // e.g. `var x = BigFloat(50);` -- builds a zero-valued BigDecimal at
        // N fractional digits. BigFloat() / BigFloat with no call at all is
        // handled by the (BigFloat)expr / (BigFloat(N))expr cast forms above;
        // this covers using BigFloat itself as a value-producing call.
        if(check(TokType::BigFloatKw) && peek(1).type == TokType::LParen) {
            advance(); // BigFloat
            advance(); // (
            ExprPtr precisionExpr;
            if(!check(TokType::RParen)) precisionExpr = parseExpression();
            expect(TokType::RParen,")");
            return std::make_shared<BigFloatCastExpr>(std::make_shared<NumberExpr>(0.0, "0"), precisionExpr);
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
                if(check(TokType::LBracket)) {
                    // Chained index (m[1][0], t[0][1][2], ...) -- a Matrix/
                    // Tensor access. Collect every [expr] in the chain.
                    auto ta = std::make_shared<TensorAccessExpr>();
                    ta->arrayName = name;
                    ta->indices.push_back(idx);
                    while(match(TokType::LBracket)) {
                        ta->indices.push_back(parseExpression());
                        expect(TokType::RBracket,"]");
                    }
                    return ta;
                }
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

    // Counts and consumes consecutive [] pairs (0 or more), used to detect
    // Matrix (rank 2, var[][] m) / Tensor (rank 3+, var[][][] t) declarations.
    // Only bare [] pairs count -- a [expr] is left alone for the caller.
    int consumeBracketPairs() {
        int n = 0;
        while(check(TokType::LBracket) && peek(1).type == TokType::RBracket) {
            advance(); advance();
            n++;
        }
        return n;
    }

    // Parses a {...} literal at nesting depth `depth` (depth==1 is the
    // innermost row of flat scalar expressions; depth>1 is a list of
    // depth-1 nested literals). Used for var[]...[] = {{...},{...}}.
    std::shared_ptr<NestedArrayLitExpr> parseNestedArrayLit(int depth) {
        auto node = std::make_shared<NestedArrayLitExpr>();
        expect(TokType::LBrace,"{");
        if(depth <= 1) {
            node->isLeaf = true;
            if(!check(TokType::RBrace)) {
                while(true) {
                    node->leafValues.push_back(parseExpression());
                    if(match(TokType::Comma)) continue;
                    break;
                }
            }
        } else {
            node->isLeaf = false;
            if(!check(TokType::RBrace)) {
                while(true) {
                    node->children.push_back(parseNestedArrayLit(depth - 1));
                    if(match(TokType::Comma)) continue;
                    break;
                }
            }
        }
        expect(TokType::RBrace,"}");
        return node;
    }

    StmtPtr parseVarDecl(bool consumeSemicolon = true) {
        expect(TokType::Var,"var");
        auto stmt = std::make_shared<VarDeclStmt>();
        if(check(TokType::LBracket) && peek(1).type == TokType::RBracket) {
            int rank = consumeBracketPairs();
            stmt->name = expect(TokType::Ident,"identifier").text;
            stmt->isArray = true;
            if(rank >= 2) {
                stmt->tensorRank = rank;
                expect(TokType::Assign,"=");
                stmt->nestedInit = parseNestedArrayLit(rank);
                if(consumeSemicolon) expect(TokType::Semicolon,";");
                return stmt;
            }
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
        if(check(TokType::LBracket) && peek(1).type == TokType::RBracket) {
            int rank = consumeBracketPairs();
            stmt->isArray = true;
            if(rank >= 2) {
                stmt->tensorRank = rank;
                expect(TokType::Assign,"=");
                stmt->nestedInit = parseNestedArrayLit(rank);
                if(consumeSemicolon) expect(TokType::Semicolon,";");
                return stmt;
            }
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
            if(check(TokType::LBracket)) {
                // Chained index assignment: m[1][0] = v; / t[0][1][2] = v;
                auto stmt = std::make_shared<ArrayAssignStmt>();
                stmt->arrayName = id.text;
                stmt->indices.push_back(idx);
                while(match(TokType::LBracket)) {
                    stmt->indices.push_back(parseExpression());
                    expect(TokType::RBracket,"]");
                }
                expect(TokType::Assign,"=");
                stmt->value=parseExpression();
                if(consumeSemicolon) expect(TokType::Semicolon,";");
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

// Arbitrary-precision signed integer for the BigInt type. Stored in base 10,
// little-endian (digits[0] is the units digit) -- simplest to get correct,
// not the fastest representation, but this language cares about correctness
// over speed for a feature like this.
struct BigInt {
    bool negative = false;
    std::vector<uint8_t> d; // base-10 digits, little-endian

    BigInt() { d.push_back(0); }

    static BigInt fromInt64(long long v) {
        BigInt b;
        b.d.clear();
        b.negative = v < 0;
        unsigned long long uv = b.negative ? (unsigned long long)(-(v+1)) + 1 : (unsigned long long)v;
        if(uv == 0) b.d.push_back(0);
        while(uv > 0) { b.d.push_back((uint8_t)(uv % 10)); uv /= 10; }
        if(b.d.empty()) b.d.push_back(0);
        b.trim();
        return b;
    }
    static BigInt fromString(const std::string& sIn) {
        BigInt b;
        b.d.clear();
        std::string s = sIn;
        size_t i = 0;
        if(!s.empty() && (s[0]=='+'||s[0]=='-')) { b.negative = (s[0]=='-'); i = 1; }
        if(i >= s.size()) throw std::runtime_error("Invalid BigInt literal: '" + sIn + "'");
        for(char c : s.substr(i)) {
            if(!std::isdigit((unsigned char)c)) throw std::runtime_error("Invalid BigInt literal: '" + sIn + "'");
        }
        for(size_t k = s.size(); k > i; --k) b.d.push_back((uint8_t)(s[k-1]-'0'));
        b.trim();
        return b;
    }
    void trim() {
        while(d.size() > 1 && d.back() == 0) d.pop_back();
        if(d.size()==1 && d[0]==0) negative = false;
    }
    bool isZero() const { return d.size()==1 && d[0]==0; }
    std::string toString() const {
        std::string s;
        if(negative && !isZero()) s += '-';
        for(size_t k = d.size(); k > 0; --k) s += (char)('0' + d[k-1]);
        return s;
    }
    double toDouble() const {
        double r = 0;
        for(size_t k = d.size(); k > 0; --k) r = r*10.0 + d[k-1];
        return negative ? -r : r;
    }
    static int cmpAbs(const BigInt& a, const BigInt& b) {
        if(a.d.size() != b.d.size()) return a.d.size() < b.d.size() ? -1 : 1;
        for(size_t k = a.d.size(); k > 0; --k) {
            if(a.d[k-1] != b.d[k-1]) return a.d[k-1] < b.d[k-1] ? -1 : 1;
        }
        return 0;
    }
    static BigInt addAbs(const BigInt& a, const BigInt& b) {
        BigInt r; r.d.assign(std::max(a.d.size(),b.d.size())+1, 0);
        int carry = 0;
        for(size_t i = 0; i < r.d.size(); ++i) {
            int s = carry;
            if(i < a.d.size()) s += a.d[i];
            if(i < b.d.size()) s += b.d[i];
            r.d[i] = (uint8_t)(s % 10);
            carry = s / 10;
        }
        r.trim();
        return r;
    }
    static BigInt subAbs(const BigInt& a, const BigInt& b) { // requires |a| >= |b|
        BigInt r; r.d.assign(a.d.size(), 0);
        int borrow = 0;
        for(size_t i = 0; i < a.d.size(); ++i) {
            int s = a.d[i] - borrow - (i < b.d.size() ? b.d[i] : 0);
            if(s < 0) { s += 10; borrow = 1; } else borrow = 0;
            r.d[i] = (uint8_t)s;
        }
        r.trim();
        return r;
    }
    BigInt operator-() const { BigInt r = *this; if(!r.isZero()) r.negative = !r.negative; return r; }
    BigInt operator+(const BigInt& o) const {
        if(negative == o.negative) { BigInt r = addAbs(*this,o); r.negative = negative; r.trim(); return r; }
        if(cmpAbs(*this,o) >= 0) { BigInt r = subAbs(*this,o); r.negative = negative; r.trim(); return r; }
        BigInt r = subAbs(o,*this); r.negative = o.negative; r.trim(); return r;
    }
    BigInt operator-(const BigInt& o) const { return *this + (-o); }
    BigInt operator*(const BigInt& o) const {
        BigInt r; r.d.assign(d.size()+o.d.size(), 0);
        for(size_t i = 0; i < d.size(); ++i) {
            if(d[i]==0) continue;
            int carry = 0;
            for(size_t j = 0; j < o.d.size() || carry; ++j) {
                int cur = r.d[i+j] + carry + (j < o.d.size() ? d[i]*o.d[j] : 0);
                r.d[i+j] = (uint8_t)(cur % 10);
                carry = cur / 10;
            }
        }
        r.negative = (negative != o.negative);
        r.trim();
        return r;
    }
    // Schoolbook long division on decimal digits; returns quotient, writes remainder into rem.
    static BigInt divModAbs(const BigInt& a, const BigInt& b, BigInt& rem) {
        if(b.isZero()) throw std::runtime_error("BigInt division by zero");
        BigInt q; q.d.assign(a.d.size(), 0);
        BigInt cur; cur.d = {0};
        for(size_t k = a.d.size(); k > 0; --k) {
            // cur = cur*10 + a.d[k-1]
            cur.d.insert(cur.d.begin(), a.d[k-1]);
            cur.trim();
            int lo = 0, hi = 9, best = 0;
            while(lo <= hi) {
                int mid = (lo+hi)/2;
                BigInt t = b; t.negative = false;
                BigInt prod = t * fromInt64(mid);
                if(cmpAbs(prod, cur) <= 0) { best = mid; lo = mid+1; } else hi = mid-1;
            }
            q.d[k-1] = (uint8_t)best;
            BigInt t = b; t.negative = false;
            cur = subAbs(cur, t * fromInt64(best));
        }
        q.trim();
        rem = cur; rem.negative = false; rem.trim();
        return q;
    }
    BigInt operator/(const BigInt& o) const {
        BigInt rem;
        BigInt q = divModAbs(*this, o, rem);
        q.negative = (negative != o.negative) && !q.isZero();
        return q;
    }
    BigInt operator%(const BigInt& o) const {
        BigInt rem;
        divModAbs(*this, o, rem);
        rem.negative = negative && !rem.isZero(); // result follows dividend's sign, like C++ %
        return rem;
    }
    bool operator==(const BigInt& o) const { return negative==o.negative && d==o.d; }
    bool operator<(const BigInt& o) const {
        if(negative != o.negative) return negative;
        int c = cmpAbs(*this,o);
        return negative ? c > 0 : c < 0;
    }
    bool operator<=(const BigInt& o) const { return *this < o || *this == o; }
    bool operator>(const BigInt& o) const { return o < *this; }
    bool operator>=(const BigInt& o) const { return o <= *this; }
};

// Real, arbitrary-precision BigDecimal: exact decimal arithmetic (no binary
// rounding at all) backed by a BigInt mantissa plus a decimal exponent, and
// carrying its own working precision (how many fractional digits it keeps
// after divide/sqrt/transcendental ops -- multiply/add/sub stay exact up to
// `scale`). `scale`/`precision` are settable per-value via BigFloat(N) (max
// 1000 digits); BigFloat with no argument defaults to 22 fractional digits,
// which is also where the named constants (pi/e/phi/psi) live.
static const int CNR_BIGDEC_MAX_SCALE = 1000;
static const int CNR_BIGDEC_DEFAULT_SCALE = 22;

struct BigDecimal {
    // Value == mantissa * 10^-scale (mantissa is an exact BigInt integer).
    BigInt mantissa;
    int scale = CNR_BIGDEC_DEFAULT_SCALE; // fractional digits kept in `mantissa`
    int precision = CNR_BIGDEC_DEFAULT_SCALE; // working precision for this value (BigFloat(N))

    BigDecimal() { mantissa = BigInt::fromInt64(0); }

    static int clampPrecision(int p) {
        if(p < 0) p = 0;
        if(p > CNR_BIGDEC_MAX_SCALE) p = CNR_BIGDEC_MAX_SCALE;
        return p;
    }

    static BigInt pow10(int n) {
        BigInt r = BigInt::fromInt64(1);
        BigInt ten = BigInt::fromInt64(10);
        for(int i=0;i<n;++i) r = r * ten;
        return r;
    }

    static BigInt rescaleUp(const BigInt& mant, int fromScale, int toScale) {
        if(toScale <= fromScale) return mant;
        return mant * pow10(toScale - fromScale);
    }

    // Builds a BigDecimal from a long double. A long double's *exact* value
    // (its binary fraction) has far more decimal digits than are actually
    // meaningful -- but naively multiplying by 10 `scale` times accumulates
    // floating-point rounding error at every single multiplication, which
    // silently manufactures garbage digits from roughly the 17th digit
    // onward. Instead we ask the C library for the long double's own decimal
    // expansion once, with %.*Lf (correctly rounded by the libc printf
    // implementation), and pad the remainder with zeros -- so only the
    // ~18-19 digits a long double actually carries are trusted; anything
    // beyond that is explicit, honest zero-padding rather than fabricated
    // noise. This matters most for iterative algorithms like Newton's-method
    // sqrt, which need every digit of the seed value to be trustworthy.
    static BigDecimal fromLongDouble(long double v, int precision) {
        BigDecimal r;
        r.precision = clampPrecision(precision);
        char buf[1200];
        // 20 fractional digits comfortably covers a long double's real
        // precision (~18-19 significant decimal digits) without over-claiming.
        int fracDigits = std::min(r.precision, 20);
        snprintf(buf, sizeof(buf), "%.*Lf", fracDigits, v);
        return fromString(std::string(buf), r.precision);
    }

    // Parse a decimal literal string like "123.456" or "-0.5" at the given precision.
    static BigDecimal fromString(const std::string& sIn, int precision) {
        BigDecimal r;
        r.precision = clampPrecision(precision);
        std::string s = sIn;
        bool neg = false;
        size_t i = 0;
        if(!s.empty() && (s[0]=='+'||s[0]=='-')) { neg = (s[0]=='-'); i = 1; }
        std::string intPart, fracPart;
        size_t dot = s.find('.', i);
        if(dot == std::string::npos) {
            intPart = s.substr(i);
        } else {
            intPart = s.substr(i, dot - i);
            fracPart = s.substr(dot+1);
        }
        if(intPart.empty()) intPart = "0";
        int fracLen = (int)fracPart.size();
        r.scale = std::max(r.precision, fracLen);
        std::string digits = intPart + fracPart;
        for(int k=0; k < r.scale - fracLen; ++k) digits += '0';
        r.mantissa = BigInt::fromString(digits);
        r.mantissa.negative = neg && !r.mantissa.isZero();
        return r;
    }

    bool isZero() const { return mantissa.isZero(); }
    bool isNegative() const { return mantissa.negative; }

    std::string toString() const {
        BigInt m = mantissa;
        bool neg = m.negative;
        m.negative = false;
        std::string digits = m.toString();
        if((int)digits.size() <= scale) digits = std::string(scale - digits.size() + 1, '0') + digits;
        std::string intPart = digits.substr(0, digits.size() - scale);
        std::string fracPart = scale > 0 ? digits.substr(digits.size() - scale) : "";
        // Trim trailing zeros in the fractional part for display, but keep
        // at least one digit if there is a fractional part at all.
        if(!fracPart.empty()) {
            size_t last = fracPart.find_last_not_of('0');
            fracPart = (last == std::string::npos) ? "" : fracPart.substr(0, last+1);
        }
        std::string out = intPart;
        if(!fracPart.empty()) out += "." + fracPart;
        if(neg && (intPart != "0" || !fracPart.empty())) out = "-" + out;
        return out;
    }

    long double toLongDouble() const {
        std::string s = toString();
        return strtold(s.c_str(), nullptr);
    }

    static void align(const BigDecimal& a, const BigDecimal& b, BigInt& am, BigInt& bm, int& outScale) {
        outScale = std::max(a.scale, b.scale);
        am = rescaleUp(a.mantissa, a.scale, outScale);
        bm = rescaleUp(b.mantissa, b.scale, outScale);
    }

    BigDecimal add(const BigDecimal& o) const {
        BigDecimal r; BigInt am, bm; int sc;
        align(*this, o, am, bm, sc);
        r.mantissa = am + bm;
        r.scale = sc;
        r.precision = std::max(precision, o.precision);
        return r;
    }
    BigDecimal sub(const BigDecimal& o) const {
        BigDecimal r; BigInt am, bm; int sc;
        align(*this, o, am, bm, sc);
        r.mantissa = am - bm;
        r.scale = sc;
        r.precision = std::max(precision, o.precision);
        return r;
    }
    BigDecimal mul(const BigDecimal& o) const {
        BigDecimal r;
        r.mantissa = mantissa * o.mantissa;
        r.scale = scale + o.scale;
        r.precision = std::max(precision, o.precision);
        r.roundToPrecision(r.precision);
        return r;
    }
    // Long division carried out to `precision` fractional digits.
    BigDecimal div(const BigDecimal& o) const {
        if(o.isZero()) throw std::runtime_error("Division by zero");
        int p = std::max(precision, o.precision);
        BigDecimal r;
        r.precision = p;
        r.scale = p;
        // (mantissa / 10^scale) / (o.mantissa / 10^o.scale)
        //   = mantissa * 10^(o.scale - scale + p) / o.mantissa, truncated at scale p
        int shift = o.scale - scale + p;
        BigInt num = mantissa;
        if(shift >= 0) num = num * pow10(shift);
        BigInt den = o.mantissa;
        den.negative = false;
        BigInt numAbs = num; numAbs.negative = false;
        if(shift < 0) den = den * pow10(-shift);
        BigInt rem;
        BigInt q = BigInt::divModAbs(numAbs, den, rem);
        q.negative = (mantissa.negative != o.mantissa.negative) && !q.isZero();
        r.mantissa = q;
        return r;
    }
    BigDecimal mod(const BigDecimal& o) const {
        if(o.isZero()) throw std::runtime_error("Modulo by zero");
        // a % b == a - trunc(a/b)*b, matching C's fmod-style truncation semantics.
        BigDecimal q = this->div(o);
        BigDecimal qTrunc = q.truncatedToInteger();
        BigDecimal prod = qTrunc.mul(o);
        BigDecimal r = this->sub(prod);
        return r;
    }
    BigDecimal truncatedToInteger() const {
        BigDecimal r;
        r.precision = precision;
        r.scale = scale;
        BigInt m = mantissa; m.negative = false;
        std::string digits = m.toString();
        if((int)digits.size() <= scale) { r.mantissa = BigInt::fromInt64(0); return r; }
        std::string intPart = digits.substr(0, digits.size()-scale);
        BigInt truncMant = BigInt::fromString(intPart) * pow10(scale);
        truncMant.negative = mantissa.negative && !truncMant.isZero();
        r.mantissa = truncMant;
        return r;
    }
    void roundToPrecision(int p) {
        if(scale <= p) return;
        int drop = scale - p;
        BigInt divisor = pow10(drop);
        BigInt rem;
        BigInt m = mantissa; bool neg = m.negative; m.negative = false;
        BigInt q = BigInt::divModAbs(m, divisor, rem);
        // round half up
        BigInt twiceRem = rem + rem;
        if(BigInt::cmpAbs(twiceRem, divisor) >= 0) q = q + BigInt::fromInt64(1);
        q.negative = neg && !q.isZero();
        mantissa = q;
        scale = p;
    }
    BigDecimal negate() const {
        BigDecimal r = *this;
        if(!r.isZero()) r.mantissa.negative = !r.mantissa.negative;
        return r;
    }
    BigDecimal abs() const {
        BigDecimal r = *this;
        r.mantissa.negative = false;
        return r;
    }
    static int compare(const BigDecimal& a, const BigDecimal& b) {
        BigInt am, bm; int sc;
        align(a, b, am, bm, sc);
        if(am == bm) return 0;
        return am < bm ? -1 : 1;
    }
    bool isIntegerValued() const {
        BigInt m = mantissa; m.negative = false;
        std::string digits = m.toString();
        if((int)digits.size() <= scale) return digits.find_first_not_of('0') == std::string::npos;
        std::string frac = digits.substr(digits.size()-scale);
        return frac.find_first_not_of('0') == std::string::npos;
    }
    // Newton's method square root at `precision` fractional digits.
    BigDecimal sqrtNewton() const {
        if(isNegative()) throw std::runtime_error("sqrt(): cannot take the square root of a negative number");
        if(isZero()) return *this;
        int p = precision;
        int workScale = p + 15; // guard digits, absorbs rounding noise from intermediate divisions
        long double approx = std::sqrt((double)toLongDouble());
        if(approx <= 0) approx = 1.0L;
        BigDecimal x = BigDecimal::fromLongDouble(approx, workScale);
        BigDecimal self = *this; self.precision = workScale;
        BigDecimal two = BigDecimal::fromLongDouble(2.0L, workScale);
        // Newton's method roughly doubles the number of correct digits each
        // iteration. The initial guess (from a double) has ~15 correct
        // digits, so ceil(log2(workScale/15)) + a few extra passes is always
        // enough to converge fully to workScale digits, however large p is.
        int neededIters = 8;
        {
            int correct = 15;
            while(correct < workScale) { correct *= 2; neededIters++; }
        }
        for(int iter=0; iter<neededIters; ++iter) {
            BigDecimal quotient = self.div(x);
            quotient.precision = workScale;
            BigDecimal sum = x.add(quotient);
            sum.precision = workScale;
            BigDecimal next = sum.div(two);
            next.precision = workScale;
            next.roundToPrecision(workScale);
            if(BigDecimal::compare(next, x) == 0) { x = next; break; }
            x = next;
        }
        x.precision = p;
        x.roundToPrecision(p);
        return x;
    }
};

// Which numeric flavor a Value currently holds. Plain is the language's
// original, untyped double -- everything old keeps working exactly as
// before. The others are opt-in via (Int)/(long)/(Float)/(BigInt)/(BigFloat)
// casts and propagate through arithmetic (see numericBinaryOp).
enum class NumKind { Plain, F32, I32, I64, BigF, Big };

// ---- Gargalo #2 fix: cold/rarely-used Value data lives here, allocated
// lazily (only the first time it's actually needed). Structs, plain
// objects, matrices/tensors stay on Value directly (they're common in
// everyday CnR programs), but the "exotic" kinds -- server/database
// handles, typed-object maps, big-number backing -- are used by only a
// small fraction of Values in most programs (e.g. a pure-arithmetic
// script never touches any of them). Before this change every single
// Value paid for 7 shared_ptr members + 1 std::string unconditionally on
// every construction/copy/move/destruction; now the common case (plain
// number/bool/string/array) carries just one null shared_ptr for all of
// them combined.
struct Value; // forward declaration: ValueExtra holds containers of Value

struct ValueExtra {
    bool isThread = false;
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
    std::shared_ptr<BigDecimal> bigDec;
    std::shared_ptr<BigInt> big;
};

struct Value {
    bool isArray = false;
    bool isBool = false;
    bool isStruct = false;
    bool isString = false;
    bool isObject = false;
    bool isNull = false;
    double number = 0;
    bool boolean = false;
    std::string str;
    std::vector<double> array;

    // Matrix/Tensor shape: empty means this is a plain 1-D array (existing
    // behavior, unchanged). A non-empty dims (size>=2) means `array` holds
    // dims[0]*dims[1]*...*dims[n-1] elements in row-major order -- e.g.
    // dims=={2,3} is a 2x3 matrix, dims=={2,2,2} is a 2x2x2 tensor.
    std::vector<int> dims;

    // Typed-numeric extension: numKind==Plain means "number" holds the value
    // exactly as before (untyped double), so every existing var/expr keeps
    // working unchanged. Int/long live in i64 (exact); BigInt lives in big
    // (exact, unbounded); Float/BigFloat live in number/bigFlt respectively
    // (rounded through float / long double precision).
    NumKind numKind = NumKind::Plain;
    long long i64 = 0;

    // Lazily-allocated cold data (see ValueExtra above). ex() allocates on
    // first use (call at write sites); cex() never allocates, so pure
    // reads on a Value that never touched any of these fields stay free --
    // they just see the "all defaults" state.
    std::shared_ptr<ValueExtra> extra;
    ValueExtra& ex() { if (!extra) extra = std::make_shared<ValueExtra>(); return *extra; }
    const ValueExtra& cex() const { static const ValueExtra kEmptyExtra{}; return extra ? *extra : kEmptyExtra; }

    static Value makeString(std::string s) { Value v; v.isString = true; v.str = std::move(s); return v; }
    static Value makeNull() { Value v; v.isNull = true; return v; }
    static Value makeObject() { Value v; v.isObject = true; v.ex().object = std::make_shared<std::unordered_map<std::string, Value>>(); return v; }
    static Value makeObjectArray() { Value v; v.isObject = true; v.ex().isObjectArray = true; v.ex().objectArray = std::make_shared<std::vector<Value>>(); return v; }
    static Value makeBool2(bool b) { Value v; v.isBool = true; v.boolean = b; return v; }
};

// ---- Matrix/Tensor helpers (row-major flat storage in Value::array) ----

// Total element count for a given shape (product of all dimensions).
int tensorElementCount(const std::vector<int>& dims) {
    int total = 1;
    for(int d : dims) total *= d;
    return total;
}

// Row-major flat offset for a full index (one int per dimension).
int tensorFlatOffset(const std::vector<int>& dims, const std::vector<int>& idx) {
    int offset = 0;
    for(size_t i = 0; i < dims.size(); ++i) {
        offset = offset * dims[i] + idx[i];
    }
    return offset;
}

// Extracts the sub-tensor at a partial index (fewer indices than dims),
// e.g. slicing row `i` out of a matrix, or a 2-D slice out of a 3-D tensor.
// Returns a new Value with dims trimmed to the remaining dimensions.
Value tensorSlice(const Value& src, const std::vector<int>& idx) {
    if(idx.size() > src.dims.size())
        throw std::runtime_error("Too many indices for tensor (has " + std::to_string(src.dims.size()) + " dimension(s))");
    std::vector<int> subDims(src.dims.begin() + idx.size(), src.dims.end());
    int subSize = tensorElementCount(subDims);
    std::vector<int> fullPrefix = idx;
    fullPrefix.resize(src.dims.size(), 0);
    int startOffset = tensorFlatOffset(src.dims, fullPrefix);
    Value out;
    out.isArray = true;
    out.dims = subDims;
    if(subDims.empty()) {
        // Fully indexed: a single scalar. Callers that need a scalar use
        // out.array[0] rather than this Value directly.
        out.array.assign(1, src.array[startOffset]);
    } else {
        out.array.assign(src.array.begin() + startOffset, src.array.begin() + startOffset + subSize);
    }
    return out;
}

// Writes a scalar into a tensor at a full index (one int per dimension).
void tensorSetScalar(Value& v, const std::vector<int>& idx, double scalar) {
    if(idx.size() != v.dims.size())
        throw std::runtime_error("Tensor assignment requires " + std::to_string(v.dims.size()) + " index/indices, got " + std::to_string(idx.size()));
    for(size_t i = 0; i < idx.size(); ++i) {
        if(idx[i] < 0 || idx[i] >= v.dims[i])
            throw std::runtime_error("Tensor index out of bounds at dimension " + std::to_string(i) + ": " + std::to_string(idx[i]));
    }
    v.array[tensorFlatOffset(v.dims, idx)] = scalar;
}

// Formats a Matrix/Tensor Value as nested brackets matching its shape, e.g.
// a 2x2 matrix prints as [[1,2],[3,4]].
void tensorToDisplayInto(std::ostringstream& oss, const std::vector<int>& dims, const double* data, size_t& cursor) {
    if(dims.empty()) {
        double d = data[cursor++];
        if(d == (long long)d) oss << (long long)d; else oss << d;
        return;
    }
    oss << '[';
    std::vector<int> innerDims(dims.begin() + 1, dims.end());
    for(int i = 0; i < dims[0]; ++i) {
        if(i > 0) oss << ',';
        tensorToDisplayInto(oss, innerDims, data, cursor);
    }
    oss << ']';
}
std::string tensorToDisplayString(const Value& v) {
    std::ostringstream oss;
    size_t cursor = 0;
    tensorToDisplayInto(oss, v.dims, v.array.data(), cursor);
    return oss.str();
}

std::string valueToDisplayString(const Value& v) {
    if(v.isNull) return "null";
    if(v.isString) return v.str;
    if(v.isBool) return v.boolean ? "true" : "false";
    if(v.isObject) return v.cex().isObjectArray ? "[array]" : "[object]";
    if(v.isStruct) return "[struct " + v.cex().structType + "]";
    if(v.cex().isDatabase) return "[database]";
    if(v.isArray && !v.dims.empty()) return tensorToDisplayString(v);
    if(v.isArray) return "[array]";
    if(v.numKind == NumKind::Big) return v.cex().big ? v.cex().big->toString() : "0";
    if(v.numKind == NumKind::I32 || v.numKind == NumKind::I64) return std::to_string(v.i64);
    if(v.numKind == NumKind::BigF) {
        return v.cex().bigDec ? v.cex().bigDec->toString() : "0";
    }
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
    if(a.isBool || b.isBool) {
        double av = a.isBool ? (a.boolean ? 1.0 : 0.0) : a.number;
        double bv = b.isBool ? (b.boolean ? 1.0 : 0.0) : b.number;
        return av == bv;
    }
    if(a.numKind == NumKind::Big || b.numKind == NumKind::Big) {
        BigInt av = a.numKind == NumKind::Big ? *a.cex().big : BigInt::fromInt64((long long)a.number);
        BigInt bv = b.numKind == NumKind::Big ? *b.cex().big : BigInt::fromInt64((long long)b.number);
        return av == bv;
    }
    if(a.numKind == NumKind::BigF || b.numKind == NumKind::BigF) {
        BigDecimal av = a.numKind == NumKind::BigF ? *a.cex().bigDec : BigDecimal::fromLongDouble((long double)a.number, CNR_BIGDEC_DEFAULT_SCALE);
        BigDecimal bv = b.numKind == NumKind::BigF ? *b.cex().bigDec : BigDecimal::fromLongDouble((long double)b.number, CNR_BIGDEC_DEFAULT_SCALE);
        return BigDecimal::compare(av, bv) == 0;
    }
    if((a.numKind == NumKind::I32 || a.numKind == NumKind::I64) &&
       (b.numKind == NumKind::I32 || b.numKind == NumKind::I64)) {
        return a.i64 == b.i64;
    }
    double av = a.number;
    double bv = b.number;
    return av == bv;
}

// ---- Typed-numeric helpers (Int/long/BigInt/Float/BigFloat) ----
// Every factory below also mirrors the value into the legacy `number` field
// so that all the pre-existing double-based code paths in this interpreter
// (array indices, http bodies, etc.) keep working when handed a typed value.
Value makeIntValue(long long v, bool wide) {
    Value r;
    r.numKind = wide ? NumKind::I64 : NumKind::I32;
    r.i64 = wide ? v : (long long)(int32_t)v;
    r.number = (double)r.i64;
    return r;
}
Value makeFloatValue(double v) {
    Value r;
    r.numKind = NumKind::F32;
    r.number = (double)(float)v;
    return r;
}
Value makeBigValue(BigInt v) {
    Value r;
    r.numKind = NumKind::Big;
    r.ex().big = std::make_shared<BigInt>(std::move(v));
    r.number = r.cex().big->toDouble();
    return r;
}
Value makeBigFloatValue(BigDecimal v) {
    Value r;
    r.numKind = NumKind::BigF;
    r.ex().bigDec = std::make_shared<BigDecimal>(std::move(v));
    r.number = (double)r.cex().bigDec->toLongDouble();
    return r;
}
// Compatibility overload: builds a BigDecimal at the default (22-digit)
// precision from a long double -- used by call sites that only have a
// machine-precision intermediate result (e.g. std::sqrt/std::sin fallbacks).
Value makeBigFloatValue(long double v) {
    return makeBigFloatValue(BigDecimal::fromLongDouble(v, CNR_BIGDEC_DEFAULT_SCALE));
}
// Builds a BigDecimal at a specific precision -- used by BigFloat(N) casts
// and literals so the requested number of digits (up to 1000) is honored.
Value makeBigFloatValue(long double v, int precision) {
    return makeBigFloatValue(BigDecimal::fromLongDouble(v, precision));
}
Value makePlainNumber(double v) { Value r; r.number = v; return r; }

// Named math constants, given here to 1000 significant fractional digits so
// that BigFloat(N) for any N up to 1000 can slice out an exact prefix instead
// of re-deriving them at runtime. Stored as decimal-string literals and
// parsed into a BigDecimal at whatever precision is requested (default 22,
// which is where `pi`/`e`/`phi`/`psi` live when used unqualified).
const char* CNR_PI_DIGITS =
"3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706798214808651328230664709384460955058223172535940812848111745028410270193852110555964462294895493038196"
"4428810975665933446128475648233786783165271201909145648566923460348610454326648213393607260249141273724587006606315588174881520920962829254091715364367892590360011330530548820466521384146951941511609";
const char* CNR_E_DIGITS =
"2.71828182845904523536028747135266249775724709369995957496696762772407663035354759457138217852516642742746639193200305992181741359662904357290033429526059563073813232862794349076323382988075319525101"
"9011573834187930702154089149934884167509244761460668082264800168477411853742345442437107539077744992069551702761838606261331384583000752044933826560297606737113200709328709127443747047230696977209310";
const char* CNR_PHI_DIGITS = // golden ratio (1+sqrt5)/2
"1.61803398874989484820458683436563811772030917980576286213544862270526046281890244970720720418939113748475408807538689175212663386222353693179318006076672635443338908659593958290563832266131992829026788"
"0675208429328856889398872842471309796739001248531447477074391088947627773217772210532032069658335980999479864530392936089192421778062772653830098980338956095050923751881108102679558728752778566118821454";
const char* CNR_PSI_DIGITS = // golden ratio conjugate (1-sqrt5)/2, i.e. 1 - phi
"-0.61803398874989484820458683436563811772030917980576286213544862270526046281890244970720720418939113748475408807538689175212663386222353693179318006076672635443338908659593958290563832266131992829026788"
"0675208429328856889398872842471309796739001248531447477074391088947627773217772210532032069658335980999479864530392936089192421778062772653830098980338956095050923751881108102679558728752778566118821454";

BigDecimal cnrConstantAtPrecision(const char* fullDigits, int precision) {
    int p = BigDecimal::clampPrecision(precision);
    std::string s(fullDigits);
    size_t dot = s.find('.');
    // fullDigits has (dot+1 .. end) fractional digits available; if the
    // caller asks for more than we've hardcoded, just hand back what we have
    // (still far beyond double precision) rather than fabricating digits.
    size_t maxFrac = s.size() - dot - 1;
    size_t want = std::min((size_t)p, maxFrac);
    std::string truncated = s.substr(0, dot + 1 + want);
    return BigDecimal::fromString(truncated, p);
}
bool valueIsIntegral(const Value& v) {
    if(v.isBool) return true;
    switch(v.numKind) {
        case NumKind::Big: case NumKind::I32: case NumKind::I64: return true;
        case NumKind::BigF: return v.cex().bigDec ? v.cex().bigDec->isIntegerValued() : true;
        default: return v.number == std::floor(v.number);
    }
}
long long valueToI64(const Value& v) {
    if(v.isBool) return v.boolean ? 1 : 0;
    switch(v.numKind) {
        case NumKind::I32: case NumKind::I64: return v.i64;
        case NumKind::Big: return (long long)v.cex().big->toDouble();
        case NumKind::BigF: return v.cex().bigDec ? (long long)v.cex().bigDec->toLongDouble() : 0;
        default: return (long long)v.number;
    }
}
long double valueToLongDouble(const Value& v) {
    if(v.isBool) return v.boolean ? 1.0L : 0.0L;
    switch(v.numKind) {
        case NumKind::BigF: return v.cex().bigDec ? v.cex().bigDec->toLongDouble() : 0.0L;
        case NumKind::Big: return (long double)v.cex().big->toDouble();
        case NumKind::I32: case NumKind::I64: return (long double)v.i64;
        default: return (long double)v.number;
    }
}
BigInt valueToBigInt(const Value& v) {
    if(v.numKind == NumKind::Big) return *v.cex().big;
    if(!valueIsIntegral(v))
        throw std::runtime_error("Cannot use a non-integer value as BigInt -- cast to BigFloat instead");
    if(v.numKind == NumKind::I32 || v.numKind == NumKind::I64) return BigInt::fromInt64(v.i64);
    if(v.numKind == NumKind::BigF && v.cex().bigDec) {
        BigInt rem;
        BigInt m = v.cex().bigDec->mantissa; bool neg = m.negative; m.negative = false;
        BigInt divisor = BigDecimal::pow10(v.cex().bigDec->scale);
        BigInt q = BigInt::divModAbs(m, divisor, rem); // exact since valueIsIntegral was checked above
        q.negative = neg && !q.isZero();
        return q;
    }
    return BigInt::fromInt64((long long)v.number);
}
// Working precision to use when a value needs to be brought into the
// BigDecimal domain for a mixed-kind operation: an existing BigFloat keeps
// its own precision; anything else falls back to the 22-digit default.
int valuePrecisionOrDefault(const Value& v) {
    return (v.numKind == NumKind::BigF && v.cex().bigDec) ? v.cex().bigDec->precision : CNR_BIGDEC_DEFAULT_SCALE;
}
// Converts any Value into an exact BigDecimal at the given precision. Used
// whenever an operation must run in the BigDecimal domain (arithmetic,
// comparisons, math builtins) so mixing e.g. a plain int literal with a
// BigFloat(500) doesn't get flattened down to double precision first.
BigDecimal valueToBigDecimal(const Value& v, int precision) {
    if(v.numKind == NumKind::BigF && v.cex().bigDec) {
        BigDecimal r = *v.cex().bigDec;
        r.precision = BigDecimal::clampPrecision(precision);
        return r;
    }
    if(v.numKind == NumKind::Big && v.cex().big) {
        BigDecimal r;
        r.precision = BigDecimal::clampPrecision(precision);
        r.scale = 0;
        r.mantissa = *v.cex().big;
        return r;
    }
    if(v.isBool) return BigDecimal::fromLongDouble(v.boolean ? 1.0L : 0.0L, precision);
    if(v.numKind == NumKind::I32 || v.numKind == NumKind::I64)
        return BigDecimal::fromLongDouble((long double)v.i64, precision);
    return BigDecimal::fromLongDouble((long double)v.number, precision);
}
// Ranks which numeric domain an arithmetic op should run in when the two
// operands don't match: BigInt beats everything (exact), then BigFloat
// (long double), then 64-bit int, then 32-bit int, then Float, then the
// original untyped double -- so mixing a typed value with a plain number
// promotes to the typed side instead of silently truncating it.
int numKindRank(NumKind k) {
    switch(k) {
        case NumKind::Big: return 5;
        case NumKind::BigF: return 4;
        case NumKind::I64: return 3;
        case NumKind::I32: return 2;
        case NumKind::F32: return 1;
        default: return 0;
    }
}

double valueToApproxDouble(const Value& v) {
    if(v.isBool) return v.boolean ? 1.0 : 0.0;
    switch(v.numKind) {
        case NumKind::Big: return v.cex().big->toDouble();
        case NumKind::BigF: return v.cex().bigDec ? (double)v.cex().bigDec->toLongDouble() : 0.0;
        case NumKind::I32: case NumKind::I64: return (double)v.i64;
        default: return v.number;
    }
}

// -1/0/1, comparing in whichever domain the higher-ranked operand needs.
int numericCompare(const Value& L, const Value& R) {
    NumKind kind = numKindRank(L.numKind) >= numKindRank(R.numKind) ? L.numKind : R.numKind;
    if(kind == NumKind::Big && valueIsIntegral(L) && valueIsIntegral(R)) {
        BigInt a = valueToBigInt(L), b = valueToBigInt(R);
        if(a < b) return -1; if(b < a) return 1; return 0;
    }
    if(kind == NumKind::Big || kind == NumKind::BigF) {
        int p = std::max(valuePrecisionOrDefault(L), valuePrecisionOrDefault(R));
        BigDecimal a = valueToBigDecimal(L, p), b = valueToBigDecimal(R, p);
        return BigDecimal::compare(a, b);
    }
    if(kind == NumKind::I64 || kind == NumKind::I32) {
        long long a = valueToI64(L), b = valueToI64(R);
        if(a < b) return -1; if(a > b) return 1; return 0;
    }
    double a = L.isBool ? (L.boolean?1.0:0.0) : L.number;
    double b = R.isBool ? (R.boolean?1.0:0.0) : R.number;
    if(a < b) return -1; if(a > b) return 1; return 0;
}

// +,-,*,/,% across mixed numeric kinds. The result's kind is the highest-
// ranked domain involved (see numKindRank), so e.g. Int + BigInt = BigInt,
// and an untagged literal mixed with a typed value is promoted rather than
// silently truncating the typed side.
Value numericBinaryOp(TokType op, const Value& L, const Value& R) {
    // String concatenation: "a" + "b", or "count: " + 5, or 5 + " items"
    // (either side may be a string -- the non-string side is stringified the
    // same way print() would display it). Only '+' does this; other
    // operators on a string operand fall through to the numeric paths below
    // and error out naturally (a string has no numeric value).
    if(op == TokType::Plus && (L.isString || R.isString)) {
        return Value::makeString(valueToDisplayString(L) + valueToDisplayString(R));
    }
    NumKind kind = numKindRank(L.numKind) >= numKindRank(R.numKind) ? L.numKind : R.numKind;
    if(kind == NumKind::Big && valueIsIntegral(L) && valueIsIntegral(R)) {
        BigInt a = valueToBigInt(L), b = valueToBigInt(R);
        switch(op) {
        case TokType::Plus: return makeBigValue(a + b);
        case TokType::Minus: return makeBigValue(a - b);
        case TokType::Star: return makeBigValue(a * b);
        case TokType::Slash:
            if(b.isZero()) throw std::runtime_error("Division by zero");
            return makeBigValue(a / b);
        case TokType::Percent:
            if(b.isZero()) throw std::runtime_error("Modulo by zero");
            return makeBigValue(a % b);
        default: throw std::runtime_error("Unsupported BigInt operator");
        }
    }
    if(kind == NumKind::Big || kind == NumKind::BigF) {
        int p = std::max(valuePrecisionOrDefault(L), valuePrecisionOrDefault(R));
        BigDecimal a = valueToBigDecimal(L, p), b = valueToBigDecimal(R, p);
        switch(op) {
        case TokType::Plus: return makeBigFloatValue(a.add(b));
        case TokType::Minus: return makeBigFloatValue(a.sub(b));
        case TokType::Star: return makeBigFloatValue(a.mul(b));
        case TokType::Slash:
            if(b.isZero()) throw std::runtime_error("Division by zero");
            return makeBigFloatValue(a.div(b));
        case TokType::Percent:
            if(b.isZero()) throw std::runtime_error("Modulo by zero");
            return makeBigFloatValue(a.mod(b));
        default: throw std::runtime_error("Unsupported BigFloat operator");
        }
    }
    if(kind == NumKind::I64 || kind == NumKind::I32) {
        long long a = valueToI64(L), b = valueToI64(R);
        bool wide = (kind == NumKind::I64);
        switch(op) {
        case TokType::Plus: return makeIntValue(a + b, wide);
        case TokType::Minus: return makeIntValue(a - b, wide);
        case TokType::Star: return makeIntValue(a * b, wide);
        case TokType::Slash:
            if(b == 0) throw std::runtime_error("Division by zero");
            return makeIntValue(a / b, wide);
        case TokType::Percent:
            if(b == 0) throw std::runtime_error("Modulo by zero");
            return makeIntValue(a % b, wide);
        default: throw std::runtime_error("Unsupported integer operator");
        }
    }
    if(kind == NumKind::F32) {
        double a = valueToLongDouble(L), b = valueToLongDouble(R);
        switch(op) {
        case TokType::Plus: return makeFloatValue(a + b);
        case TokType::Minus: return makeFloatValue(a - b);
        case TokType::Star: return makeFloatValue(a * b);
        case TokType::Slash:
            if(b == 0) throw std::runtime_error("Division by zero");
            return makeFloatValue(a / b);
        case TokType::Percent:
            if(b == 0) throw std::runtime_error("Modulo by zero");
            return makeFloatValue(fmod(a, b));
        default: throw std::runtime_error("Unsupported Float operator");
        }
    }
    // Plain: exact legacy double behavior, unchanged.
    double a = L.isBool ? (L.boolean?1.0:0.0) : L.number;
    double b = R.isBool ? (R.boolean?1.0:0.0) : R.number;
    Value v;
    switch(op) {
    case TokType::Plus: v.number = a + b; return v;
    case TokType::Minus: v.number = a - b; return v;
    case TokType::Star: v.number = a * b; return v;
    case TokType::Slash:
        if(b == 0) throw std::runtime_error("Division by zero");
        v.number = a / b; return v;
    case TokType::Percent:
        if((int)b == 0) throw std::runtime_error("Modulo by zero");
        v.number = (double)((int)a % (int)b); return v;
    default: throw std::runtime_error("Unsupported arithmetic operator");
    }
}

// Exact BigInt exponentiation (repeated squaring) -- used by pow() when the
// base is a BigInt and the exponent is a non-negative whole number, so
// pow((BigInt)2, 200) stays exact instead of falling back to long double.
Value bigPow(const BigInt& base, long long exp) {
    if(exp < 0) throw std::runtime_error("pow(): a BigInt base needs a non-negative exponent (cast to BigFloat for negative/fractional exponents)");
    BigInt result = BigInt::fromInt64(1);
    BigInt b = base;
    long long e = exp;
    while(e > 0) {
        if(e & 1) result = result * b;
        if(e > 1) b = b * b;
        e >>= 1;
    }
    return makeBigValue(result);
}

bool isMathBuiltinName(const std::string& name) {
    static const std::unordered_map<std::string,int> names = {
        {"sqrt",0},{"sqroot",0},{"pow",0},{"abs",0},{"floor",0},{"ceil",0},
        {"round",0},{"log",0},{"ln",0},{"log10",0},{"exp",0},
        {"sin",0},{"cos",0},{"tan",0},{"min",0},{"max",0},
        {"random",0},{"randomSeed",0}
    };
    return names.count(name) > 0;
}

// PRNG for random()/randomSeed() -- a plain std::mt19937 seeded from
// random_device by default (so successive random() calls differ run to
// run), or reseeded deterministically by randomSeed(n) when the caller
// wants reproducible sequences (e.g. reproducible NN weight init).
std::mt19937& cnrRngEngine() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}

// Built-in math functions. Available everywhere a call expression is,
// e.g. `sqrt(2)`, `pow(2, 10)`, `print(max(a, b))`. When an argument is a
// BigFloat, the result stays a real arbitrary-precision BigDecimal at that
// value's own precision (BigFloat(N) -> N fractional digits), instead of
// getting flattened down to a plain double. sqrt() runs Newton's method
// entirely in exact decimal arithmetic, so it honors the full precision
// (up to 1000 digits). log/log10/exp/sin/cos/tan still route through the
// C math library at long-double precision (~18-19 significant digits) and
// then get stored into a BigDecimal at the requested precision -- these
// are true transcendental functions and don't have a closed-form decimal
// algorithm here, so digits beyond long double's precision are not
// meaningful for those specific functions.
Value callMathBuiltin(const std::string& name, std::vector<Value>& args) {
    auto need = [&](size_t n) {
        if(args.size() != n)
            throw std::runtime_error("'" + name + "()' expects " + std::to_string(n) +
                                      " argument(s), got " + std::to_string(args.size()));
    };
    bool useBig = false;
    int bigPrecision = CNR_BIGDEC_DEFAULT_SCALE;
    for(auto& a : args) if(a.numKind == NumKind::BigF) { useBig = true; bigPrecision = std::max(bigPrecision, valuePrecisionOrDefault(a)); }

    if(name == "sqrt" || name == "sqroot") {
        need(1);
        if(useBig) {
            BigDecimal x = valueToBigDecimal(args[0], bigPrecision);
            return makeBigFloatValue(x.sqrtNewton());
        }
        long double x = valueToLongDouble(args[0]);
        if(x < 0) throw std::runtime_error("sqrt(): cannot take the square root of a negative number");
        long double r = std::sqrt(x);
        return makePlainNumber((double)r);
    }
    if(name == "pow") {
        need(2);
        if(args[0].numKind == NumKind::Big && valueIsIntegral(args[1]) && valueToLongDouble(args[1]) >= 0)
            return bigPow(*args[0].cex().big, valueToI64(args[1]));
        if(useBig) return makeBigFloatValue(std::pow(valueToLongDouble(args[0]), valueToLongDouble(args[1])), bigPrecision);
        return makePlainNumber(std::pow(valueToApproxDouble(args[0]), valueToApproxDouble(args[1])));
    }
    if(name == "abs") {
        need(1);
        Value& a = args[0];
        switch(a.numKind) {
        case NumKind::Big: { BigInt b = *a.cex().big; b.negative = false; return makeBigValue(b); }
        case NumKind::BigF: return makeBigFloatValue(a.cex().bigDec ? a.cex().bigDec->abs() : BigDecimal());
        case NumKind::I32: return makeIntValue(a.i64 < 0 ? -a.i64 : a.i64, false);
        case NumKind::I64: return makeIntValue(a.i64 < 0 ? -a.i64 : a.i64, true);
        case NumKind::F32: return makeFloatValue(a.number < 0 ? -a.number : a.number);
        default: return makePlainNumber(std::fabs(a.number));
        }
    }
    if(name == "floor" || name == "ceil" || name == "round") {
        need(1);
        if(useBig) {
            BigDecimal x = valueToBigDecimal(args[0], bigPrecision);
            BigDecimal trunc = x.truncatedToInteger();
            if(name == "floor") {
                if(x.isNegative() && BigDecimal::compare(x, trunc) != 0) {
                    BigDecimal one = BigDecimal::fromString("1", bigPrecision);
                    trunc = trunc.sub(one);
                }
                return makeBigFloatValue(trunc);
            }
            if(name == "ceil") {
                if(!x.isNegative() && BigDecimal::compare(x, trunc) != 0) {
                    BigDecimal one = BigDecimal::fromString("1", bigPrecision);
                    trunc = trunc.add(one);
                }
                return makeBigFloatValue(trunc);
            }
            // round-half-up (matching std::round's away-from-zero behavior)
            BigDecimal half = BigDecimal::fromString(x.isNegative() ? "-0.5" : "0.5", bigPrecision);
            BigDecimal shifted = x.add(half);
            return makeBigFloatValue(shifted.truncatedToInteger());
        }
        long double x = valueToLongDouble(args[0]);
        long double r = (name=="floor") ? std::floor(x) : (name=="ceil") ? std::ceil(x) : std::round(x);
        return makePlainNumber((double)r);
    }
    if(name == "log" || name == "ln") { // natural log, matches C's log()
        need(1);
        long double x = valueToLongDouble(args[0]);
        if(x <= 0) throw std::runtime_error("log(): argument must be positive");
        long double r = std::log(x);
        return useBig ? makeBigFloatValue(r, bigPrecision) : makePlainNumber((double)r);
    }
    if(name == "log10") {
        need(1);
        long double x = valueToLongDouble(args[0]);
        if(x <= 0) throw std::runtime_error("log10(): argument must be positive");
        long double r = std::log10(x);
        return useBig ? makeBigFloatValue(r, bigPrecision) : makePlainNumber((double)r);
    }
    if(name == "exp") {
        need(1);
        long double r = std::exp(valueToLongDouble(args[0]));
        return useBig ? makeBigFloatValue(r, bigPrecision) : makePlainNumber((double)r);
    }
    if(name == "sin" || name == "cos" || name == "tan") {
        need(1);
        long double x = valueToLongDouble(args[0]);
        long double r = (name=="sin") ? std::sin(x) : (name=="cos") ? std::cos(x) : std::tan(x);
        return useBig ? makeBigFloatValue(r, bigPrecision) : makePlainNumber((double)r);
    }
    if(name == "min" || name == "max") {
        need(2);
        int c = numericCompare(args[0], args[1]);
        if(name == "min") return c <= 0 ? args[0] : args[1];
        return c >= 0 ? args[0] : args[1];
    }
    if(name == "random") {
        // random() -> uniform double in [0,1). random(n) -> uniform double
        // in [0,n). random(lo, hi) -> uniform double in [lo,hi).
        if(args.size() == 0) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            return makePlainNumber(dist(cnrRngEngine()));
        }
        if(args.size() == 1) {
            double hi = valueToApproxDouble(args[0]);
            std::uniform_real_distribution<double> dist(0.0, hi);
            return makePlainNumber(dist(cnrRngEngine()));
        }
        need(2);
        double lo = valueToApproxDouble(args[0]);
        double hi = valueToApproxDouble(args[1]);
        std::uniform_real_distribution<double> dist(lo, hi);
        return makePlainNumber(dist(cnrRngEngine()));
    }
    if(name == "randomSeed") {
        need(1);
        cnrRngEngine().seed((unsigned long)valueToApproxDouble(args[0]));
        return makePlainNumber(0);
    }
    throw std::runtime_error("Unknown built-in math function '" + name + "'");
}

// Statement-execution result: `execute`/`executeBlock` used to signal a
// `return` by throwing ReturnSignal (a C++ exception) and catching it at
// every function-call boundary. Per profiling, throw/catch is dramatically
// more expensive than a plain value return (stack unwinding + RTTI type
// matching on every single CnR function call/return). ExecResult replaces
// that: execute() returns a status telling the caller whether a `return`
// statement fired, and (if so) the returned Value is written into an
// out-parameter passed down the call chain. executeBlock() checks the
// status after each statement and stops early on Return, exactly mirroring
// what "the exception propagates past not-yet-executed statements" used to
// do -- just without ever unwinding the C++ stack.
enum class ExecResult { Normal, Return };

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
            (*obj.cex().object)[key] = val;
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
            arr.cex().objectArray->push_back(val);
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
        for(auto& kv : *v.cex().fields) {
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
        if(v.cex().isObjectArray) {
            oss << '[';
            bool first = true;
            for(auto& item : *v.cex().objectArray) {
                if(!first) oss << ',';
                first = false;
                serializeJsonInto(oss, item);
            }
            oss << ']';
        } else {
            oss << '{';
            bool first = true;
            for(auto& kv : *v.cex().object) {
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
    (*r.cex().object)["method"] = Value::makeString(req.method);
    (*r.cex().object)["path"] = Value::makeString(req.path);
    (*r.cex().object)["ip"] = Value::makeString(req.clientIp);
    (*r.cex().object)["contentType"] = Value::makeString(req.contentType);
    { Value cl; cl.number = (double)req.contentLength; (*r.cex().object)["contentLength"] = cl; }
    (*r.cex().object)["body"] = Value::makeString(req.body);

    Value paramsObj = Value::makeObject();
    paramsObj.ex().isLenientMap = true;
    for(auto& kv : params) (*paramsObj.cex().object)[kv.first] = Value::makeString(kv.second);
    (*r.cex().object)["params"] = paramsObj;

    Value queryObj = Value::makeObject();
    queryObj.ex().isLenientMap = true;
    for(auto& kv : req.query) (*queryObj.cex().object)[kv.first] = Value::makeString(kv.second);
    (*r.cex().object)["query"] = queryObj;

    Value headerObj = Value::makeObject();
    headerObj.ex().isLenientMap = true;
    for(auto& kv : req.headers) (*headerObj.cex().object)[kv.first] = Value::makeString(kv.second);
    (*r.cex().object)["header"] = headerObj;

    Value cookieObj = Value::makeObject();
    cookieObj.ex().isLenientMap = true;
    for(auto& kv : req.cookies) (*cookieObj.cex().object)[kv.first] = Value::makeString(kv.second);
    (*r.cex().object)["cookie"] = cookieObj;

    if(!req.body.empty()) {
        try {
            (*r.cex().object)["json"] = parseJsonString(req.body);
        } catch(...) {
            (*r.cex().object)["json"] = Value::makeNull();
        }
    } else {
        (*r.cex().object)["json"] = Value::makeNull();
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
            auto it = records[i].cex().fields->find(primaryKeyField);
            if(it != records[i].cex().fields->end())
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
        seedConstants();
    }

    Interpreter(std::unordered_map<std::string, FunctionDecl> fns,
                std::unordered_map<std::string, StructDecl> strs,
                std::unordered_map<std::string, DataDecl> das)
        : functions(std::move(fns)), structs(std::move(strs)) {
        scopes.push_back({});
        seedConstants();
        initDatabases(das);
    }

    Interpreter(const Interpreter& parent, bool /*forThread*/)
        : functions(parent.functions), structs(parent.structs), threadRegistry(parent.threadRegistry) {
        scopes.push_back({});
        seedConstants();
    }

    // Pre-populates the global scope with named math constants (pi, e/euler,
    // phi, psi) so they read like ordinary variables anywhere in a program.
    // A local `var pi = ...;` in the same scope will still shadow/overwrite
    // them like any other var -- these aren't hard-protected constants.
    void seedConstants() {
        scopes.back()["pi"] = makeBigFloatValue(cnrConstantAtPrecision(CNR_PI_DIGITS, CNR_BIGDEC_DEFAULT_SCALE));
        scopes.back()["e"] = makeBigFloatValue(cnrConstantAtPrecision(CNR_E_DIGITS, CNR_BIGDEC_DEFAULT_SCALE));
        scopes.back()["euler"] = makeBigFloatValue(cnrConstantAtPrecision(CNR_E_DIGITS, CNR_BIGDEC_DEFAULT_SCALE));
        scopes.back()["phi"] = makeBigFloatValue(cnrConstantAtPrecision(CNR_PHI_DIGITS, CNR_BIGDEC_DEFAULT_SCALE));
        scopes.back()["psi"] = makeBigFloatValue(cnrConstantAtPrecision(CNR_PSI_DIGITS, CNR_BIGDEC_DEFAULT_SCALE));
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
        auto it = currentSelf->cex().fields->find(name);
        if(it != currentSelf->cex().fields->end()) return it->second;
    }
    return lookupVar(name);
}

// Walks a NestedArrayLitExpr (built from var[]...[] = {{...},{...}}) and
// flattens it into row-major `array` data, inferring `dims` from the shape
// of the literal itself. Every row/sub-list at a given depth must have the
// same length as its siblings, mirroring how a real matrix/tensor requires
// a consistent shape (a "ragged" literal is a usage error, not silently
// accepted).
void collectTensorShape(const std::shared_ptr<NestedArrayLitExpr>& node, int depth, std::vector<int>& dims) {
    size_t n = node->isLeaf ? node->leafValues.size() : node->children.size();
    if((int)dims.size() <= depth) dims.push_back((int)n);
    else if(dims[depth] != (int)n)
        throw std::runtime_error("Inconsistent Matrix/Tensor shape: expected " + std::to_string(dims[depth]) + " element(s) at depth " + std::to_string(depth) + ", got " + std::to_string(n));
    if(!node->isLeaf) {
        for(auto& child : node->children) collectTensorShape(child, depth + 1, dims);
    }
}
void flattenTensorLiteral(const std::shared_ptr<NestedArrayLitExpr>& node, std::vector<double>& out) {
    if(node->isLeaf) {
        for(auto& e : node->leafValues) out.push_back(evalNumber(e));
    } else {
        for(auto& child : node->children) flattenTensorLiteral(child, out);
    }
}
Value buildTensorFromLiteral(const std::shared_ptr<NestedArrayLitExpr>& lit) {
    Value v;
    v.isArray = true;
    collectTensorShape(lit, 0, v.dims);
    flattenTensorLiteral(lit, v.array);
    return v;
}

// Evaluates a BinaryExpr at the Value level so typed operands (Int/long/
// BigInt/Float/BigFloat) keep their type through comparisons and arithmetic
// instead of being flattened to a plain double. evalNumber/evalBool/
// evalToValue all funnel BinaryExpr through here.
Value evalBinaryValue(const std::shared_ptr<BinaryExpr>& b) {
    if(b->op==TokType::AndAnd) { Value v; v.isBool=true; v.boolean = evalBool(b->left) && evalBool(b->right); return v; }
    if(b->op==TokType::OrOr) { Value v; v.isBool=true; v.boolean = evalBool(b->left) || evalBool(b->right); return v; }
    if(b->op==TokType::EqualEqual || b->op==TokType::BangEqual) {
        Value L = evalToValue(b->left);
        Value R = evalToValue(b->right);
        bool eq = valuesEqual(L, R);
        Value v; v.isBool=true; v.boolean = (b->op==TokType::EqualEqual ? eq : !eq); return v;
    }
    if(b->op==TokType::Less || b->op==TokType::Greater || b->op==TokType::LessEqual || b->op==TokType::GreaterEqual) {
        Value L = evalToValue(b->left);
        Value R = evalToValue(b->right);
        int c = numericCompare(L, R);
        bool res = false;
        switch(b->op) {
        case TokType::Less: res = c<0; break;
        case TokType::Greater: res = c>0; break;
        case TokType::LessEqual: res = c<=0; break;
        case TokType::GreaterEqual: res = c>=0; break;
        default: break;
        }
        Value v; v.isBool=true; v.boolean = res; return v;
    }
    Value L = evalToValue(b->left);
    Value R = evalToValue(b->right);
    return numericBinaryOp(b->op, L, R);
}

double evalNumber(const ExprPtr& expr)
{
    // Dispatch on the AST tag instead of trying std::dynamic_pointer_cast<T>
    // against every possible type in sequence. Each case below is exactly
    // the original branch's body, with `dynamic_pointer_cast<T>(expr)`
    // replaced by `static_cast<T*>(expr.get())`, which is safe here because
    // `kind` is set once at construction and always matches the real type.
    switch(expr->kind) {
    case ExprKind::Number:
        return static_cast<NumberExpr*>(expr.get())->value;

    case ExprKind::Bool:
        return static_cast<BoolExpr*>(expr.get())->value ? 1.0 : 0.0;

    case ExprKind::Var: {
        auto v = static_cast<VarExpr*>(expr.get());
        Value& val = resolveVar(v->name);
        if(val.isArray)
            throw std::runtime_error("Cannot use array '" + v->name + "' as a number");
        if(val.isStruct)
            throw std::runtime_error("Cannot use struct '" + v->name + "' as a number");
        if(val.isBool) return val.boolean ? 1.0 : 0.0;
        return val.number;
    }

    case ExprKind::Unary: {
        auto u = static_cast<UnaryExpr*>(expr.get());
        if(u->op==TokType::Bang) return evalBool(u->expr) ? 0.0 : 1.0;
        double x = evalNumber(u->expr);
        if(u->op==TokType::Minus) return -x;
        return x;
    }

    case ExprKind::CharCast:
        return evalNumber(static_cast<CharCastExpr*>(expr.get())->expr);

    case ExprKind::IntCast:
    case ExprKind::LongCast:
    case ExprKind::FloatCast:
    case ExprKind::BigIntCast:
    case ExprKind::BigFloatCast:
        return valueToApproxDouble(evalToValue(expr));

    case ExprKind::Binary: {
        auto b = std::static_pointer_cast<BinaryExpr>(expr);
        Value v = evalBinaryValue(b);
        return v.isBool ? (v.boolean ? 1.0 : 0.0) : valueToApproxDouble(v);
    }

    case ExprKind::ArrayAccess: {
        auto a = static_cast<ArrayAccessExpr*>(expr.get());
        if(!a->index)
            throw std::runtime_error("Array '" + a->arrayName + "' used without an index in this context");
        Value& val = resolveVar(a->arrayName);
        if(!val.isArray)
            throw std::runtime_error("'" + a->arrayName + "' is not an array");
        int index = (int)evalNumber(a->index);
        if(val.dims.size() > 1)
            throw std::runtime_error("'" + a->arrayName + "' is a Matrix/Tensor and needs " + std::to_string(val.dims.size()) + " indices to use as a number, e.g. " + a->arrayName + "[i][j]...");
        if(index < 0 || index >= (int)val.array.size())
            throw std::runtime_error("Array index out of bounds for '" + a->arrayName + "': " + std::to_string(index));
        return val.array[index];
    }

    case ExprKind::TensorAccess: {
        auto ta = static_cast<TensorAccessExpr*>(expr.get());
        Value& val = resolveVar(ta->arrayName);
        if(!val.isArray || val.dims.empty())
            throw std::runtime_error("'" + ta->arrayName + "' is not a Matrix/Tensor");
        std::vector<int> idx;
        for(auto& e : ta->indices) idx.push_back((int)evalNumber(e));
        if(idx.size() != val.dims.size())
            throw std::runtime_error("'" + ta->arrayName + "' needs " + std::to_string(val.dims.size()) + " index/indices to use as a number, got " + std::to_string(idx.size()));
        for(size_t i=0;i<idx.size();++i)
            if(idx[i]<0 || idx[i]>=val.dims[i])
                throw std::runtime_error("Tensor index out of bounds for '" + ta->arrayName + "' at dimension " + std::to_string(i) + ": " + std::to_string(idx[i]));
        return val.array[tensorFlatOffset(val.dims, idx)];
    }

    case ExprKind::Len: {
        auto l = static_cast<LenExpr*>(expr.get());
        Value& val = resolveVar(l->arrayName);
        if(!val.isArray)
            throw std::runtime_error("'" + l->arrayName + "' is not an array");
        if(!val.dims.empty()) return (double)val.dims[0];
        return (double)val.array.size();
    }

    case ExprKind::MemberAccess: {
        auto m = std::static_pointer_cast<MemberAccessExpr>(expr);
        Value v = evalMemberAccess(m);
        if(v.isArray) throw std::runtime_error("Cannot use array field '" + m->member + "' as a number");
        if(v.isStruct) throw std::runtime_error("Cannot use struct field '" + m->member + "' as a number");
        if(v.isObject) throw std::runtime_error("Cannot use object field '" + m->member + "' as a number");
        if(v.isString) throw std::runtime_error("Cannot use string field '" + m->member + "' as a number (use (string) cast or print it directly)");
        if(v.isNull) throw std::runtime_error("Field '" + m->member + "' is null");
        return v.isBool ? (v.boolean ? 1.0 : 0.0) : v.number;
    }

    case ExprKind::Call: {
        auto c = static_cast<CallExpr*>(expr.get());
        Value v = callCallable(c->name, c->args);
        if(v.isArray) throw std::runtime_error("Cannot use array result of '" + c->name + "' as a number");
        if(v.isStruct) throw std::runtime_error("Cannot use struct result of '" + c->name + "' as a number");
        return v.isBool ? (v.boolean ? 1.0 : 0.0) : v.number;
    }

    case ExprKind::Thread: {
        auto t = std::static_pointer_cast<ThreadExpr>(expr);
        Value v = spawnThread(t);
        return v.number;
    }

    case ExprKind::Join: {
        auto j = std::static_pointer_cast<JoinExpr>(expr);
        Value v = joinThread(j);
        if(v.isArray) throw std::runtime_error("Cannot use array result of join() as a number");
        if(v.isStruct) throw std::runtime_error("Cannot use struct result of join() as a number");
        return v.isBool ? (v.boolean ? 1.0 : 0.0) : v.number;
    }

    case ExprKind::ArrayMethodCall: {
        auto am = std::static_pointer_cast<ArrayMethodCallExpr>(expr);
        return callArrayMethod(am);
    }

    default:
        break;
    }

    throw std::runtime_error("Cannot evaluate numeric expression.");
}
bool evalBool(const ExprPtr& expr)
{
    // Same tag-dispatch approach as evalNumber. The original fell through to
    // `evalNumber(expr)!=0` for every kind not explicitly handled above
    // (including a Unary whose op isn't Bang) -- that fallthrough is
    // preserved exactly via the switch's `default`/`break` path below.
    switch(expr->kind) {
    case ExprKind::Bool:
        return static_cast<BoolExpr*>(expr.get())->value;

    case ExprKind::Unary: {
        auto u = static_cast<UnaryExpr*>(expr.get());
        if(u->op==TokType::Bang) return !evalBool(u->expr);
        break;
    }

    case ExprKind::Var: {
        auto v = static_cast<VarExpr*>(expr.get());
        Value& val = resolveVar(v->name);
        if(val.isBool) return val.boolean;
        return val.number != 0;
    }

    case ExprKind::Binary: {
        auto op = std::static_pointer_cast<BinaryExpr>(expr);
        Value v = evalBinaryValue(op);
        return v.isBool ? v.boolean : (valueToApproxDouble(v) != 0);
    }

    default:
        break;
    }

    return evalNumber(expr)!=0;
}

Value evalToValue(const ExprPtr& expr) {
    // Tag dispatch, same technique as evalNumber/evalBool. Kinds that had no
    // explicit branch in the original if-chain (Number, Len, CharCast) fall
    // through to the same final `evalNumber(expr)` computation, preserved
    // below as the switch's default case.
    switch(expr->kind) {
    case ExprKind::Call: {
        auto c = static_cast<CallExpr*>(expr.get());
        return callCallable(c->name, c->args);
    }
    case ExprKind::MemberAccess:
        return evalMemberAccess(std::static_pointer_cast<MemberAccessExpr>(expr));
    case ExprKind::Bool: {
        Value v; v.isBool=true; v.boolean=static_cast<BoolExpr*>(expr.get())->value; return v;
    }
    case ExprKind::Thread:
        return spawnThread(std::static_pointer_cast<ThreadExpr>(expr));
    case ExprKind::Join:
        return joinThread(std::static_pointer_cast<JoinExpr>(expr));
    case ExprKind::JoinAll:
        return joinAllThreads(std::static_pointer_cast<JoinAllExpr>(expr));
    case ExprKind::ArrayMethodCall: {
        Value v; v.number = callArrayMethod(std::static_pointer_cast<ArrayMethodCallExpr>(expr)); return v;
    }
    case ExprKind::StringLit:
        return Value::makeString(static_cast<StringLitExpr*>(expr.get())->value);
    case ExprKind::StringCast: {
        auto sc = static_cast<StringCastExpr*>(expr.get());
        return Value::makeString(valueToDisplayString(evalToValue(sc->expr)));
    }
    case ExprKind::JsonObjectLit: {
        auto jo = static_cast<JsonObjectLitExpr*>(expr.get());
        Value obj = Value::makeObject();
        for(auto& kv : jo->entries) {
            std::string key = valueToDisplayString(evalToValue(kv.first));
            (*obj.cex().object)[key] = evalToValue(kv.second);
        }
        return obj;
    }
    case ExprKind::HttpCall:
        return performHttpCall(std::static_pointer_cast<HttpCallExpr>(expr));
    case ExprKind::ServerConfig: {
        auto sc = static_cast<ServerConfigExpr*>(expr.get());
        Value v;
        v.ex().isServer = true;
        v.ex().server = std::make_shared<ServerInstance>();
        for(auto& kv : sc->entries) v.cex().server->config[kv.first] = evalToValue(kv.second);
        return v;
    }
    case ExprKind::ObjectMethodCall:
        return callObjectMethod(std::static_pointer_cast<ObjectMethodCallExpr>(expr));
    case ExprKind::DbMethodCall:
        return callDbMethod(std::static_pointer_cast<DbMethodCallExpr>(expr));
    case ExprKind::Fail:
        throw NodeFailSignal{};
    case ExprKind::Throw: {
        auto th = static_cast<ThrowExpr*>(expr.get());
        std::string msg = valueToDisplayString(evalToValue(th->messageExpr));
        throw CnrThrowSignal{msg};
    }
    case ExprKind::Var: {
        auto v = static_cast<VarExpr*>(expr.get());
        Value& val = resolveVar(v->name);
        return val;
    }
    case ExprKind::Binary:
        return evalBinaryValue(std::static_pointer_cast<BinaryExpr>(expr));
    case ExprKind::Unary: {
        auto u = static_cast<UnaryExpr*>(expr.get());
        if(u->op==TokType::Bang) { Value v; v.isBool=true; v.boolean = !evalBool(u->expr); return v; }
        Value x = evalToValue(u->expr);
        if(u->op==TokType::Minus) {
            switch(x.numKind) {
            case NumKind::Big: return makeBigValue(-(*x.cex().big));
            case NumKind::BigF: return makeBigFloatValue(x.cex().bigDec ? x.cex().bigDec->negate() : BigDecimal());
            case NumKind::I32: return makeIntValue(-x.i64, false);
            case NumKind::I64: return makeIntValue(-x.i64, true);
            case NumKind::F32: return makeFloatValue(-x.number);
            default: { Value v; v.number = -(x.isBool ? (x.boolean?1.0:0.0) : x.number); return v; }
            }
        }
        return x;
    }
    case ExprKind::IntCast: {
        auto ic = static_cast<IntCastExpr*>(expr.get());
        return makeIntValue(valueToI64(evalToValue(ic->expr)), false);
    }
    case ExprKind::LongCast: {
        auto lc = static_cast<LongCastExpr*>(expr.get());
        return makeIntValue(valueToI64(evalToValue(lc->expr)), true);
    }
    case ExprKind::FloatCast: {
        auto fc = static_cast<FloatCastExpr*>(expr.get());
        return makeFloatValue(valueToLongDouble(evalToValue(fc->expr)));
    }
    case ExprKind::BigIntCast: {
        auto bic = static_cast<BigIntCastExpr*>(expr.get());
        if(bic->expr->kind == ExprKind::Number) {
            auto num = static_cast<NumberExpr*>(bic->expr.get());
            if(!num->text.empty() && num->text.find('.') == std::string::npos)
                return makeBigValue(BigInt::fromString(num->text));
        }
        return makeBigValue(valueToBigInt(evalToValue(bic->expr)));
    }
    case ExprKind::BigFloatCast: {
        auto bfc = static_cast<BigFloatCastExpr*>(expr.get());
        int precision = CNR_BIGDEC_DEFAULT_SCALE;
        if(bfc->precisionExpr) precision = BigDecimal::clampPrecision((int)evalNumber(bfc->precisionExpr));
        // A literal numeric expression is parsed straight from its exact
        // source text ("3.14159...") rather than round-tripping through a
        // double/long double first, so BigFloat(N) on a literal keeps every
        // digit the user actually wrote, up to N fractional digits.
        if(bfc->expr->kind == ExprKind::Number) {
            auto num = static_cast<NumberExpr*>(bfc->expr.get());
            if(!num->text.empty()) return makeBigFloatValue(BigDecimal::fromString(num->text, precision));
        }
        Value inner = evalToValue(bfc->expr);
        if(inner.numKind == NumKind::BigF && inner.cex().bigDec) {
            BigDecimal r = *inner.cex().bigDec;
            r.precision = precision;
            r.roundToPrecision(precision);
            return makeBigFloatValue(r);
        }
        if(inner.numKind == NumKind::Big && inner.cex().big) {
            BigDecimal r;
            r.precision = precision;
            r.scale = 0;
            r.mantissa = *inner.cex().big;
            return makeBigFloatValue(r);
        }
        return makeBigFloatValue(valueToLongDouble(inner), precision);
    }
    case ExprKind::ArrayAccess: {
        auto a = static_cast<ArrayAccessExpr*>(expr.get());
        if(!a->index)
            throw std::runtime_error("Array '" + a->arrayName + "' used without an index in this context");
        Value& val = resolveVar(a->arrayName);
        if(!val.isArray)
            throw std::runtime_error("'" + a->arrayName + "' is not an array");
        int index = (int)evalNumber(a->index);
        if(!val.dims.empty()) {
            if(index < 0 || index >= val.dims[0])
                throw std::runtime_error("Tensor index out of bounds for '" + a->arrayName + "': " + std::to_string(index));
            if(val.dims.size() == 1) {
                Value v; v.number = val.array[index]; return v;
            }
            return tensorSlice(val, {index});
        }
        if(index < 0 || index >= (int)val.array.size())
            throw std::runtime_error("Array index out of bounds for '" + a->arrayName + "': " + std::to_string(index));
        Value v; v.number = val.array[index]; return v;
    }
    case ExprKind::TensorAccess: {
        auto ta = static_cast<TensorAccessExpr*>(expr.get());
        Value& val = resolveVar(ta->arrayName);
        if(!val.isArray || val.dims.empty())
            throw std::runtime_error("'" + ta->arrayName + "' is not a Matrix/Tensor");
        std::vector<int> idx;
        for(auto& e : ta->indices) idx.push_back((int)evalNumber(e));
        if(idx.size() > val.dims.size())
            throw std::runtime_error("Too many indices for '" + ta->arrayName + "' (has " + std::to_string(val.dims.size()) + " dimension(s))");
        for(size_t i=0;i<idx.size();++i)
            if(idx[i]<0 || idx[i]>=val.dims[i])
                throw std::runtime_error("Tensor index out of bounds for '" + ta->arrayName + "' at dimension " + std::to_string(i) + ": " + std::to_string(idx[i]));
        if(idx.size() == val.dims.size()) {
            Value v; v.number = val.array[tensorFlatOffset(val.dims, idx)]; return v;
        }
        return tensorSlice(val, idx);
    }
    default:
        break;
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
    (*result.cex().object)["status"] = statusVal;
    (*result.cex().object)["statusText"] = Value::makeString(resp.statusText);
    (*result.cex().object)["body"] = Value::makeString(resp.body);

    Value headersObj = Value::makeObject();
    for(auto& kv : resp.headers) (*headersObj.cex().object)[kv.first] = Value::makeString(kv.second);
    (*result.cex().object)["headers"] = headersObj;

    // Best-effort JSON parse of the body for `.json` access; on failure,
    // `.json` becomes null rather than throwing, since not every response is JSON.
    try {
        if(!resp.body.empty()) {
            Value parsed = parseJsonString(resp.body);
            (*result.cex().object)["json"] = parsed;
        } else {
            (*result.cex().object)["json"] = Value::makeNull();
        }
    } catch(...) {
        (*result.cex().object)["json"] = Value::makeNull();
    }

    return result;
}

Value evalMemberAccess(const std::shared_ptr<MemberAccessExpr>& m) {
    Value obj = evalToValue(m->base);

    if(obj.isStruct) {
        auto it = obj.cex().fields->find(m->member);
        if(it == obj.cex().fields->end())
            throw std::runtime_error("Struct '" + obj.cex().structType + "' has no field '" + m->member + "'");
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
        if(obj.cex().isObjectArray)
            throw std::runtime_error("Cannot access member '" + m->member + "' on a JSON array (index it with [i] instead)");
        auto it = obj.cex().object->find(m->member);
        if(it == obj.cex().object->end()) {
            if(obj.cex().isLenientMap) return Value::makeString(""); // e.g. request.query.missingKey -> ""
            throw std::runtime_error("Object has no field '" + m->member + "'");
        }
        Value field = it->second;
        if(m->index) {
            if(field.isObject && field.cex().isObjectArray) {
                int idx = (int)evalNumber(m->index);
                if(idx < 0 || idx >= (int)field.cex().objectArray->size())
                    throw std::runtime_error("Array index out of bounds for field '" + m->member + "': " + std::to_string(idx));
                return (*field.cex().objectArray)[idx];
            }
            if(field.isObject) {
                // String-keyed lookup, e.g. request.header["Authorization"] or
                // request.cookie["session"] -- field is itself a map, and the
                // index expression is a string key rather than a numeric position.
                Value keyVal = evalToValue(m->index);
                std::string key = valueToDisplayString(keyVal);
                auto fit = field.cex().object->find(key);
                if(fit == field.cex().object->end()) return Value::makeString(""); // absent header/cookie -> empty string
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
    handle.ex().isThread = true;
    handle.number = (double)handleId;
    return handle;
}

size_t resolveThreadHandle(const ExprPtr& handleExpr) {
    double idNum;
    if(std::dynamic_pointer_cast<ArrayAccessExpr>(handleExpr)) {
        idNum = evalNumber(handleExpr);
    } else {
        Value v = evalToValue(handleExpr);
        if(!v.cex().isThread)
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

    if(isMathBuiltinName(name)) return callMathBuiltin(name, argVals);

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
        executeBlock(fn.body, result);
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
    instance.ex().structType = sd.name;
    instance.ex().fields = std::make_shared<std::unordered_map<std::string, Value>>();
    for(auto& f : sd.fields) {
        Value fv;
        fv.isArray = f.isArray;
        (*instance.cex().fields)[f.name] = fv;
    }

    scopes.push_back({});
    for(size_t i=0;i<sd.ctorParams.size();++i) scopes.back()[sd.ctorParams[i]] = args[i];
    Value* prevSelf = currentSelf;
    currentSelf = &instance;
    try {
        Value discardedReturn;
        executeBlock(sd.ctorBody, discardedReturn);
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
        v.ex().isDatabase = true;
        v.ex().database = db;
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
            const auto& fields = *rec.cex().fields;
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
            rec.ex().structType = structType;
            rec.ex().fields = std::make_shared<std::unordered_map<std::string, Value>>();
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
                (*rec.cex().fields)[fname] = fv;
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
    if(!dbVal.cex().isDatabase)
        throw std::runtime_error("'" + dm->dataName + "' is not a Data instance");
    DatabaseInstance& db = *dbVal.cex().database;
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
        auto it = rec.cex().fields->find(table.primaryKeyField);
        if(it == rec.cex().fields->end())
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
            for(auto& fkv : *table.records[i].cex().fields) {
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
            auto it = table.records[i].cex().fields->find(fieldName);
            if(it == table.records[i].cex().fields->end()) continue;
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
            auto it = table.records[i].cex().fields->find(fieldName);
            if(it == table.records[i].cex().fields->end()) continue;
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
        auto& fields = *table.records[idx].cex().fields;
        auto fit = fields.find(fieldName);
        if(fit == fields.end())
            throw std::runtime_error("table.insert(): struct '" + table.structType + "' has no field '" + fieldName + "'");
        if(!table.primaryKeyField.empty() && fieldName == table.primaryKeyField)
            throw std::runtime_error("table.insert(): cannot modify primary key field '" + fieldName + "' via insert(); use updateById() on a non-key field instead");
        fit->second = std::move(newVal);
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
            auto it = rec.cex().fields->find(fieldName);
            if(it == rec.cex().fields->end()) continue;
            if(valueToDisplayString(it->second) != matchValue) continue;
            auto tit = rec.cex().fields->find(targetField);
            if(tit == rec.cex().fields->end())
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
        auto& fields = *table.records[it->second].cex().fields;
        auto fit = fields.find(fieldName);
        if(fit == fields.end())
            throw std::runtime_error("table.updateById(): struct '" + table.structType + "' has no field '" + fieldName + "'");
        fit->second = std::move(newVal);
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
            auto ia = table.records[a].cex().fields->find(fieldName);
            auto ib = table.records[b].cex().fields->find(fieldName);
            if(ia == table.records[a].cex().fields->end() || ib == table.records[b].cex().fields->end())
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

// Runs every statement in `block` in order, stopping immediately if any of
// them executes a `return` (ExecResult::Return), and writes the returned
// value into `returnValue`. This early-stop is what makes `return` inside
// a nested if/while/for correctly skip the rest of the enclosing block,
// exactly like the old throw/catch did, but via ordinary control flow.
ExecResult executeBlock(const std::shared_ptr<BlockStmt>& block, Value& returnValue) {
    for(auto& stmt : block->statements) {
        if(execute(stmt, returnValue) == ExecResult::Return) return ExecResult::Return;
    }
    return ExecResult::Normal;
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
    (*scopes.back()["response"].cex().object)["status"] = [](){ Value v; v.number = 200; return v; }();

    Value returned;
    bool hadReturn = false;
    try {
        hadReturn = (executeBlock(route.body, returned) == ExecResult::Return);
    } catch(const std::exception& e) {
        scopes.pop_back();
        currentResponseState = prevRs;
        ServerResponseState errRs;
        errRs.status = 500;
        Value errBody = Value::makeObject();
        (*errBody.cex().object)["error"] = Value::makeString(e.what());
        return buildRawHttpResponse(errRs, errBody, true, serverName);
    }

    // response.status = N; is stored as a plain field on the `response`
    // object; pull it out into the response state before serializing.
    Value& responseVal = scopes.back()["response"];
    if(responseVal.isObject) {
        auto it = responseVal.cex().object->find("status");
        if(it != responseVal.cex().object->end() && !it->second.isNull) {
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
        (*body.cex().object)["error"] = Value::makeString(pathExistsForOtherMethod ? "Method Not Allowed" : "Not Found");
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
            // A bare `return;` inside a node body ends that attempt
            // successfully without propagating out of the workflow (Nodes
            // bodies aren't functions) -- so both a Normal finish and a
            // Return status count as attempt-succeeded here; only the
            // pendingNodeFailFromCatch check below can turn this into a
            // retry.
            Value discardedReturn;
            executeBlock(node.body, discardedReturn);
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

ExecResult execute(const StmtPtr& stmt, Value& returnValue)
{
    // Tag dispatch: switch on the statement's kind instead of trying every
    // std::dynamic_pointer_cast<T> in sequence (this chain, run for every
    // statement of every loop iteration, was one of the two dominant costs
    // identified by profiling). Each case body is unchanged from the
    // original; only the cast mechanism changed.
    switch(stmt->kind) {
    case StmtKind::TryCatch: {
        auto s = static_cast<TryCatchStmt*>(stmt.get());
        // A `return;` inside a try block still returns from the enclosing
        // function normally -- so if executeBlock(tryBlock) reports Return,
        // we propagate that status upward immediately without touching the
        // catch block, matching the old "ReturnSignal not caught here"
        // behavior.
        ExecResult r = ExecResult::Normal;
        try {
            r = executeBlock(s->tryBlock, returnValue);
        } catch(NodeFailSignal&) {
            // Fail() caught by a try/catch inside a Nodes node body: run the
            // catch block as normal, but remember that Fail() happened so the
            // enclosing node attempt still gets marked failed for retries.
            pendingNodeFailFromCatch = true;
            scopes.push_back({});
            scopes.back()[s->catchVarName] = Value::makeString("Fail() called");
            r = executeBlock(s->catchBlock, returnValue);
            scopes.pop_back();
            return r;
        } catch(CnrThrowSignal& t) {
            scopes.push_back({});
            scopes.back()[s->catchVarName] = Value::makeString(t.message);
            r = executeBlock(s->catchBlock, returnValue);
            scopes.pop_back();
            return r;
        } catch(const std::exception& e) {
            scopes.push_back({});
            scopes.back()[s->catchVarName] = Value::makeString(e.what());
            r = executeBlock(s->catchBlock, returnValue);
            scopes.pop_back();
            return r;
        }
        return r;
    }
    case StmtKind::Nodes: {
        auto s = std::static_pointer_cast<NodesStmt>(stmt);
        runNodesWorkflow(s);
        return ExecResult::Normal;
    }
    case StmtKind::Parallel: {
        auto s = static_cast<ParallelStmt*>(stmt.get());
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
                    Value discardedReturn;
                    worker.executeBlock(block, discardedReturn);
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
        return ExecResult::Normal;
    }
    case StmtKind::VarDecl: {
        auto s = static_cast<VarDeclStmt*>(stmt.get());
        Value value;
        value.isArray = s->isArray;
        if(s->tensorRank >= 2) {
            value = buildTensorFromLiteral(s->nestedInit);
        } else if(s->isArray) {
            for(auto& e : s->arrayValues) value.array.push_back(evalNumber(e));
        } else {
            value = evalToValue(s->value);
        }
        scopes.back()[s->name]=value;
        return ExecResult::Normal;
    }
    case StmtKind::Assign: {
        auto s = static_cast<AssignStmt*>(stmt.get());
        Value& val = resolveVar(s->name);
        if(val.isArray)
            throw std::runtime_error("Cannot assign a number to array '" + s->name + "'");
        Value newVal = evalToValue(s->value);
        val = std::move(newVal);
        return ExecResult::Normal;
    }
    case StmtKind::ArrayAssign: {
        auto s = static_cast<ArrayAssignStmt*>(stmt.get());
        Value& val = resolveVar(s->arrayName);
        if(!val.isArray)
            throw std::runtime_error("'" + s->arrayName + "' is not an array");
        if(!s->indices.empty()) {
            if(val.dims.empty())
                throw std::runtime_error("'" + s->arrayName + "' is not a Matrix/Tensor");
            std::vector<int> idx;
            for(auto& e : s->indices) idx.push_back((int)evalNumber(e));
            if(idx.size() != val.dims.size())
                throw std::runtime_error("'" + s->arrayName + "' needs " + std::to_string(val.dims.size()) + " index/indices to assign a scalar, got " + std::to_string(idx.size()));
            tensorSetScalar(val, idx, evalNumber(s->value));
            return ExecResult::Normal;
        }
        int index = (int)evalNumber(s->index);
        if(!val.dims.empty())
            throw std::runtime_error("'" + s->arrayName + "' is a Matrix/Tensor and needs " + std::to_string(val.dims.size()) + " index/indices, e.g. " + s->arrayName + "[" + std::to_string(index) + "]" + std::string(val.dims.size()>1 ? "[...]" : "") + " = value;");
        if(index < 0 || index >= (int)val.array.size())
            throw std::runtime_error("Array index out of bounds for '" + s->arrayName + "': " + std::to_string(index));
        val.array[index] = evalNumber(s->value);
        return ExecResult::Normal;
    }
    case StmtKind::MemberAssign: {
        auto s = static_cast<MemberAssignStmt*>(stmt.get());
        Value& obj = resolveVar(s->objectName);
        if(obj.isObject && !obj.cex().isObjectArray) {
            // Dynamic object field assignment (e.g. response.status = 200;
            // response.headersToSend["X"] = "Y";). Creates the field if absent,
            // matching object-literal semantics elsewhere in the language.
            Value& field = (*obj.cex().object)[s->member];
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
            return ExecResult::Normal;
        }
        if(!obj.isStruct)
            throw std::runtime_error("'" + s->objectName + "' is not a struct instance or object");
        auto it = obj.cex().fields->find(s->member);
        if(it == obj.cex().fields->end())
            throw std::runtime_error("Struct '" + obj.cex().structType + "' has no field '" + s->member + "'");
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
            field = std::move(newVal);
        }
        return ExecResult::Normal;
    }
    case StmtKind::Print: {
        auto s = static_cast<PrintStmt*>(stmt.get());
        if(s->printCharArray) {
            auto& arr = resolveVar(s->arrayName).array;
            for(double c : arr) std::cout << (char)c;
            std::cout << '\n';
            return ExecResult::Normal;
        }
        if(s->printChar) { std::cout << (char)evalNumber(s->expr); return ExecResult::Normal; }
        Value v = evalToValue(s->expr);
        std::cout << valueToDisplayString(v) << '\n';
        return ExecResult::Normal;
    }
    case StmtKind::If: {
        auto s = static_cast<IfStmt*>(stmt.get());
        if(evalBool(s->condition)) return executeBlock(s->thenBlock, returnValue);
        else if(s->elseBlock) return executeBlock(s->elseBlock, returnValue);
        return ExecResult::Normal;
    }
    case StmtKind::While: {
        auto s = static_cast<WhileStmt*>(stmt.get());
        while(evalBool(s->condition)) {
            if(executeBlock(s->body, returnValue) == ExecResult::Return) return ExecResult::Return;
        }
        return ExecResult::Normal;
    }
    case StmtKind::For: {
        auto s = static_cast<ForStmt*>(stmt.get());
        execute(s->init, returnValue);
        while(evalBool(s->condition)) {
            if(executeBlock(s->body, returnValue) == ExecResult::Return) return ExecResult::Return;
            execute(s->increment, returnValue);
        }
        return ExecResult::Normal;
    }
    case StmtKind::Return: {
        auto s = static_cast<ReturnStmt*>(stmt.get());
        if(s->value) returnValue = evalToValue(s->value);
        else returnValue = Value();
        return ExecResult::Return;
    }
    case StmtKind::ExprS: {
        auto s = static_cast<ExprStmt*>(stmt.get());
        evalToValue(s->expr);
        return ExecResult::Normal;
    }
    case StmtKind::RouteDecl: {
        auto s = static_cast<RouteDeclStmt*>(stmt.get());
        Value& serverVal = resolveVar(s->serverName);
        if(!serverVal.cex().isServer)
            throw std::runtime_error("'" + s->serverName + "' is not a Server instance");
        RegisteredRoute route;
        route.method = s->method;
        route.pathPattern = valueToDisplayString(evalToValue(s->pathExpr));
        route.body = s->body;
        serverVal.cex().server->routes.push_back(route);
        return ExecResult::Normal;
    }
    case StmtKind::ServerStart: {
        auto s = static_cast<ServerStartStmt*>(stmt.get());
        Value& serverVal = resolveVar(s->serverName);
        if(!serverVal.cex().isServer)
            throw std::runtime_error("'" + s->serverName + "' is not a Server instance");
        runServerLoop(serverVal.cex().server);
        return ExecResult::Normal;
    }
    default:
        break;
    }
    throw std::runtime_error("Unknown statement.");
}
};

void runProgram(const Program& program)
{
    Interpreter interpreter(program.functions, program.structs, program.datas);
    Value discardedReturn;
    for(auto& stmt : program.statements) interpreter.execute(stmt, discardedReturn);
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