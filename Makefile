obj-m = driver.o

PID := 2752

KVERSION = $(shell uname -r)
KDIR := /lib/modules/$(shell uname -r)/build


all:
	make -C $(KDIR) /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

eve:
	sudo rmmod driver
	make -C $(KDIR) /lib/modules/$(KVERSION)/build M=$(PWD) modules
	sudo insmod driver.ko
	g++ -Wall -Wextra -std=c++17 -pthread tester.cpp -o test
	sudo ./test


test: tester.cpp
	g++ -Wall -Wextra -std=c++17 -pthread tester.cpp -o test

test_run: test
	./test

test_lkm: tester_lkm.cpp
	g++ -Wall -Wextra -std=c++17 -pthread tester_lkm.cpp -o test_lkm

test_run_lkm: test_lkm
	./test_lkm