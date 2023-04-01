#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

using namespace std;

void perform_job(char* name);

int main(int argc, char *argv[]) {
    int period, exec_time, deadline;
    pid_t pid;
    auto init = chrono::steady_clock::now();

    chrono::steady_clock::time_point exec_start, finish;


    long wakeup_time, finish_time;

    period = atoi(argv[1]);
    exec_time = atoi(argv[2]);
    deadline = atoi(argv[3]);

    pid = getpid();

//    syscall(437, pid, period, deadline, exec_time);  // SYS_register_dm = 437

    int job = 1;
    do {
        exec_start = chrono::steady_clock::now();
        wakeup_time = chrono::duration_cast<chrono::milliseconds>(exec_start - init).count();
        cout << "Wakeup: " << wakeup_time << endl;

        perform_job(argv[4]);

        finish = chrono::steady_clock::now();
        finish_time = chrono::duration_cast<chrono::milliseconds>(finish - exec_start).count();
        cout << "Time to finish: " << finish_time << endl;
        cout << "Ran for " << finish_time - wakeup_time << "ms"<< endl;

//        syscall(444, pid);  //

        job++;

        // Scheduling should be performed after the first invocation of sys_yield()
    } while(job < 1);

//    syscall(439, pid);  //

    return 0;

}



void perform_job(char* name) {

    long long k = 0;
    for(int i = 0; i < 1e7; i++){
        ++k;
    }

    cout << "My name is " << name << " And I just computed " << k << endl;

}
