# Clite Lexer and Parser

A hand-written lexer and recursive-descent parser for **Clite**, a small C-like teaching language, written in C++20 with a native Windows GUI built directly on the Win32 API (no frameworks or external libraries).

Type or load source code, then click a button to see either the token stream or the abstract syntax tree (AST).

## Features

### Lexer
- Tokenizes source into 11 token types: integer, float, string, boolean and character literals, identifiers, keywords, operators, punctuation, errors and end of input
- Tracks the line and column of every token
- Numbers: integers, decimals, scientific notation (`1e5`, `2.5E-3`), and negative literals
- Negative numbers are context-sensitive: in `x = -5` the `-` is part of the number, but in `x - 5` it is a binary operator
- String and character literals, including escape sequences such as `\"` and `\\`
- Line comments (`//`) and block comments (`/* */`)
- Reports unterminated strings, unterminated block comments and unknown characters with their location

### Parser
- Recursive descent, with one function per grammar rule
- Correct operator precedence through layered expression rules
- Builds an AST from a polymorphic node hierarchy (`std::unique_ptr` and virtual `print()`)
- Reports the first syntax error with its line, column and the offending token
- Reports lexical errors before parsing starts

### GUI
- Native Win32 window with a source editor and an output pane
- **Open File...** loads `.json` or any text file (UTF-8, with or without a BOM)
- **Run Lexer** shows the token stream
- **Run Parser** shows the indented AST or the parse error

## Example

Input:

```c
int main() {
    int x = -42;
    if (x > 10) {
        x++;
    }
}
```

**Run Lexer** output (excerpt):

```
Token(KEYWORD, 'int', line=1, col=1)
Token(KEYWORD, 'main', line=1, col=5)
Token(PUNCTUATION, '(', line=1, col=9)
...
Token(INT_LITERAL, '-42', line=2, col=13)
```

**Run Parser** output:

```
Parse successful. AST:

Program
  Function main() -> int
    Block
      Declaration(int x)
        Literal(int, -42)
      If
        Condition:
          Binary(>)
            Variable(x)
            Literal(int, 10)
        Then:
          Block
            IncDec(x++)
```

A parse error looks like this:

```
Parse error at line 1, col 22: Expected an expression but found ';' (PUNCTUATION)
```

## Grammar

The parser implements the following (an extended subset of the Clite grammar):

```
Program     -> "int" "main" "(" ")" Block
Block       -> "{" (Declaration | Statement)* "}"
Declaration -> Type IDENTIFIER ("=" Expression)? ";"
Statement   -> Block | IfStmt | WhileStmt | Assignment
Assignment  -> IDENTIFIER ("=" | "+=" | "-=" | "*=" | "/=") Expression ";"
             | IDENTIFIER ("++" | "--") ";"
IfStmt      -> "if" "(" Expression ")" Statement ("else" Statement)?
WhileStmt   -> "while" "(" Expression ")" Statement

Expression  -> Conjunction ("||" Conjunction)*
Conjunction -> Equality ("&&" Equality)*
Equality    -> Relation (("==" | "!=") Relation)?
Relation    -> Addition (("<" | "<=" | ">" | ">=") Addition)?
Addition    -> Term (("+" | "-") Term)*
Term        -> Unary (("*" | "/") Unary)*
Unary       -> ("-" | "!") Unary | Primary
Primary     -> IDENTIFIER | literal | "(" Expression ")"
```

## Project structure

```
.
├── Lexer.cpp       # Token types, Token, and the LEXER class
├── Parser.cpp      # AST nodes, Parser class, ParseTokensToString()
├── main.cpp        # Win32 GUI (window, buttons, file loading)
├── test.json       # Sample file for exercising the lexer
└── CMakeLists.txt
```

`Lexer.cpp` and `Parser.cpp` are included directly by `main.cpp` (a single translation unit), and both start with `#pragma once`.

## Building

**Requirements:** Windows, a C++20 compiler (MSVC or MinGW-w64), and CMake 3.20 or newer. CLion works well.

Example `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(CliteLexerParser CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(CliteLexerParser WIN32 main.cpp)
target_link_libraries(CliteLexerParser comdlg32)
```

Build from the command line:

```bash
cmake -S . -B build
cmake --build build
```

Or open the folder in CLion and press **Run**. Only `main.cpp` goes in `add_executable`, because it includes the other two files.

## Usage

1. Run the program.
2. Type Clite code into the input box, or click **Open File...**.
3. Click **Run Lexer** to see tokens, or **Run Parser** to see the AST.

Try `test.json` with **Run Lexer** to see negative numbers, exponents, escaped strings and nested punctuation. The parser expects Clite code, so it will report errors on JSON.

## Known limitations

- The parser checks syntax only. There is no type checking, and undeclared variables are not detected.
- It stops at the first error and does not recover.
- It differs from the official Clite grammar in a few places. It accepts `++`, `--`, compound assignment (`+=` and so on), initializers (`int x = 5;`) and declarations anywhere in a block. It does not yet support arrays, type casts, the `%` operator, or multiple declarators in one declaration (`int a, b;`).
- `null` in JSON is tokenized as an identifier.
- Windows only, because of the Win32 GUI. The lexer and parser themselves are portable C++.

## Possible next steps

- Bring the parser in line with the full Clite grammar (arrays, casts, `%`, multiple declarators)
- Add a semantic analysis pass with a symbol table and type checking
- Add error recovery so several errors are reported in one run
- Split the code into proper headers and source files
