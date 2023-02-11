#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <iostream>
#include "cst.h"

using namespace std;

int main(int argc, char **argv) {


    if(atoi(argv[1]) == 452){

        struct pid_ctxt_switch pp;
        long ret = syscall(atoi(argv[1]), &pp);

        std::cout << "ninvctxt " << pp.ninvctxt << std::endl;
        std::cout << "nvctxt " << pp.nvctxt << std::endl;


        std::cout << "Syscall output " << ret << std::endl;
    

    }else{
        pid_t pid = std::stoi(argv[2]);

        long ret = syscall(atoi(argv[1]), pid);

        std::cout << "Syscall output " << ret << std::endl;

    }

    return 0;
}
