#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "functionsList.h"

#define SIZE 16


/* Функция clearlist() освобождает память, занимаемую списком 
(если он не пуст), и делает список пустым. Переменную sizelist 
(размер списка) обнуляет, переменную curlist, указывающую очередную 
свободную позицию в списке, тоже обнуляет. */
void clearlist() { 
	int i;
    sizelist = 0;
	curlist = 0;
	if (lst == NULL) 
		return;
	for (i = 0; lst[i] != NULL; i++)
        free(lst[i]);
    free(lst);
	lst = NULL; 
}

void initread() {
    readbuf = malloc(SIZE+1);
    memset(readbuf, '\0', SIZE+1);
}

int getsym() {
    prevc = c;
    if (readbuf[readcur] == '\0') {
        memset(readbuf, '\0', SIZE+1);
        readcur = 0;
        if (fgets(readbuf, SIZE, stdin) == NULL) {
            free(readbuf);
            return EOF;
        }
    }
    return readbuf[readcur++];
}

void null_list() {
	sizelist = 0;
	curlist = 0;
	lst = NULL; 
}

/* Функция termlist() завершает список, добавляя NULL в позицию curlist и обрезает память,
 занимаемую списком, до точного размера. */
void termlist() {
	if (lst == NULL) 
		return; 
	if (curlist > sizelist-1)
    	lst = realloc(lst,(sizelist+1)*sizeof(*lst));
    if (lst == NULL) {
    	fprintf(stderr, "ERROR");
    	clearlist();
    	exit(0);
    }
	lst[curlist] = NULL;
	/*выравниваем используемую под список память точно по размеру списка*/ 
	lst = realloc(lst,(sizelist=curlist+1)*sizeof(*lst));
	if (lst == NULL) {
    	fprintf(stderr, "ERROR");
    	clearlist();
    	exit(0);
    }
}

/* Функция nullbuf() присваивает переменной buf значение NULL,
 переменной sizebuf (размер буфера) присваивает значение 0, 
 переменной curbuf, указывающей очередную свободную позицию в буфере, 
 присваивает значение 0.*/
void nullbuf() { 
	buf = NULL;
	sizebuf = 0;
	curbuf = 0; 
}

/* Функция addsym()добавляет очередной символ в буфер в позицию curbuf , 
после чего переменная curbuf увеличивается на 1. Если буфер был пуст, 
то он создается. Если размер буфера превышен, то он увеличивается на константу SIZE,
 заданную директивой define. */
void addsym() {
	if (curbuf > sizebuf-1)
		buf = realloc(buf, sizebuf+=SIZE); /* увеличиваем буфер при необходимости */
	if (buf == NULL) {
		fprintf(stderr, "ERROR");
		clearlist();
		exit(0);
	}
	buf[curbuf++] = c;
}

void addspecifiedsym(char sym) {
    if (curbuf > sizebuf-1)
        buf = realloc(buf, sizebuf+=SIZE); /* увеличиваем буфер при необходимости */
    if (buf == NULL) {
        fprintf(stderr, "ERROR");
        clearlist();
        exit(0);
    }
    buf[curbuf++] = sym;
}

/* Функция addword() завершает текущее слово в буфере, добавляя ’\0’ в позицию curbuf 
(увеличив, если нужно, буфер), и обрезает память, занимаемую словом, до точного размера;
затем добавляет слово в список в позицию curlist, после чего значение curlist увеличивается на 1. 
Если список был пуст, то он создается. Если размер списка превышен, то он увеличивается на константу SIZE. */
void addword() {
	if (curbuf > sizebuf-1)
	buf = realloc(buf, sizebuf+=1); /* для записи ’\0’ увеличиваем буфер при необходимости */
	if (buf == NULL) {
		fprintf(stderr, "ERROR");
		clearlist();
		exit(0);
	}
	buf[curbuf++] = '\0';
	/*выравниваем используемую память точно по размеру слова*/ 
    buf = realloc(buf,sizebuf=curbuf);
    if (buf == NULL) {
		fprintf(stderr, "ERROR");
		clearlist();
		exit(0);
	}
	if (curlist > sizelist-1)
		lst = realloc(lst, (sizelist+=SIZE)*sizeof(*lst)); /* увеличиваем массив под список при необходимости */
	if (buf == NULL) {
		fprintf(stderr, "ERROR");
		clearlist();
		exit(0);
	}
	lst[curlist++] = buf; 
}



int mystrcmp ( const char *s1, const char *s2 ) {
    for( ; *s1 == *s2; ++s1, ++s2 )
        if ( *s2 == '\0' )
            return 0;
    return *s1 - *s2;
}

void printlist(char ** inlist) {
    int i = 0;
    if (inlist == NULL) {
        fprintf(stderr, "Empty list:\n");
        return;
    }
    fprintf(stderr, "Our list:\n");
    while (inlist[i] != NULL) {
        fprintf(stderr, "%s\n", inlist[i++]);
    }
    fprintf(stderr, "End of list\n");
}

char ** getlist() {
    return lst;
}

/* Обозначение specialsym означает любой специальный символ. */
int specialsym(int c1) {
	return c == ' ' || c == '\n' || c == '\t' ||c == EOF ||c == '$' || c == '|' || c == '&' || c == ';' || c == '>' || c == '<' || c == '(' || c == ')';
}