CFLAGS=-I/usr/include/nlohmann -std=c++17 -g

jsoncpp: main.cpp
	g++ $(CFLAGS) main.cpp -o jsoncpp
