Mython

Additional project in the course of Yandex.
A simple lightweight interpreter with python-like syntax.

The class interfaces and the parser implementation were developed by the Yandex Practicum team.
This is a good starting point for learning the AST and building it without third party dependencies.

Build:

Build tested on Windows 10. 

Tested with mingw https://winlibs.com/ (g++ 12.2.0 + MinGW Makefiles)

Tested with Visual Studio 17 2022

Commands with gnu utils for Windows:

cd mython_dir

mkdir debug

cd debug

cmake -G "MinGW Makefiles" .. -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug

cmake --build . -j


Run:
echo print "Hello world" >> test.mython
cat test.mython | mython >> result.txt

You will see "Hello world" in the result.txt

Dependence:
free

Evolution:

-add cycles 

-add single functions
