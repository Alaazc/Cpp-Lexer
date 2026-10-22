cmake_minimum_required(VERSION 3.16)
project(Cpp_Lexer CXX)

set(CMAKE_CXX_STANDARD 20)

# Add your source files (no external find_package needed!)
add_executable(Cpp_Lexer WIN32 Gui.cpp Lexer.cpp Lexer.h)

# Link Windows system libraries if needed (MinGW handles most automatically)
target_link_libraries(Cpp_Lexer)