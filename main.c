#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>

#include "functionsList.h"
#include "tree.h"
#include "exec.h"
#include "main.h"


static int volatile isBackground = 2;
// 0 == NO 1 == YES 2 == Undefined
static int volatile isKillingChild = 2;
// 0 == NO 1 == YES 2 == Undefined

static int pidCountBackground;
static int pidLengthBackground;
static int *pidsBackground;
int mainpid;
int lastStatus = 0; //this is constant what returns wait(%status), when child do exit(0)

static void updateWorkingDirAndCleanup(void) {
    char *newdir = shmat(cdshmgid, NULL, 0);
//    fprintf(stderr, "new dir is '%s'\n", newdir);
    chdir(newdir);
    for (int i = 0; i < PATH_MAX; i++) {
        newdir[i] = '\0';
    }
    shmdt(newdir);
    return;
}

static void addPid(int pid) {
    if (pidCountBackground + 1 >= pidLengthBackground) {
        pidLengthBackground += PIDS_SIZE; // just add PIDS_SIZE more spaces for pidsBackground array
        int *newpids = realloc(pidsBackground, pidLengthBackground * sizeof(int));
        if (newpids == NULL) { //ERROR!!!!!
            fprintf(stderr, "Failed to realloc pidsBackground list. Error exiting...\n");
            pidsBackground[pidLengthBackground - 1] = pid;
            for (int i = 0; i < pidLengthBackground; i++) {
                if (pidsBackground[i] != 0) {
                    kill(pidsBackground[i], SIGTERM);
                    waitpid(pidsBackground[i], NULL, 0);
                }
            }
            exit(1);
        } else {
            pidsBackground = newpids;
            for (int i = pidLengthBackground - PIDS_SIZE; i < pidLengthBackground; i++) { // set 0 all new array spots
                pidsBackground[i] = 0;
            }
        }
    }
    int i = -1;
    while (pidsBackground[++i] != 0);
    fprintf(stderr, "[%d] %d\n", i, pid);
    fflush(stdout);
    pidsBackground[i] = pid;
    pidCountBackground++;
}

void checkZombieProcesses(void) {
    int alreadyWritten = 0;
    for (int i = 0; i < pidLengthBackground; i++) {
        if (pidsBackground[i] != 0) {
            int status;
            int temp = waitpid(pidsBackground[i], &status, WNOHANG);
            if (temp > 0) {
                if (alreadyWritten == 1) {
                    fprintf(stderr, "\n[%d] Done", i);
                } else {
                    fprintf(stderr, "[%d] Done", i);
                    alreadyWritten = 1;
                }
                lastStatus = status;
                pidsBackground[i] = 0;
                pidCountBackground--;
            }
        }
    }
    if (alreadyWritten) {
        fprintf(stderr, "\n");
    }
}

void mySignalHandler(int sig) {
    if (sig == SIGUSR1) {
//        fprintf(stderr, "Background!\n");
        isBackground = 1;
    } else if (sig == SIGUSR2) {
//        fprintf(stderr, "Not Background, should waitpid() right now!\n");
        isBackground = 0;
    } else if (sig == SIGINT) {
        if (isKillingChild == 2) {
            if (isKillingChild != 1) {
                if (pidCountBackground != 0) {
                    fprintf(stderr, "Killing all background processes\n");
                    int alreadyWritten = 0;
                    for (int i = 0; i < pidLengthBackground; i++) {
                        if (pidsBackground[i] != 0) {
                            kill(pidsBackground[i], SIGTERM);
                            waitpid(pidsBackground[i], NULL, 0);
                            if (alreadyWritten == 0) {
                                fprintf(stderr, "\n[%d] Killed\n", i);
                                alreadyWritten = 1;
                            } else {
                                fprintf(stderr, "[%d] Killed\n", i);
                            }
                        }
                    }
                }
                fprintf(stderr, "Exiting....\n");
                exit(0);
            }
        } else {
            isKillingChild = 1;
        }
    }
}

int main() {

    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    mainpid = getpid();

    signal(SIGUSR1, mySignalHandler);
    signal(SIGUSR2, mySignalHandler);
    signal(SIGINT, mySignalHandler);

    cdshmgid = shmget(123457, 4096 * sizeof(char), 0600| IPC_PRIVATE | IPC_CREAT);
    if (cdshmgid == -1) {
        fprintf(stderr, "Failed to create cd shared memory. Error exiting...\n");
        exit(1);
    }

    if (1) {
//        printf("%d\n", cdshmgid);
        char *newdir = shmat(cdshmgid, NULL, 0);
        for (int i = 0; i < PATH_MAX; i++) {
            newdir[i] = '\0';
        }
        shmdt(newdir);
    }

    pidLengthBackground = PIDS_SIZE; // just init value
    pidCountBackground = 0; // at start we have 0 child processes
    pidsBackground = calloc(pidLengthBackground, sizeof(int));

    if (pidsBackground == NULL) {
        fprintf(stderr, "Failed to init pidsBackground list. Error exiting...\n");
        exit(1);
    }

    char **mylist;
    while ((mylist = getNewList()) != NULL) {
//        printlist(mylist);
//        fprintf(stderr, "\n");
        cmd_inf *tree = build_tree(mylist);
//        print_tree(tree, 0);
//        fprintf(stderr, "\nProgram output:\n");

        isKillingChild = 0;
        int shellPid = execute_tree(tree);

        while (isBackground == 2 && isKillingChild != 1) {
            usleep(SLEEP_TIME);
        }
        if (isKillingChild == 1) {
            fprintf(stderr, "Killed current executing command\n");
            waitpid(shellPid, &lastStatus, 0);
        }
        if (isBackground == 1) {
            addPid(shellPid);
        } else {
            waitpid(shellPid, &lastStatus, 0);
        }
        isBackground = 2;
        isKillingChild = 2;

        updateWorkingDirAndCleanup();

        clear_tree(tree);
    }
    fprintf(stderr, "Exiting..\n");
    shmctl(cdshmgid, IPC_RMID, 0);
    return 0;
}
