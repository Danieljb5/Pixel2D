:: Create a PATH variable pointing to the directory that contains g++.exe
:: Make sure you are using MinGW 64 bit
:: To build an example, run the script with the name of the example excluding the file extension
:: e.g. buildExamples.bat example

mkdir build
copy ..\bin build
g++ -c %1.cpp -I..\ -I..\asio-1.18.2\include -I . -w -lpthread -g -Wall -Wextra -Werror -pedantic -std=c++17
g++ -O *.o -o build\%1.exe -I ..\ -I ..\asio-1.18.2\include -I . -lsfml-graphics -lsfml-window -lsfml-system -lpthread -g -Wall -Wextra -Werror -pedantic -std=c++17 -L ..\lib
del *.o
mkdir build\fonts
mkdir build\textures
copy fonts\* build\fonts
copy textures\* build\textures
