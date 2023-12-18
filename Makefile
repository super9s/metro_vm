all:
	g++ -D_METRO_DEBUG_ -std=c++20 -g -O0 $(wildcard *.cpp) -o lang