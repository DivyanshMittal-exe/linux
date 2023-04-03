#include <chrono>
#include <iostream>
#include <unistd.h>
#include <sys/syscall.h>

#include <thread>
#include "common.h"


void register_resources() {
    syscall(456, PID, 1);
    syscall(456, PID, 2);
}

int main(int argc, char *argv[]) {
    int exec_time = std::stoi(argv[1]);
    int period = std::stoi(argv[2]);
    int deadline = std::stoi(argv[3]);

    register_task(period, deadline, exec_time);


    register_resources();
     std::this_thread::sleep_for(std::chrono::milliseconds(100));


    std::chrono::steady_clock::time_point init = std::chrono::steady_clock::now();
    for (int i = 0; i < 10; i++) {
        std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

        auto strt_loc = std::chrono::duration_cast<std::chrono::milliseconds>(start_time - init).count();
        std::cout << "Program 3 started at " << strt_loc << "ms Period: " << period << std::endl;

        acquire_lock(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(exec_time/2));

        acquire_lock(2);
        std::this_thread::sleep_for(std::chrono::milliseconds(exec_time/2));

        release_lock(2);
        release_lock(1);

        std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
        auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        std::cout << "Program 3 executed in " << execution_time << "ms" << std::endl;
        yield();
    }

    deregister_task();


    return 0;
}
