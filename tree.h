#ifndef tree_h
#define tree_h

enum type_of_next{
    NEXT, AND, OR
};

typedef struct cmd_inf {
    char **argv;
    char *infile;
    char *outfile;
    int append;     // == 1 then >>
    int backgrnd;
    struct cmd_inf* psubcmd; // subshell
    struct cmd_inf* pipe;
    struct cmd_inf* next; // Next cmd after ;  &  &&  ||
    enum type_of_next type; // Type of cmd after ;  &  &&  ||
} cmd_inf;

cmd_inf *build_tree(char **list);
void print_tree(cmd_inf *tree, int shift);
void clear_tree(cmd_inf *tree);

int isFileRedirect(char *arg);
int isEndOfCommand(char *arg);
int isBracket(char *arg);
#endif
