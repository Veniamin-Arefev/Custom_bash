#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "functionsList.h"

static vertex V = Start;
static int inited = 0;

void startReading();

char **getNewList() {
    if (!inited) {
        initread();
        inited = 1;
    }
    if (V == Stop) {
        return NULL;
    }
    clearlist();
    while (getlist() == NULL && V != Stop) {
        checkZombieProcesses();
        V = Start;
        startReading();
    }
    if (getlist() != NULL && getlist()[0] != NULL && strcmp(getlist()[0], "exit") == 0) {
        V = Stop;
        return NULL;
    }
    return getlist();
}

void startReading() {
    printf("$> ");
    fflush(stdout);
    c = getsym();
    clearlist();
    null_list();
    while (V != Newline && V != Stop) {
        switch (V) {
            case Start:
                V = start();
                break;

            case Word:
                V = word();
                break;

            case Newline:
                fprintf(stderr, "LIST ERROR!!!"); // unreachable
                break;

            case Dollar:
                V = dollar();
                break;

            case Apostrophe:
                V = apostrophe();
                break;

            case Apostrophe2:
                V = apostrophe2();
                break;

            case Or:
                V = or();
                break;

            case Or2:
                V = or2();
                break;

            case Ampersand:
                V = ampersand();
                break;

            case Ampersand2:
                V = ampersand2();
                break;

            case Semicolon:
                V = semicolon();
                break;

            case Greater:
                V = greater();
                break;

            case Greater2:
                V = greater2();
                break;

            case Less:
                V = less();
                break;

            case Lbkt:
                V = lbkt();
                break;

            case Rbkt:
                V = rbkt();
                break;

            case Stop:
                fprintf(stderr, "LIST ERROR!!!"); // unreachable
        }
    }
}

vertex start() {
    if (c == ' ' || c == '\t') {
        c = getsym();
        return Start;
    } else if (c == EOF) {
        termlist();
        return Stop;
    } else if (c == '\n') {
        termlist();
        return Newline;
    } else if (specialsym(c)) {
        if (c == '$') {
            nullbuf();
            addsym();
            c = getsym();
            return Dollar;
        } else if (c == '|') {
            nullbuf();
            addsym();
            return Or;
        } else if (c == '&') {
            nullbuf();
            addsym();
            return Ampersand;
        } else if (c == ';') {
            nullbuf();
            addsym();
            return Semicolon;
        } else if (c == '>') {
            nullbuf();
            addsym();
            return Greater;
        } else if (c == '<') {
            nullbuf();
            addsym();
            return Less;
        } else if (c == '(') {
            nullbuf();
            addsym();
            return Lbkt;
        } else if (c == ')') {
            nullbuf();
            addsym();
            return Rbkt;
        }
    } else if (c == '\'') {
        nullbuf();
        c = getsym();
        return Apostrophe;
    } else if (c == '"') {
        nullbuf();
        c = getsym();
        return Apostrophe2;
    } else {
        nullbuf();
        return Word;
    }
    return Start;
}

vertex word() {
    if (c == '\\') {
        c = getsym();
        if (c == '\\') {
            addspecifiedsym('\\');
            return Word;
        } else if (c != '\n') {
            addspecifiedsym('\\');
            addsym();
        }
        c = getsym();
        return Word;
    } else if (!specialsym(c)) {
        addsym();
        c = getsym();
        return Word;
    } else {
        addword();
        return Start;
    }
}

vertex apostrophe() { // '
    if (c == '\'') {
        addword();
        c = getsym();
        return Start;
    } else if (c == '\\') {
        c = getsym();
        if (c != '\n') {
            addspecifiedsym('\\');
            addsym();
        }
        c = getsym();
        return Apostrophe2;
    } else if (c == EOF) {
        addword();
        termlist();
        clearlist();
        fprintf(stderr, "Wrong apostrophe syntax!\n");
        return Stop;
    } else {
        addsym();
        c = getsym();
        return Apostrophe;
    }
}

vertex apostrophe2() { // "
    if (c == '"') {
        addword();
        c = getsym();
        return Start;
    } else if (c == '\\') {
        c = getsym();
        if (c != '\n') {
            addspecifiedsym('\\');
            addsym();
        }
        c = getsym();
        return Apostrophe2;
    } else if (c == EOF) {
        addword();
        termlist();
        clearlist();
        fprintf(stderr, "Wrong apostrophe syntax!\n");
        return Stop;
    } else {
        addsym();
        c = getsym();
        return Apostrophe2;
    }
}

vertex newline() {
    return Start;
}

vertex dollar() {
    if (specialsym(c) || (c == '\'') || (c == '"') || (c == '\\')) {
        addword();
        return Start;
    } else {
        addsym();
        c = getsym();
        return Dollar;
    }
}

vertex or() {
    c = getsym();
    if (c == '|') {
        addsym();
        return Or2;
    } else {
        addword();
        return Start;
    }
}

vertex or2() {
    addword();
    c = getsym();
    return Start;
}

vertex ampersand() {
    c = getsym();
    if (c == '&') {
        addsym();
        return Ampersand2;
    } else {
        addword();
        return Start;
    }
}

vertex ampersand2() {
    addword();
    c = getsym();
    return Start;
}

vertex semicolon() {
    addword();
    c = getsym();
    return Start;
}

vertex greater() {
    c = getsym();
    if (c == '>') {
        addsym();
        return Greater2;
    } else {
        addword();
        return Start;
    }
}

vertex greater2() {
    addword();
    c = getsym();
    return Start;
}

vertex less() {
    addword();
    c = getsym();
    return Start;
}

vertex lbkt() {
    addword();
    c = getsym();
    return Start;
}

vertex rbkt() {
    addword();
    c = getsym();
    return Start;
}
