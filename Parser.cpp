//
// Created by alari on 10/22/2026.
//
#define CPP_LEXER_PARSER_H
#include <memory>

#include "Lexer.cpp"

struct ParseError {
    std :: string message;
    int line;
    int column;
};

inline std::string Pad (int indent){
    return std::string(static_cast<size_t>(indent) * 2,' ');
}

//expressions
struct Expr {
    virtual ~Expr() = default;
    virtual void print(std::string& out, int indent) const = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

struct LiteralExpr : Expr {
    std::string value;
    std::string kind;

    LiteralExpr(std::string k,std::string v ) : kind(std::move(k)), value(std::move(v)) {}

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + " " + "Literal(" + kind + "," + value + ")\r\n";
    }
};

struct VariableExpr : Expr {
    std::string name;

    explicit VariableExpr(std::string n) : name(std::move(n)) {}

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + " " + "Variable(" + name + ")\r\n";
    }
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr operand;

    UnaryExpr(std::string o,ExprPtr e) : op(std::move(o)), operand(std::move(e)) {}
    void print(std::string& out, int indent) const override {
        out += Pad(indent) + " " + "Unary(" + op + ")\r\n";
        operand->print(out, indent);
    }
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr left, right;

    BinaryExpr(std::string o, ExprPtr l, ExprPtr r): op(std::move(o)), left(std::move(l)), right(std::move(r)) {}

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + " " + "Binary(" + op + ")\r\n";
        left->print(out, indent + 1);
        right->print(out, indent + 1);
    }
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual void print(std::string& out, int indent) const = 0;
};

using StmtPtr = std::unique_ptr<Stmt>;

struct DeclStmt : Stmt {
    std::string name;
    std::string type;

    ExprPtr init;

    DeclStmt(std::string t, std::string n, ExprPtr i): name(std::move(n)), type(std::move(t)), init(std::move(i)) {}

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + "Declaration(" + type + " " + name + ")\r\n";
        if (init) init->print(out, indent + 1);
    }
};

struct AssignStmt : Stmt {
    std::string name;
    std::string op;

    ExprPtr value;

    AssignStmt(std::string n, std::string o, ExprPtr v) : name(std::move(n)), op(std::move(o)), value(std::move(v)) {}

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + "Assign(" + name + "," + op + ")\r\n";
        value->print(out, indent + 1);
    }
};

struct IncDecStmt : Stmt {
    std::string name;
    std::string op;

    IncDecStmt(std::string n, std::string o) : name(std::move(n)), op(std::move(o)) {}

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + "IncDec(" + name + ")\r\n";
    }
};

struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + "Block\r\n";
        for (const auto& s : statements) s->print(out, indent + 1);
    }
};

struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + "If\r\n";
        out += Pad(indent + 1) + "Condition\r\n";

        condition->print(out, indent + 2);

        out += Pad(indent + 1) + "Then\r\n";
        thenBranch->print(out, indent + 2);
        if (elseBranch) {
            out += Pad(indent + 1) + "Else\r\n";
            elseBranch->print(out, indent + 2);
        }
    }
};

struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;

    void print(std::string& out, int indent) const override {
        out += Pad(indent) + "While\r\n";
        out += Pad(indent + 1) + "Condition\r\n";
        condition->print(out, indent + 2);
        out += Pad(indent + 1) + "Body\r\n";
        body->print(out, indent + 2);
    }
};

struct ProgramNode {
    std::unique_ptr<BlockStmt> body;

    void print(std::string& out) const {
        out += "Program\r\n";
        out += Pad(1) + "Function main() -> \r\n";
        body->print(out, 2);
    }
};

// ---------------------------------------------------------------------------
// Parser (recursive descent)
//
// Program     -> "int" "main" "(" ")" Block
// Block       -> "{" (Declaration | Statement)* "}"
// Declaration -> Type IDENTIFIER ("=" Expression)? ";"
// Statement   -> Block | IfStmt | WhileStmt | Assignment
// Assignment  -> IDENTIFIER ("=" | "+=" | "-=" | "*=" | "/=") Expression ";"
//              | IDENTIFIER ("++" | "--") ";"
// IfStmt      -> "if" "(" Expression ")" Statement ("else" Statement)?
// WhileStmt   -> "while" "(" Expression ")" Statement
//
// Expression  -> Conjunction ("||" Conjunction)*
// Conjunction -> Equality ("&&" Equality)*
// Equality    -> Relation (("==" | "!=") Relation)?
// Relation    -> Addition (("<" | "<=" | ">" | ">=") Addition)?
// Addition    -> Term (("+" | "-") Term)*
// Term        -> Unary (("*" | "/") Unary)*
// Unary       -> ("-" | "!") Unary | Primary
// Primary     -> IDENTIFIER | literal | "(" Expression ")"
// ---------------------------------------------------------------------------



struct Parser {
    std::vector<Token> tokens;
    size_t current_token = 0;

    explicit Parser(std::vector<Token> t) : tokens(std::move(t)) {
        if (tokens.empty() || tokens.back().type != Token_Type::END_OF_LINE) {
            tokens.emplace_back(Token_Type::END_OF_LINE, "" , 1, 1);
        }
    }


    [[nodiscard]] const Token& peek() const {return tokens[current_token];}

    [[nodiscard]] bool atEnd() const {return peek().type == Token_Type::END_OF_LINE;}

    const Token& advance() {
        const Token& t = tokens[current_token];

        if (!atEnd()) current_token++;

        return t;
    }

    [[nodiscard]] static std::string describe(const Token& t) {
        if (t.type == Token_Type::END_OF_LINE) return "end of input";
        static const char* names[] = {"INT_LITERAL", "FLOAT_LITERAL", "STRING_LITERAL",
            "BOOL_LITERAL", "CHAR_LITERAL", "IDENTIFIER", "OPERATOR", "PUNCTUATION",
            "LEX_ERROR", "END_OF_LINE", "KEYWORD"};
        return "'" + t.lexeme + "' (" + names[static_cast<int>(t.type)] + ")";
    }

    [[noreturn]] void fail(const std::string& message) const {
        throw ParseError{message, peek().line, peek().column};
    }

    [[nodiscard]] bool check(Token_Type type, const std::string& lexeme) const {
        return peek().type == type && peek().lexeme == lexeme;
    }

    bool match(Token_Type type, const std::string& lexeme) {
        if (check(type, lexeme)) {
            advance();
            return true;
        }
        return false;
    }

    void expect(Token_Type type, const std::string& lexeme) {
        if (!check(type, lexeme)) {
            fail("Expected '" + lexeme + "' but found " + describe(peek()));
        }
        advance();
    }

    // If the current token is one of the given operators, consume it and store it in 'matched'.
    bool matchAnyOp(std::initializer_list<const char*> ops, std::string& matched) {
        if (peek().type != Token_Type::OPERATOR) return false;
        for (const char* op : ops) {
            if (peek().lexeme == op) {
                matched = op;
                advance();
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] static bool isTypeKeyword(const std::string& s) {
        return s == "int" || s == "bool" || s == "float" || s == "char";
    }

    // ----- program / statements -----

    std::unique_ptr<ProgramNode> parseProgram() {
        //errors
        for (const auto& t : tokens) {
            if (t.type == Token_Type::LEX_ERROR) {
                throw ParseError{"Lexical error: " + t.lexeme, t.line, t.column};
            }
        }

        expect(Token_Type::KEYWORD, "int");
        expect(Token_Type::KEYWORD, "main");
        expect(Token_Type::PUNCTUATION, "(");
        expect(Token_Type::PUNCTUATION, ")");

        auto program = std::make_unique<ProgramNode>();
        program->body = parseBlock();

        if (!atEnd()) {
            fail("Unexpected "+ describe(peek()) + " after end of program");
        }
        return program;
    }

    std::unique_ptr<BlockStmt> parseBlock() {
        expect(Token_Type::PUNCTUATION, "{");
        auto block = std::make_unique<BlockStmt>();
        while (!check(Token_Type::PUNCTUATION, "}")) {
            if (atEnd()) fail("Expected '}' but reached end of input");
            block->statements.push_back(parseStatement());
        }
        expect(Token_Type::PUNCTUATION, "}");
        return block;
    }

    StmtPtr parseStatement() {
        const Token& t = peek();

        if (t.type == Token_Type::PUNCTUATION && t.lexeme == "{") return parseBlock();

        if (t.type == Token_Type::KEYWORD) {
            if (t.lexeme == "if") return parseIf();
            if (t.lexeme == "while") return parseWhile();
            if (isTypeKeyword(t.lexeme)) return parseDeclaration();
        }

        if (t.type == Token_Type::IDENTIFIER) return parseAssignment();

        fail("Expected a statement but found " + describe(t));
    }

    StmtPtr parseDeclaration() {
        std::string type = advance().lexeme;

        if (peek().type != Token_Type::IDENTIFIER) {
            fail("Expected a variable name after '" + type  + "' but found " + describe(peek()));
        }
        std::string name = advance().lexeme;

        ExprPtr init;
        if (match(Token_Type::OPERATOR, "=")) {
            init = parseExpression();
        }
        expect(Token_Type::PUNCTUATION, ";");
        return std::make_unique<DeclStmt>(type, name, std::move(init));
    }

    StmtPtr parseAssignment() {
        std::string name = advance().lexeme;

        if (peek().type == Token_Type::OPERATOR) {
            std::string op = peek().lexeme;

            if (op == "++" || op == "--") {
                advance();
                expect(Token_Type::PUNCTUATION, ";");
                return std::make_unique<IncDecStmt>(name, op);   // return here
            }
            if (op == "=" || op == "+=" || op == "-=" || op == "*=" || op == "/=") {
                advance();
                ExprPtr value = parseExpression();
                expect(Token_Type::PUNCTUATION, ";");
                return std::make_unique<AssignStmt>(name, op, std::move(value));
            }
        }

        fail("Expected an assignment operator after '" + name + "' but found " + describe(peek()));
    }

    StmtPtr parseIf() {
        expect(Token_Type::KEYWORD, "if");
        expect(Token_Type::PUNCTUATION, "(");
        auto node = std::make_unique<IfStmt>();
        node->condition = parseExpression();
        expect(Token_Type::PUNCTUATION, ")");
        node->thenBranch = parseStatement();
        if (match(Token_Type::KEYWORD, "else")) {
            node->elseBranch = parseStatement();
        }
        return node;
    }

    StmtPtr parseWhile() {
        expect(Token_Type::KEYWORD, "while");
        expect(Token_Type::PUNCTUATION, "(");

        auto node = std::make_unique<WhileStmt>();
        node->condition = parseExpression();

        expect(Token_Type::PUNCTUATION, ")");
        node->body = parseStatement();
        return node;
    }

    ExprPtr parseExpression() {
        return parseOr();
    }

    ExprPtr parseOr() {
        ExprPtr left = parseAnd();
        std::string op;
        while (matchAnyOp({ "||"}, op)) {
            ExprPtr right = parseAnd();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parseAnd() {
        ExprPtr left = parseEquality();
        std::string op;
        while (matchAnyOp({"&&"}, op)) {
            ExprPtr right = parseEquality();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parseEquality() {
        ExprPtr left = parseRelation();
        std::string op;
        if (matchAnyOp({"==", "!="}, op)) {
            ExprPtr right = parseRelation();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parseRelation() {
        ExprPtr left = parseAddition();
        std::string op;
        if (matchAnyOp({"<=", ">=", "<", ">"}, op)) {
            ExprPtr right = parseAddition();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parseAddition() {
        ExprPtr left = parseTerm();
        std::string op;
        while (matchAnyOp({"+", "-"}, op)) {
            ExprPtr right = parseTerm();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parseTerm() {
        ExprPtr left = parseUnary();
        std::string op;
        while (matchAnyOp({"*", "/"}, op)) {
            ExprPtr right = parseUnary();
            left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parseUnary() {
        std::string op;
        if (matchAnyOp({"-", "!"}, op)) {
            return std::make_unique<UnaryExpr>(op, parseUnary());
        }
        return parsePrimary();
    }

    ExprPtr parsePrimary() {
        const Token& t = peek();

        switch (t.type) {
            case Token_Type::INT_LITERAL:    return literal("int");
            case Token_Type::FLOAT_LITERAL:  return literal("float");
            case Token_Type::BOOL_LITERAL:   return literal("bool");
            case Token_Type::CHAR_LITERAL:   return literal("char");
            case Token_Type::STRING_LITERAL: return literal("string");
            case Token_Type::IDENTIFIER:
                return std::make_unique<VariableExpr>(advance().lexeme);
            case Token_Type::PUNCTUATION:
                if (t.lexeme == "(") {
                    advance();
                    ExprPtr inner = parseExpression();
                    expect(Token_Type::PUNCTUATION, ")");
                    return inner;
                }
                break;
            default:
                break;
        }

        fail("Expected an expression but found " + describe(t));
    }

    ExprPtr literal(const std::string& kind) {
        return std::make_unique<LiteralExpr>(kind, advance().lexeme);
    }
};

inline std::string ParseTokensToString(std::vector<Token> tokens) {
    try {
        Parser parser(std::move(tokens));
        auto program = parser.parseProgram();

        std::string out = "Parse successful. AST:\r\n\r\n";
        program->print(out);
        return out;
    } catch (const ParseError& e) {
        return "Parse error at line " + std::to_string(e.line) +
               ", col " + std::to_string(e.column) + ": " + e.message + "\r\n";
    }
}
