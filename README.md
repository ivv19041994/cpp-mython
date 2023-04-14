Mython

Дополнительный проект в курсе Яндекса.
Простой легковестный интерпритатор с синтоксисом похожим на python.

Интерфейсы классов и реализация парсера разработаны командой Яндекс Практикум.
Это хорошая отправня точка для изучения AST и его построения без сторонних зависимостей.

Build

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
