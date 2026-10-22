//
// Created by alari on 10/15/2026.
//

#include "Lexer.h"
#include <cctype>
#include <ranges>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

enum class Token_Type {
     INT_LITERAL,
     FLOAT_LITERAL,
     STRING_LITERAL,
     BOOL_LITERAL,
     CHAR_LITERAL,
     IDENTIFIER,
     OPERATOR,
     PUNCTUATION,
     LEX_ERROR,
     END_OF_LINE,
     KEYWORD,
 };

struct Token {
    Token_Type type;
    std::string lexeme;
    int line;
    int column;

    Token(Token_Type t, std::string lex, int l, int c): type(t), lexeme(std::move(lex)),line(l), column(c){

    }

};

struct LEXER {
    std::string source;
    size_t length;
    size_t pos = 0;
    int line = 1;
    int column = 1;

    LEXER(std::string src, int lineNum, int colNum):
        source(std::move(src)), length(source.length()), line(lineNum),column(colNum) {

    }

    [[nodiscard]] char peek(size_t offset = 0) const {
        size_t i = pos + offset;
        if (i < length) {
            return source[i];
        } else {
            return '\0';
        }
    }

    [[nodiscard]] static bool isDigitChar(char c) {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    }

    void incrementPos(int current) {
        for (int i = 0; i < current; i++) {
            if (pos >= length) {
                return;
            }
            if (source[pos] == '\n') {
                line++;
                column = 1;
            }else {
                column++;
            }
            pos++;
        }
    }

    void skipComments() {
        while (pos < length && peek() != '\n') {
            incrementPos(1);
        }
    }

    void skipBlockComments(std::vector<Token>& tokens) {
        int startLine = line;
        int startCol = column;

        incrementPos(2); // Skip opening '/*'

        while (pos < length) {
            // Check for closing '*/'
            if (peek() == '*' && peek(1) == '/') {
                incrementPos(2); // Skip closing '*/'
                return;
            }
            incrementPos(1);
        }

        // If loop finishes without returning, the comment was never closed
        tokens.emplace_back(Token_Type::LEX_ERROR, "Unterminated block comment", startLine, startCol);
    }

    [[nodiscard]] static bool previousEndsOperand(const std::vector<Token>& tokens) {
        if (tokens.empty()) {
            return false;
        }
        const Token& prev = tokens.back();
        switch (prev.type) {
            case Token_Type::INT_LITERAL:
            case Token_Type::FLOAT_LITERAL:
            case Token_Type::STRING_LITERAL:
            case Token_Type::CHAR_LITERAL:
            case Token_Type::BOOL_LITERAL:
            case Token_Type::IDENTIFIER:
                return true;
            case Token_Type::PUNCTUATION:
                return prev.lexeme == ")" || prev.lexeme == "]";
            default:
                return false;
        }
    }

    std::vector<Token> Tokenize() {
        std::vector<Token> tokens;

        while (pos < length) {
            char currentChar = peek();

            // handle whitespace
            if (currentChar == ' ' || currentChar == '\t' || currentChar == '\n' || currentChar == '\r') {
                incrementPos(1);
                continue;
            }

            // skip comment
            if (currentChar == '/' && peek(1) == '/') {
                skipComments();
                continue;
            }

            // skip comment block
            if (currentChar == '/' && peek(1) == '*') {
                skipBlockComments(tokens);
                continue;
            }

            // 2 char operators
            std::unordered_set<std::string> twoCharOps = {"==", "!=", "<=", ">=", "&&", "||", "++", "--", "+=", "-=", "*=", "/="};
            std::string StringOp = source.substr(pos, 2);
            if (twoCharOps.contains(StringOp)) {
                tokens.emplace_back(Token_Type::OPERATOR,StringOp, line, column);
                incrementPos(2);
                continue;
            }

            // one char operators
            static const std::unordered_set<char> singleOp = {'+', '-', '*', '/', '=', '!', '<', '>'};
            char charOp = source[pos];
            if (singleOp.contains(charOp)) {
                std::string lexeme(1, charOp);
                tokens.emplace_back(Token_Type::OPERATOR, lexeme, line, column);
                incrementPos(1);
                continue;
            }

            // Punctuation/containers
            static const std:: unordered_set<char> punc = {'(', ')', '{', '}', '[', ']', ';', ',', ':'};
            char PuncOp = source[pos];
            if (punc.contains(PuncOp)) {
                std::string lexeme(1, PuncOp);
                tokens.emplace_back(Token_Type::PUNCTUATION, lexeme, line, column);
                incrementPos(1);
                continue;
            }

            // number literals
            // number literals: 42, 3.14, 1e5, 2.5E-3, and negative versions of each
            bool negativeNumber = currentChar == '-' && isDigitChar(peek(1)) && !previousEndsOperand(tokens);
            if (isDigitChar(currentChar) || negativeNumber) {
                int startCol = column;
                std::string numStr;
                bool isFloat = false;

                if (negativeNumber) {
                    numStr += '-';
                    incrementPos(1);
                }

                // integer part
                while (isDigitChar(peek())) {
                    numStr += peek();
                    incrementPos(1);
                }

                // fractional part (only if a digit follows the dot)
                if (peek() == '.' && isDigitChar(peek(1))) {
                    isFloat = true;
                    numStr += '.';
                    incrementPos(1);
                    while (isDigitChar(peek())) {
                        numStr += peek();
                        incrementPos(1);
                    }
                }

                // exponent part: e / E, optional sign, at least one digit
                if (peek() == 'e' || peek() == 'E') {
                    size_t offset = 1;
                    if (peek(offset) == '+' || peek(offset) == '-') {
                        offset++;
                    }
                    if (isDigitChar(peek(offset))) {
                        isFloat = true;
                        for (size_t i = 0; i < offset; i++) { // the 'e' and the sign
                            numStr += peek();
                            incrementPos(1);
                        }
                        while (isDigitChar(peek())) {
                            numStr += peek();
                            incrementPos(1);
                        }
                    }
                }

                tokens.emplace_back(isFloat ? Token_Type::FLOAT_LITERAL : Token_Type::INT_LITERAL,
                                    numStr, line, startCol);
                continue;
            }

            //char literal
            if (currentChar == '\'') {
                int startCol = column;
                std::string lexeme(1, currentChar);
                incrementPos(1);

                if (pos < length) {
                    lexeme += source[pos];
                    incrementPos(1);
                }

                if (pos < length && source[pos] == '\'') {
                    lexeme += '\'';
                    incrementPos(1);
                    tokens.emplace_back(Token_Type::CHAR_LITERAL, lexeme, line, startCol);
                } else {
                    tokens.emplace_back(Token_Type::LEX_ERROR, lexeme, line, startCol);
                }
                continue;
            }

            // string literal
            if (currentChar == '"') {
                int startCol = column;
                std::string lexeme(1, '"');
                incrementPos(1);

                bool terminated = false;
                while (pos < length) {
                    char c = peek();
                    if (c == '\n'){
                        break;
                    }

                    // unterminated
                    if (c == '\\' && pos + 1 < length && peek(1) != '\n') {
                        lexeme += c;
                        lexeme += peek(1);
                        incrementPos(2);
                        continue;
                    }

                    lexeme += c;
                    incrementPos(1);
                    if (c == '"') { terminated = true;
                        break; }
                }

                if (terminated) {
                    tokens.emplace_back(Token_Type::STRING_LITERAL, lexeme, line, startCol);
                } else {
                    tokens.emplace_back(Token_Type::LEX_ERROR, "Unterminated string", line, startCol);
                }
                continue;
            }
            // identifiers and keywords
            if (isalpha(static_cast<unsigned char>(currentChar)) || currentChar == '_') {
                static const std::unordered_set<std::string> KEYWORDS =
                    {"int", "bool", "float", "char", "main", "if", "else", "while", "NULL"};
                static const std::unordered_set<std::string> BOOLS = {"true", "false"};

                int startCol = column;
                std::string identStr;
                while (pos < length && (isalnum(static_cast<unsigned char>(source[pos])) || source[pos] == '_')) {
                    identStr += source[pos];
                    incrementPos(1);
                }

                if (BOOLS.contains(identStr)) {
                    tokens.emplace_back(Token_Type::BOOL_LITERAL, identStr, line, startCol);
                }else if (KEYWORDS.contains(identStr)) {
                    tokens.emplace_back(Token_Type::KEYWORD, identStr, line, startCol);
                }else {
                    tokens.emplace_back(Token_Type::IDENTIFIER, identStr, line, startCol);
                }
                continue;
            }

            // anything else
            tokens.emplace_back(Token_Type::LEX_ERROR, "Unknown character", line, column);
            incrementPos(1);
        }   // end while

        tokens.emplace_back(Token_Type::END_OF_LINE, "", line, column);
        return tokens;
    }
};