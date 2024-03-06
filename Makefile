CFLAGS=-I/usr/include/nlohmann -Iincludes -std=c++20 
SOURCES= main.cpp \
				 lsp.cpp \
				 workspace.cpp

lsp: $(SOURCES)
	g++ $(CFLAGS) $(SOURCES) -o lsp
