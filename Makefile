\CFLAGS = -g -Wall
all: run
list.o: list.c functionsList.h
	gcc $(CFLAGS) list.c -c -o list.o
fun.o: fun.c functionsList.h
	gcc $(CFLAGS) fun.c -c -o fun.o
tree.o: tree.c tree.h
	gcc $(CFLAGS) tree.c -c -o tree.o
exec.o:	exec.c exec.h tree.h
	gcc $(CFLAGS) exec.c -c -o exec.o
shell: main.c list.o fun.o tree.o exec.o
	gcc $(CFLAGS) main.c list.o fun.o tree.o exec.o -o shell
shell_test1: main_test1.c list.o fun.o
	gcc $(CFLAGS) main_test1.c list.o fun.o -o shell_test1
shell_test2: main_test2.c list.o fun.o tree.o
	gcc $(CFLAGS) main_test2.c list.o fun.o tree.o -o shell_test2
shell_test3: main_test3.c list.o fun.o tree.o exec.o
	gcc $(CFLAGS) main_test3.c list.o fun.o tree.o exec.o -o shell_test3
clean:
	rm -f *.o shell test shell_test
run: shell
	rlwrap ./shell
runtest: shell_test3
	rlwrap ./shell_test
