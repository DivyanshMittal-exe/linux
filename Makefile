all: test
	./test $(A)

test: test.cpp
	g++ test test.cpp


clean:
	rm -f test
