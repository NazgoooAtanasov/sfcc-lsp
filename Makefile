CFLAGS=-I/usr/include/nlohmann -Iincludes -std=c++20 

lsp: main.cpp lsp.cpp
	g++ $(CFLAGS) main.cpp lsp.cpp -o lsp
