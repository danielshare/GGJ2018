GGJ2018: main.o
	g++ main.o -g -o GGJ2018 -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system

main.o: main.cpp
	g++ -g -c main.cpp

clean :
	-rm *.o $(objects) GGJ2018
