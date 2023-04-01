CC=g++
CFLAGS=-Wall -std=c++17

all: test_1
	 ./test_1 1000 200 1500 "A" & \
	 ./test_1 1500 300 2000 "B" & \
	 ./test_1 2000 400 2500 "C"

test_1: test_1.o
	$(CC) $(CFLAGS) -o test_1 test_1.o

test_1.o: test_1.cpp
	$(CC) $(CFLAGS) -c test_1.cpp

clean:
	rm -f test_1 test_1.o

run:
	nohup ./test_1 1000 200 1500 "A" & \
	nohup ./test_1 1500 300 2000 "B" & \
	nohup ./test_1 2000 400 2500 "C" &
