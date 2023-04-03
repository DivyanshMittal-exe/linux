#define PID getpid()

void acquire_lock(int rid) {
    syscall(458, PID, rid);
}

void release_lock(int rid) {
    syscall(459, PID, rid);
}



void register_task(int period, int deadline, int exec_time) {
    syscall(451, PID, period, deadline, exec_time);
}

void deregister_task() {
    syscall(454, PID);
}


void yield() {
    syscall(453, PID);
}