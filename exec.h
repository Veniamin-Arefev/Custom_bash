#ifndef exec_h
#define exec_h

#include "tree.h"

enum {
    PIDS_SIZE = 5,
    SLEEP_TIME = 1000,
};

int execute_tree(cmd_inf *tree); // creates separate process for executing new shell expression and deal with them
//void checkZombieProcesses(intlist *list);
#endif
