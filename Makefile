CFLAGS=-I/usr/include/nlohmann -std=c++20 -g

lsp: main.cpp
	g++ $(CFLAGS) main.cpp -o lsp
