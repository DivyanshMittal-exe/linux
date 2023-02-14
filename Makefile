obj-m = signal_module.o

PID := 2150

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

test:
	echo "$(PID), 19" > /proc/sig_target; echo "$(PID), 18" > /proc/sig_target;
