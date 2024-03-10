CFLAGS=-I/usr/include/nlohmann -Iincludes -std=c++20 
SOURCES= main.cpp \
				 lsp.cpp \
				 workspace.cpp \
				 treesitter.cpp \
				 vendor/tree-sitter/libtree-sitter.a \
				 vendor/tree-sitter-javascript/libtree-sitter-javascript.a

lsp: $(SOURCES)
	g++ $(CFLAGS) $(SOURCES) -o lsp

tree-sitter-javascript:
	cd vendor/tree-sitter-javascript; \
	gcc -c parser.c scanner.c; \
	ar rcs libtree-sitter-javascript.a parser.o scanner.o 
