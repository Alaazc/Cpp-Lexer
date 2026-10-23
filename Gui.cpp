#include <windows.h>
#include <commdlg.h> 

#include <exception>
#include <string>
#include <vector>

#include "Lexer.cpp"
#include "Parser.cpp"

#ifdef _MSC_VER
#pragma comment(lib, "comdlg32.lib")
#endif

// Control IDs
#define ID_BTN_RUN 101
#define ID_EDIT_INPUT 102
#define ID_EDIT_OUTPUT 103
#define ID_BTN_OPEN 104
#define ID_BTN_PARSE 105

static HWND hInputBox = nullptr;
static HWND hOutputBox = nullptr;

std::string Token_TypeToString(Token_Type type) {
    switch (type) {
        case Token_Type::INT_LITERAL:    return "INT_LITERAL";
        case Token_Type::FLOAT_LITERAL:  return "FLOAT_LITERAL";
        case Token_Type::CHAR_LITERAL:   return "CHAR_LITERAL";
        case Token_Type::STRING_LITERAL: return "STRING_LITERAL";
        case Token_Type::BOOL_LITERAL:   return "BOOL_LITERAL";
        case Token_Type::KEYWORD:        return "KEYWORD";
        case Token_Type::IDENTIFIER:     return "IDENTIFIER";
        case Token_Type::OPERATOR:       return "OPERATOR";
        case Token_Type::PUNCTUATION:    return "PUNCTUATION";
        case Token_Type::END_OF_LINE:    return "EOF";
        case Token_Type::LEX_ERROR:      return "ERROR";
    }
    return "UNKNOWN";
}

static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), out.data(), size, nullptr, nullptr);
    return out;
}

static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring out(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), size);
    return out;
}


// File loading

static bool ReadFileToString(const wchar_t* path, std::string& out) {
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size;
    if (!GetFileSizeEx(h, &size)) {
        CloseHandle(h);
        return false;
    }

    out.resize(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    BOOL ok = out.empty() ? TRUE
                          : ReadFile(h, out.data(), (DWORD)out.size(), &read, nullptr);
    CloseHandle(h);
    if (!ok) return false;
    out.resize(read);

    // strip UTF-8 BOM if present
    if (out.size() >= 3 && (unsigned char)out[0] == 0xEF &&
        (unsigned char)out[1] == 0xBB && (unsigned char)out[2] == 0xBF) {
        out.erase(0, 3);
    }
    return true;
}

// Edit controls need \r\n line endings, but files usually have just \n
static std::string NormalizeNewlines(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '\r' && i + 1 < s.size() && s[i + 1] == '\n') continue; // drop \r, re-added below
        if (s[i] == '\n') out += "\r\n";
        else out += s[i];
    }
    return out;
}

static void OpenFileIntoInput(HWND hwnd) {
    wchar_t path[MAX_PATH] = {};

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hwnd;
    ofn.lpstrFilter = L"JSON files (*.json)\0*.json\0All files (*.*)\0*.*\0";
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&ofn)) return; 

    std::string content;
    if (!ReadFileToString(path, content)) {
        MessageBoxW(hwnd, L"Could not read the file.", L"Error", MB_OK | MB_ICONERROR);
        return;
    }

    SetWindowTextW(hInputBox, Utf8ToWide(NormalizeNewlines(content)).c_str());
}


// Lexer

static void RunLexer() {
    try {
        int length = GetWindowTextLengthW(hInputBox);
        if (length == 0) {
            SetWindowTextW(hOutputBox, L"Input is empty!");
            return;
        }

        std::wstring wide(length + 1, L'\0');
        int copied = GetWindowTextW(hInputBox, wide.data(), length + 1);
        wide.resize(copied);

        LEXER lexer(WideToUtf8(wide), 1, 1);
        std::vector<Token> tokens = lexer.Tokenize();

        std::string output;
        for (const auto& token : tokens) {
            output += "Token(" + Token_TypeToString(token.type) +
                      ", '" + token.lexeme +
                      "', line=" + std::to_string(token.line) +
                      ", col=" + std::to_string(token.column) + ")\r\n";
        }

        SetWindowTextW(hOutputBox, Utf8ToWide(output).c_str());
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Lexer Exception Error", MB_OK | MB_ICONERROR);
    } catch (...) {
        MessageBoxA(nullptr, "An unknown error occurred during tokenization.",
                    "Unknown Error", MB_OK | MB_ICONERROR);
    }
}

static void RunParser() {
    try {
        int length = GetWindowTextLengthW(hInputBox);
        if (length == 0) {
            SetWindowTextW(hOutputBox, L"Input is empty!");
            return;
        }

        std::wstring wide(length + 1, L'\0');
        int copied = GetWindowTextW(hInputBox, wide.data(), length + 1);
        wide.resize(copied);

        LEXER lexer(WideToUtf8(wide), 1, 1);
        std::string output = ParseTokensToString(lexer.Tokenize());

        SetWindowTextW(hOutputBox, Utf8ToWide(output).c_str());
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Parser Exception Error", MB_OK | MB_ICONERROR);
    } catch (...) {
        MessageBoxA(nullptr, "An unknown error occurred during parsing.",
                    "Unknown Error", MB_OK | MB_ICONERROR);
    }
}


// Window

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE inst = GetModuleHandleW(nullptr);
            HFONT mono = (HFONT)GetStockObject(ANSI_FIXED_FONT);

            CreateWindowW(L"STATIC", L"Clite / JSON Source Input:", WS_VISIBLE | WS_CHILD,
                          20, 10, 250, 20, hwnd, nullptr, inst, nullptr);

            hInputBox = CreateWindowW(
                L"EDIT",
                L"int main() {\r\n    int x = -42;\r\n    float y = 1.5e-3;\r\n    if (x > 10) {\r\n        x++;\r\n    }\r\n}",
                WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
                    ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
                20, 35, 740, 200, hwnd, (HMENU)ID_EDIT_INPUT, inst, nullptr);
            SendMessageW(hInputBox, EM_SETLIMITTEXT, 0, 0); // lift the default ~32K limit
            SendMessageW(hInputBox, WM_SETFONT, (WPARAM)mono, TRUE);

            CreateWindowW(L"BUTTON", L"Open File...",
                          WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                          190, 245, 120, 30, hwnd, (HMENU)ID_BTN_OPEN, inst, nullptr);

            CreateWindowW(L"BUTTON", L"Run Lexer",
                          WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON,
                          320, 245, 120, 30, hwnd, (HMENU)ID_BTN_RUN, inst, nullptr);

            CreateWindowW(L"BUTTON", L"Run Parser",
                          WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                          450, 245, 120, 30, hwnd, (HMENU)ID_BTN_PARSE, inst, nullptr);

            CreateWindowW(L"STATIC", L"Output (Tokens / AST):", WS_VISIBLE | WS_CHILD,
                          20, 285, 200, 20, hwnd, nullptr, inst, nullptr);

            hOutputBox = CreateWindowW(
                L"EDIT", L"",
                WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL |
                    ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                20, 310, 740, 210, hwnd, (HMENU)ID_EDIT_OUTPUT, inst, nullptr);
            SendMessageW(hOutputBox, EM_SETLIMITTEXT, 0, 0);
            SendMessageW(hOutputBox, WM_SETFONT, (WPARAM)mono, TRUE);
            break;
        }
        case WM_COMMAND:
            if (HIWORD(wp) == BN_CLICKED) {
                switch (LOWORD(wp)) {
                    case ID_BTN_RUN:  RunLexer(); break;
                    case ID_BTN_PARSE: RunParser(); break;
                    case ID_BTN_OPEN: OpenFileIntoInput(hwnd); break;
                }
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"CliteLexerClass";

    if (!RegisterClassW(&wc)) return 0;

    HWND hwnd = CreateWindowW(L"CliteLexerClass", L"Clite Lexical Analyzer (Native Win32)",
                              WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 800, 580,
                              nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (int)msg.wParam;
}
