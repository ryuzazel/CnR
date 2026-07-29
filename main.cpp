#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cctype>
#include <stdexcept>

enum class TokType {
    Number, Ident,
    Var, Print, If, Else, While, For, TrueKw, FalseKw, CharKw, LenKw,
    Function, Return, StructKw,
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
    std::vector<Token> tokenize() {
        std::vector<Token> out;
        while (true) {
            skipWhitespace();
            if (peek()=='\0') { out.push_back({TokType::End,"",0,line}); break; }
            char c=peek();
            int l=line;
            if (std::isdigit((unsigned char)c)) { out.push_back(number()); continue; }
            if (std::isalpha((unsigned char)c) || c=='_') { out.push_back(identifier()); continue; }
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

struct VarDeclStmt : Stmt { bool isArray = false; std::string name; ExprPtr value; std::vector<ExprPtr> arrayValues; };
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
        return parsePrimary();
    }
    ExprPtr parsePrimary() {
        if(check(TokType::Number)) { double v = advance().number; return std::make_shared<NumberExpr>(v); }
        if(match(TokType::TrueKw)) return std::make_shared<BoolExpr>(true);
        if(match(TokType::FalseKw)) return std::make_shared<BoolExpr>(false);
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
            if(match(TokType::Dot)) {
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
        stmt->name= expect(TokType::Ident,"identifier").text;
        if(match(TokType::LBracket)) {
            expect(TokType::RBracket,"]");
            stmt->isArray=true;
            expect(TokType::Assign,"=");
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
        if(match(TokType::Dot)) {
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
            auto stmt= std::make_shared<ArrayAssignStmt>();
            stmt->arrayName=id.text;
            stmt->index=parseExpression();
            expect(TokType::RBracket,"]");
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

    StmtPtr parseStatement() {
        if(check(TokType::Var)) return parseVarDecl();
        if(check(TokType::Print)) return parsePrint();
        if(check(TokType::If)) return parseIf();
        if(check(TokType::While)) return parseWhile();
        if(check(TokType::For)) return parseFor();
        if(check(TokType::Return)) return parseReturn();
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
    double number = 0;
    bool boolean = false;
    std::vector<double> array;
    std::string structType;
    std::shared_ptr<std::unordered_map<std::string, Value>> fields;
};

// Thrown by 'return' statements; carries the returned value up to the call site.
struct ReturnSignal { Value value; };

class Interpreter {
public:
    Interpreter(std::unordered_map<std::string, FunctionDecl> fns,
                std::unordered_map<std::string, StructDecl> strs)
        : functions(std::move(fns)), structs(std::move(strs)) {
        scopes.push_back({});
    }

    std::vector<std::unordered_map<std::string, Value>> scopes;
    std::unordered_map<std::string, FunctionDecl> functions;
    std::unordered_map<std::string, StructDecl> structs;
    Value* currentSelf = nullptr; // non-null while executing a struct constructor

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
    if(auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
        Value& val = resolveVar(v->name);
        return val; // copy (arrays/struct fields copied by value, consistent with rest of language)
    }
    Value v; v.number = evalNumber(expr); return v;
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
void execute(const StmtPtr& stmt)
{
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