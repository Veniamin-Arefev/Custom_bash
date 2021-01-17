#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "tree.h"

static int error_flag = 0;// текущий символ
static int bracket_returning = 0;// internal flag used for brackets sequences
static int cur_word_count = 0;// индекс текущего слова
static int brackets_count = 0; // счетчик для контроля кол-ва скобок
// ( add one and ) remove one from counter

static cmd_inf *init_cmd_inf(void);

static void add_arg(cmd_inf *cur_cmd_inf, char *arg);

static cmd_inf *build_cmd_inf(char **list);

cmd_inf *build_tree(char **list) {
    cur_word_count = 0;
    error_flag = 0;
    cmd_inf *ret_cmd_int = build_cmd_inf(list);
    if (error_flag != 0) {
        clear_tree(ret_cmd_int);
        return NULL;
    } else {
        return ret_cmd_int;
    }
}

cmd_inf *build_cmd_inf(char **list) {
    cmd_inf *cur_cmd;

    cur_cmd = init_cmd_inf();

    if (isEndOfCommand(list[cur_word_count]) ||
        isFileRedirect(list[cur_word_count])) {
        printf("Wrong command: first arg should be executable\n");
        return NULL;
    }

    while (list[cur_word_count] != NULL) {
        if (isFileRedirect(list[cur_word_count])) {
            if (list[cur_word_count + 1] == NULL) {
                error_flag = 1;
                printf("Redirect file doesnt specified\n");
                break;
            } else {
                if (strcmp(list[cur_word_count], ">") == 0) {
                    cur_cmd->outfile = list[cur_word_count + 1];
                    cur_cmd->append = 0;
                } else if (strcmp(list[cur_word_count], ">>") == 0) {
                    cur_cmd->outfile = list[cur_word_count + 1];
                    cur_cmd->append = 1;
                } else { // strcmp(list[cur_word_count], "<") == 0
                    cur_cmd->infile = list[cur_word_count + 1];
                }
                cur_word_count += 2;
            }
        } else if (isBracket(list[cur_word_count])) {
            if (strcmp(list[cur_word_count], "(") == 0) {
                if (cur_cmd->argv != NULL) {
                error_flag = 1;
                    printf("Incorrect bracket syntax, subcommand shouldn't have args\n");
                break;
                }
                brackets_count += 1;
                cur_word_count++;
                cur_cmd->psubcmd = build_cmd_inf(list);
                if (bracket_returning > 0) {
                    bracket_returning--;
                }
            } else { // strcmp(list[cur_word_count], ")") == 0
                brackets_count -= 1;
                cur_word_count++;
                bracket_returning = 1;
                if (brackets_count < 0) {
                    if (error_flag != 1) {
                        error_flag = 1;
                        printf("Incorrect bracket sequence\n");
                    }
                    break;
                }
                return cur_cmd;
            }
        } else if (isEndOfCommand(list[cur_word_count])) {
            if ((strcmp(list[cur_word_count], "&") == 0) || (strcmp(list[cur_word_count], ";") == 0)) {
                if (strcmp(list[cur_word_count], "&") == 0) {
                    cur_cmd->backgrnd = 1;
                }
                cur_word_count++;
                if (list[cur_word_count] != NULL && (strcmp(list[cur_word_count], ")") != 0)) {
                    cur_cmd->next = build_cmd_inf(list);
                }
            } else { // | or || or &&
                cur_word_count++;
                if (list[cur_word_count] == NULL) {
                    printf("After |, && or && must be next command to run\n");
                    error_flag = 1;
                    return cur_cmd;
                }
                if (strcmp(list[cur_word_count-1], "|") == 0) {
                    cur_cmd->pipe = build_cmd_inf(list);
                } else if (strcmp(list[cur_word_count-1], "&&") == 0) {
                    cur_cmd->type = AND;
                    cur_cmd->next = build_cmd_inf(list);
                } else { // strcmp(list[cur_word_count-1], "||") == 0
                    cur_cmd->type = OR;
                    cur_cmd->next = build_cmd_inf(list);
                }
            }
            if (bracket_returning > 0) {
                return cur_cmd;
            }
        } else {
            add_arg(cur_cmd, list[cur_word_count++]);
        }


        if (error_flag == 1) { // somewhere is error
            break;
        }
    }
    if (brackets_count != 0) {
        if (error_flag != 1) {
            error_flag = 1;
            printf("Incorrect bracket sequence\n");
        }
    }
    return cur_cmd;

}

static cmd_inf *init_cmd_inf() {
    cmd_inf *ret_cmd = NULL;
    ret_cmd = calloc(1, sizeof(cmd_inf));
/*    ret_cmd->argv = NULL;
    ret_cmd->infile = NULL;
    ret_cmd->outfile = NULL;
    ret_cmd->append = 0;
    ret_cmd->backgrnd = 0;
    ret_cmd->psubcmd = NULL;
    ret_cmd->pipe = NULL;
    ret_cmd->next = NULL;*/
    return ret_cmd;
}

/*-------------------------------------------- +ЭЛЕМЕНТ В ДЕР---------------------------------------*/

void add_arg(cmd_inf *cur_cmd_inf, char *arg) {
    int i = 0;
    if (cur_cmd_inf->argv == NULL) {
        cur_cmd_inf->argv = calloc(1, sizeof(*cur_cmd_inf->argv));
    }
    while (cur_cmd_inf->argv[i] != NULL) {
        i++;
    }
    cur_cmd_inf->argv = realloc(cur_cmd_inf->argv, ((i + 2) * sizeof(*cur_cmd_inf->argv)));
    cur_cmd_inf->argv[i] = arg;
    cur_cmd_inf->argv[i + 1] = NULL;
}

/*----------------------------------------ФУНКЦИЯ ПЕЧАТИ ДЕРЕВА----------------------------------------------------*/

void make_shift(int n) {
    while (n--)
        putc(' ', stderr);
}

void print_argv(char **p, int shift) {
    char **q = p;
    if (p != NULL) {
        while (*p != NULL) {
            make_shift(shift);
            fprintf(stderr, "argv[%d]=%s\n", (int) (p - q), *p);
            p++;
        }
    }
}

void print_tree(cmd_inf *cur_cmd_inf, int shift) {
    if (cur_cmd_inf == NULL) {
        printf("Empty tree\n");
        return;
    }
    if (cur_cmd_inf->argv != NULL) {
        print_argv(cur_cmd_inf->argv, shift);
    } else {
        make_shift(shift);
        fprintf(stderr, "This is sub shell\n");
    }
    make_shift(shift);
    fprintf(stderr, "infile = %s\n", cur_cmd_inf->infile == NULL ? "NULL" : cur_cmd_inf->infile);
    make_shift(shift);
    fprintf(stderr, "outfile = %s\n", cur_cmd_inf->outfile == NULL ? "NULL" : cur_cmd_inf->outfile);
    make_shift(shift);
    fprintf(stderr, "append = %d\n", cur_cmd_inf->append);
    make_shift(shift);
    fprintf(stderr, "background = %d\n", cur_cmd_inf->backgrnd);
    make_shift(shift);
    fprintf(stderr, "type=%s\n", cur_cmd_inf->type == NEXT ? "NEXT" : cur_cmd_inf->type == OR ? "OR" : "AND");
    make_shift(shift);
    if (cur_cmd_inf->psubcmd == NULL) {
        fprintf(stderr, "psubcmd=NULL \n");
    } else {
        fprintf(stderr, "psubcmd---> \n");
        print_tree(cur_cmd_inf->psubcmd, shift + 5);
    }
    make_shift(shift);
    if (cur_cmd_inf->pipe == NULL) {
        fprintf(stderr, "pipe=NULL \n");
    } else {
        fprintf(stderr, "pipe---> \n");
        print_tree(cur_cmd_inf->pipe, shift + 5);
    }
    make_shift(shift);
    if (cur_cmd_inf->next == NULL) {
        fprintf(stderr, "next=NULL \n");
    } else {
        fprintf(stderr, "next---> \n");
        print_tree(cur_cmd_inf->next, shift + 5);
    }
}


/*----------------------------------------------УДАЛЕНИЕ && ОЧИЩЕНИЕ---------------------------------------*/

void clear_tree(cmd_inf *T) {
    if (T == NULL) {
        return;
    }
    clear_tree(T->psubcmd);
    clear_tree(T->pipe);
    clear_tree(T->next);
    free(T);
}

int isFileRedirect(char *arg) {
    return (strcmp(arg, ">") == 0) || (strcmp(arg, ">>") == 0) || (strcmp(arg, "<") == 0);
}

int isEndOfCommand(char *arg) {
    return (strcmp(arg, "&") == 0) || (strcmp(arg, "&&") == 0) || (strcmp(arg, ";") == 0) ||
           (strcmp(arg, "|") == 0) || (strcmp(arg, "||") == 0);
}

int isBracket(char *arg) {
    return (strcmp(arg, "(") == 0) || (strcmp(arg, ")") == 0);
}