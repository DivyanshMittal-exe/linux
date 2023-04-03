CC=g++
CFLAGS=-Wall -std=c++17

all: test_1
	 ./test_1 1000 50 1000 "A" & \
	 ./test_1 1200 50 1200 "B" & \
	 ./test_1 10000 50 10000 "Slowest"

test_1: test_1.o
	$(CC) $(CFLAGS) -o test_1 test_1.o

test_1.o: test_1.cpp
	$(CC) $(CFLAGS) -c test_1.cpp

clean:
	rm -f test_1 test_1.o program1 program2 program3 program4

run:
	 ./test_1 100 50 100 "A" & \
	 ./test_1 120 50 120 "B" & \
	 ./test_1 400 50 400 "C"

pcp: program1 program2 program3 program4
	./program1 50 100 100 & \
	./program2 35 300 300 & \
	./program3 40 200 200 & \
	./program4 40 150 150


program1: program1.cpp
	$(CXX) $(CXXFLAGS) -o program1 program1.cpp

program2: program2.cpp
	$(CXX) $(CXXFLAGS) -o program2 program2.cpp

program3: program3.cpp
	$(CXX) $(CXXFLAGS) -o program3 program3.cpp

program4: program4.cpp
	$(CXX) $(CXXFLAGS) -o program4 program4.cpp
