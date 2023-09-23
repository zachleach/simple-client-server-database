all: dbclient dbserver

dbclient: dbclient.o
	gcc -o dbclient dbclient.o

dbserver: dbserver.o -lpthread
	gcc -lpthread -o dbserver dbserver.o

dbclient.o: dbclient.c msg.h 
	gcc -std=gnu99 -Wall -Werror -c dbclient.c 

dbserver.o: dbserver.c msg.h 
	gcc -lpthread -Wall -Werror -std=gnu99 -c dbserver.c

clean: 
	rm dbserver dbclient dbclient.o dbserver.o
