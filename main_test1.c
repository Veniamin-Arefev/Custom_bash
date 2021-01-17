#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "functionsList.h"
#include "main.h"

int *backgroundPids;
int backgroundpidCount;
int backgroundpidLength;

void checkZombieProcesses(void) {

};

int main() {
//    signal(SIGINT, SIG_IGN);
    char **mylist;
    while ((mylist = getNewList()) != NULL) {
        printlist(mylist);

    }
    printf("Exiting...\n");
}
