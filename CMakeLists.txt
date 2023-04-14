cmake_minimum_required(VERSION 3.11)

project(Mython CXX)
set(CMAKE_CXX_STANDARD 17)

if (CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

set(SOURCE_FILES lexer.cpp lexer_test_open.cpp
            main.cpp parse.cpp
			parse_test.cpp runtime.cpp
			runtime_test.cpp statement.cpp
			statement_test.cpp)

add_executable(mython ${SOURCE_FILES})