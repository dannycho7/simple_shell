main: shell.o parser.o
	g++ -std=c++11 shell.o parser.o -o shell.out
shell.o:
	g++ -std=c++11 -c shell.cpp
parser.o:
	g++ -std=c++11 -c parser.cpp
clean:
	rm *.out *.o