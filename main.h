#ifndef main_h
#define main_h

#include <limits.h>

int mainpid;
int cdshmgid;

void checkZombieProcesses(void);

void updateWorkingDir(void);

#endif
