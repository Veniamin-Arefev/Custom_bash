#ifndef functionsList_h
#define functionsList_h

#define SIZE 16

int prevc; /*текущий символ */
int c; /*текущий символ */
char **lst; /* список слов (в виде массива)*/
char *buf; /* буфер для накопления текущего слова*/ 
int sizebuf; /* размер буфера текущего слова*/
int sizelist; /* размер списка слов*/
int curbuf; /* индекс текущего символа в буфере*/
int curlist; /* индекс текущего слова в списке*/
char *readbuf; /* буфер для накопления входных слов*/
int readcur; /* интекс в текущем буфере накопления*/

typedef enum {
    Start,
    Stop,
    Word,
    Newline,

    Apostrophe,
    Apostrophe2,

    Dollar,
    Or,
    Or2,
    Ampersand,
    Ampersand2,
    Semicolon,
    Greater,
    Greater2,
    Less,
    Lbkt,
    Rbkt
} vertex;


vertex start();
vertex word();
vertex newline();

vertex apostrophe();
vertex apostrophe2();

vertex dollar();
vertex or();
vertex or2();
vertex ampersand();
vertex ampersand2();
vertex semicolon();
vertex greater();
vertex greater2();
vertex less();
vertex lbkt();
vertex rbkt();


int getsym();
void initread();
char ** getlist();
void clearlist();
void null_list();
void termlist();
void nullbuf();
void addsym();
void addspecifiedsym(char sym);
void addword();
int mystrcmp();
void bubble_list_sort();
int specialsym(int c);

void printlist(char ** inlist);
void startReading();
char **getNewList();
#endif
