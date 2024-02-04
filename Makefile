CFLAGS=-I/usr/include/nlohmann -std=c++17

jsoncpp: main.cpp
	g++ $(CFLAGS) main.cpp -o jsoncpp
