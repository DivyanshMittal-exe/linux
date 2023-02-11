
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/types.h>


#ifndef _LINUX_SYS_H
#define _LINUX_SYS_H



struct pid_node {
    pid_t pid ; /* process id */
    struct list_head next_prev_list ; /* contains pointersA to previous and next elements */
};


struct pid_ctxt_switch {
    unsigned long ninvctxt; /* Count of involuntary context switches */
    unsigned long nvctxt; /* Count of voluntary context switches */
};

#endif