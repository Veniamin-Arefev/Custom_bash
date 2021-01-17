#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/shm.h>

#include "exec.h"

extern int cdshmgid; // variable from main.c

static int masterShellPid; //pid created in execute_tree

static int pidCount;
static int pidLength;
static int *pids;
int lastStatus; //this is constant what returns wait(%status), when child do exit(0)

static void execute_cmd_inf(cmd_inf *tree); //recursive executing
static void execute_psubcmd_cmd_inf(cmd_inf *cmdInf); // executing for sub commands
static void execute_pipe_cmd_inf(cmd_inf *cmdInf, int infd); //recursive executing for one line of pipes
static int is_background_pipe(cmd_inf *tree); //recursive to last pipe cmd and check background flag
static void deal_with_file_redirect(cmd_inf *tree); //recursive to last pipe cmd and check background flag

static volatile int backgroundPsubcmd = 2; //this is constant what returns wait(%status), when child do exit(0)
// 0 == NO 1 == YES 2 == Undefined

static void MyPsubcmdSigHandler(int sig) {
    if (backgroundPsubcmd != 1) {
        if (sig == SIGUSR1) {
            backgroundPsubcmd = 1;
        } else { //sig == SIGUSR2
            backgroundPsubcmd = 0;
        }
    }
}

static void dealWithEnvVariables(char *argv[]) { //substitute correct values
    int i = 1;
    while (argv[i] != NULL) {
        if (argv[i][0] == '$') { //this is environment variable
            if (argv[i][1] == '?' && argv[i][2] == '\0') {
                free(argv[i]);
                char *newarg = malloc(sizeof(char) * 30);
                if (newarg == NULL) {
                    fprintf(stderr, "Malloc error in environment variable handler\n");
                    exit(1);
                }
                if (WIFEXITED(lastStatus) != 0) {
                    sprintf(newarg, "%d", WEXITSTATUS(lastStatus));
                } else {
                    sprintf(newarg, "%d", -1);
                }
                argv[i] = newarg;
            } else {
                char *envVar = getenv(argv[i] + 1);
                free(argv[i]);
                char *newarg;
                if (envVar == NULL) { //get empty environment variable
                    newarg = malloc(sizeof(char) * 2);
                    if (newarg == NULL) {
                        fprintf(stderr, "Malloc error in environment variable handler\n");
                        exit(1);
                    }
                    newarg[0] = ' ';
                    newarg[1] = '\0';
                } else {
                    newarg = malloc(sizeof(char) * strlen(envVar));
                    if (newarg == NULL) {
                        fprintf(stderr, "Malloc error in environment variable handler\n");
                        exit(1);
                    }
                    strcpy(newarg, envVar);
                }
                argv[i] = newarg;
            }
        }
        i++;
    }
}

static void mycd(char *argv[]) { //execute cd command in executing process or in subcmd
    dealWithEnvVariables(argv);
    if (argv[2] != NULL) {
        fprintf(stderr, "cd:Too many arguments\n");
        exit(1);
    }
    char *dir;
    if (argv[1] == NULL) { // $HOME is our working dir
        dir = getenv("HOME");
    } else {
        dir = argv[1];
    }
//    fprintf(stderr, "%s\n", dir);
    if (chdir(dir) == 0) {
//        fprintf(stderr, "Success\n");
        if (masterShellPid == getpid()) {
            char *newdir = shmat(cdshmgid, NULL, 0);
            for (int i = 0; i < PATH_MAX; i++) {
                newdir[i] = dir[i];
                if (dir[i] == '\0') {
                    break;
                }
            }
            shmdt(newdir);
        }
    } else {
        fprintf(stderr, "cd:Error while execution chdir()\n");
        exit(1);
    }
}

static void myexecvp(const char *file, char *argv[]) { //only use in  forked processes
    dealWithEnvVariables(argv);
    execvp(file, argv);
    exit(1);
}

static void addPid(int pid) {
    if (pidCount + 1 >= pidLength) {
        pidLength += PIDS_SIZE; // just add PIDS_SIZE more spaces for pids array
        int *newpids = realloc(pids, pidLength * sizeof(int));
        if (newpids == NULL) { //ERROR!!!!!
            fprintf(stderr, "Failed to realloc pids list. Error exiting...\n");
            pids[pidLength - 1] = pid;
            for (int i = 0; i < pidLength; i++) {
                if (pids[i] != 0) {
                    kill(pids[i], SIGKILL);
                    waitpid(pids[i], NULL, 0);
                }
            }
            exit(1);
        } else {
            pids = newpids;
            for (int i = pidLength - PIDS_SIZE; i < pidLength; i++) { // set 0 all new array spots
                pids[i] = 0;
            }
        }
    }
    int i = -1;
    while (pids[++i] != 0);
    pids[i] = pid;
    pidCount++;
}

static void initPids(int initPidLength) {
    pidLength = initPidLength; // just init value
    pidCount = 0; // at start we have 0 child processes
    pids = calloc(pidLength, sizeof(int));
    if (pids == NULL) {
        fprintf(stderr, "Failed to init pids list. Error exiting...\n");
        exit(1);
    }
}

static void checkZombieProcesses(void) {
    for (int i = 0; i < pidLength; i++) {
        if (pids[i] != 0) {
            int status;
            int temp = waitpid(pids[i], &status, WNOHANG);
            if (temp > 0) {
                lastStatus = status;
                pids[i] = 0;
                pidCount--;
            }
        }
    }
}

static int is_background_pipe(cmd_inf *tree) {
    if (tree == NULL) {
        fprintf(stderr, "Error in checking pipe background working. Error exiting...\n");
        exit(1);
    }
    while (tree->pipe != NULL) {
        tree = tree->pipe;
    }
    return tree->backgrnd;
}

int execute_tree(cmd_inf *cmd_inf) {

    int shellpid;
    if ((shellpid = fork()) == 0) { //this is process for handling shell

        signal(SIGUSR1, MyPsubcmdSigHandler);
        signal(SIGUSR2, MyPsubcmdSigHandler);

        masterShellPid = getpid();

        initPids(PIDS_SIZE);
        execute_cmd_inf(cmd_inf);
        if (pidCount == 0) {
            kill(getppid(), SIGUSR2);
        } else {
            kill(getppid(), SIGUSR1);
        }
        while (pidCount != 0) {
            checkZombieProcesses();
            usleep(SLEEP_TIME);
        }
        if (WIFEXITED(lastStatus) == 0) {
            exit(1);
        } else {
            exit(WEXITSTATUS(lastStatus));
        }
    }
    return shellpid;
}

static void execute_cmd_inf(cmd_inf *cmdInf) {

    int pid;

    if (cmdInf->argv != NULL && cmdInf->argv[0] != NULL && (strcmp(cmdInf->argv[0], "cd") == 0)) {
        mycd(cmdInf->argv);
    } else {
        if ((pid = fork()) == 0) { //handle current tree spot(or create pipe handler process)
            free(pids);
            if (cmdInf->pipe != NULL) { // we create subshell process for all one line pipe handlers
                initPids(PIDS_SIZE);
                if (is_background_pipe(cmdInf) == 1) { // should read from /dev/null
                    int fd = open("/dev/null", O_RDONLY);
                    execute_pipe_cmd_inf(cmdInf, fd); // fd already closed
                } else {
                    execute_pipe_cmd_inf(cmdInf, 0);
                }
                //wait for all pids in pipe
                while (pidCount != 0) {
                    checkZombieProcesses();
                    usleep(SLEEP_TIME);
                }
                if (WIFEXITED(lastStatus) == 0) {
                    exit(1);
                } else {
                    exit(WEXITSTATUS(lastStatus));
                }
            } else {
                deal_with_file_redirect(cmdInf);
                if (cmdInf->argv == NULL) { // this is a sub shell
                    execute_psubcmd_cmd_inf(cmdInf);
                } else {
                    myexecvp((cmdInf->argv)[0], cmdInf->argv);
                    exit(1);
                }
            }
        } else {
            // continue main recursion circle
            // only if background work or pipe
            if (cmdInf->pipe != NULL) {
                if (is_background_pipe(cmdInf) == 0) {
                    waitpid(pid, &lastStatus, 0);
                } else {
                    addPid(pid);
                }
            } else {
                if (cmdInf->argv == NULL) { // wait for signal from the sub shell
                    while (backgroundPsubcmd == 2) {
                        usleep(SLEEP_TIME);
                    }
                    if (backgroundPsubcmd == 1) {
                        addPid(pid);
                    } else {
                        waitpid(pid, &lastStatus, 0);
                    }
                    backgroundPsubcmd = 2;
                } else {
                    if (cmdInf->backgrnd == 0) {
//                    fprintf(stderr, "PID FOR WAIT %d\n", pid);
                        waitpid(pid, &lastStatus, 0);
                    } else {
                        addPid(pid);
                    }
                }
            }
        }
    }

    if (cmdInf->next != NULL) {
        if (cmdInf->type == NEXT) {
            execute_cmd_inf(cmdInf->next);
        } else if (cmdInf->type == AND) {
            if ((lastStatus == 0) || (WIFEXITED(lastStatus) != 0 && WEXITSTATUS(lastStatus) == 0)) {
                execute_cmd_inf(cmdInf->next);
            }
        } else { //cmdInf->type == OR
            if ((lastStatus != 0) || (WIFEXITED(lastStatus) != 0 && WEXITSTATUS(lastStatus) != 0)) {
                execute_cmd_inf(cmdInf->next);
            }
        }
    }
}

static void execute_psubcmd_cmd_inf(cmd_inf *cmdInf) { //returns exit status of last ended command
    initPids(PIDS_SIZE);

    int sendSignal = 0;
    if (cmdInf->backgrnd == 1) {
        sendSignal = 1;
        kill(getppid(), SIGUSR1);
    }

    execute_cmd_inf(cmdInf->psubcmd);

    if (sendSignal == 0) {
        if (pidCount == 0) {
            kill(getppid(), SIGUSR2);
        } else {
            kill(getppid(), SIGUSR1);
        }
    }

    while (pidCount != 0) {
        checkZombieProcesses();
        usleep(SLEEP_TIME);
    }
    if (WIFEXITED(lastStatus) == 0) {
        exit(1);
    } else {
        exit(WEXITSTATUS(lastStatus));
    }
}

static void execute_pipe_cmd_inf(cmd_inf *cmdInf, int infd) { //returns exit status of last ended command

    int fd[2];
    int pid;

    pipe(fd);

    if ((pid = fork()) == 0) { //handle current pipe tree spot
        if (infd != 0) {
            dup2(infd, 0);
            close(infd);
        }
        if (cmdInf->pipe != NULL) {
            dup2(fd[1], 1);
        }
        close(fd[1]);
        close(fd[0]);
        deal_with_file_redirect(cmdInf);

        if (cmdInf->argv == NULL) { // this is a sub shell
            execute_psubcmd_cmd_inf(cmdInf);
        } else {
            myexecvp((cmdInf->argv)[0], cmdInf->argv);
            exit(1);
        }
    } else {
        if (infd != 0) {
            close(infd);
        }
        close(fd[1]);
        addPid(pid);
        if (cmdInf->pipe != NULL) {
            execute_pipe_cmd_inf(cmdInf->pipe, fd[0]); //fd[0] is closed inside
        } else {
            close(fd[0]);
        }
    }
}

static void deal_with_file_redirect(cmd_inf *tree) {
    if (tree->infile != NULL) {
        int fd = open(tree->infile, O_RDONLY);
        if (fd != -1) {
            dup2(fd, 0);
            close(fd);
        } else {
            fprintf(stderr, "%s, No such file\n", tree->infile);
            exit(1);
        }
    }
    if (tree->outfile != NULL) {
        int fd;
        if (tree->append == 0) {
            fd = open(tree->outfile, O_WRONLY | O_CREAT);
        } else {
            fd = open(tree->outfile, O_WRONLY | O_CREAT | O_APPEND);
        }
        if (fd != -1) {
            dup2(fd, 1);
            close(fd);
        } else {
            fprintf(stderr, "%s, Cant create output file\n", tree->outfile);
            exit(1);
        }

    }
}
