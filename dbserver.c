/*
        Written by Zachary Leach for CS 3377.004 - Project 3
        12.14.2022
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"

#include <pthread.h>
#define _GNU_SOURCE

char FILENAME[] = "database";

typedef struct {
        int clientfd;
} t_args;

void * t_handleClient(void *arg);
int sockread(int sockfd, struct msg *mptr);
void printmsg(struct msg m);
void handleClient(int);
int sockwrite(int, char *);
int sockwritenum(int sockfd, int num);
int PUTCOUNT;
int sockwrite2(int sockfd, struct msg message);
int putRecord(int fd, struct record *rptr);
int getRecord(int fd, int id, struct record *rptr);


int main(int argc, char* argv[]) {
        if (argc != 2) {
                printf("Usage: ./dbserver <port-number>\n");
                exit(1);
        }

        int portnum = atoi(argv[1]);
        if (portnum < 1024) {
                printf("Port must be greater than 1024.\n");
        }



        // set the global put index 
        int fd = open(FILENAME, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
        if (fd == -1) {
                printf("file open fail");
                return -1;
        }
        PUTCOUNT = lseek(fd, 0, SEEK_END) / sizeof(struct record);


        // create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
                printf("socket failed");
        }


        // create server address
        struct sockaddr_in saddr;
        memset(&saddr, 0, sizeof(struct sockaddr_in));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = INADDR_ANY;
        saddr.sin_port = htons(4000);


        // set server socket options
        int optval = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));


        // bind server socket
        int bindres = bind(sockfd, (struct sockaddr *) &saddr, sizeof(saddr));
        if (bindres == -1) {
                printf("bind fail\n");
                return 0;
        }


        // listen on server socket
        int lisres = listen(sockfd, SOMAXCONN);
        if (lisres == -1) {
                printf("listen fail\n");
                return 0;
        }


        // accept next client connection
        struct sockaddr_in caddr;
        memset(&caddr, 0, sizeof(struct sockaddr_in));
        socklen_t clen = sizeof(caddr);
        int clientfd = -1;
        while (69) {
                printf("accepting\n");
                clientfd = accept(sockfd, (struct sockaddr *) &caddr, &clen);
                if (clientfd == -1) {
                        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                                continue;
                        }
                        printf("accept fail");
                        return 0;
                }

                printf("client connected\n");

                t_args *arg = malloc(sizeof(t_args));
                arg->clientfd = clientfd;
                pthread_t tid;
                pthread_create(&tid, NULL, t_handleClient, arg);

                // handleClient(clientfd);
                // accept one connection
                // break;
        }
}


void * t_handleClient(void *arg) {
        t_args *t_arg = arg;
        int clientfd = t_arg->clientfd;

        // open file
        int fd = open(FILENAME, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
        if (fd == -1) {
                printf("file open fail");
                return (void *) -1;
        }

        
        // handle client message
        while (69) {

                // read client message
                struct msg rmsg;
                int rres = sockread(clientfd, &rmsg);
                if (rres == -1) {
                        printf("socket read fail\n");
                        return (void *) -1;
                }

                /*
                printf("read message: ");
                printmsg(rmsg);
                */


                // put 
                if (rmsg.type == PUT) {
                        // make record
                        struct record r = rmsg.rd;
                        

                        // put record
                        int pres = putRecord(fd, &r);


                        // create client msg
                        struct msg wmsg;
                        memset(&wmsg.rd, 0, sizeof(struct record));
                        wmsg.type = SUCCESS;
                        if (pres == -1) {
                                wmsg.type = FAIL;
                        }

                        
                        /*
                        printf("write message: ");
                        printmsg(wmsg);
                        */


                        // send client msg
                        int wres = sockwrite2(clientfd, wmsg);
                        if (wres == -1) {
                                printf("socket write fail\ns");
                                return (void *) -1;
                        }
                }


                // get
                else if (rmsg.type == GET) {
                        // get record
                        struct record r;
                        int res = getRecord(fd, rmsg.rd.id, &r);


                        // create message
                        struct msg wmsg;
                        wmsg.rd = r;
                        wmsg.type = SUCCESS;
                        if (res == -1) {
                                wmsg.type = FAIL;
                                memset(&wmsg.rd, 0, sizeof(struct record));
                        }

                        
                        /*
                        printf("write message: ");
                        printmsg(wmsg);
                        */


                        // write msg to client
                        int wres = sockwrite2(clientfd, wmsg);
                        if (wres == -1) {
                                printf("socket write fail\n");
                                return (void *) -1;
                        }
                }
        }

}

int writeRecord(int fd, int index, struct record *rptr) {
        lseek(fd, index * sizeof(struct record), SEEK_SET);
        return write(fd, rptr, sizeof(struct record));
}

int readRecord(int fd, int index, struct record *rptr) {
        lseek(fd, index * sizeof(struct record), SEEK_SET);
        return read(fd, rptr, sizeof(struct record));
}

int findRecord(int fd, int id) {
        for (int i = 0; i * sizeof(struct record) < lseek(fd, 0, SEEK_END); i++) {
                struct record r;
                readRecord(fd, i, &r);
                if (r.id == id) {
                        return i;
                }
        }

        return -1;
}

int putRecord(int fd, struct record *rptr) {
        int wres = writeRecord(fd, PUTCOUNT, rptr);
        if (wres == -1) {
                return FAIL;
        }

        return SUCCESS;
}

int getRecord(int fd, int id, struct record *rptr) {
        int index = findRecord(fd, id);

        if (index == -1) {
                return FAIL;
        }

        readRecord(fd, index, rptr);
        return SUCCESS;
}


void printmsg(struct msg m) {
        printf("type: %d name: %s id: %d\n", m.type, m.rd.name, m.rd.id);
}

int sockwrite2(int sockfd, struct msg message) {
        while (1) {
                int wres = write(sockfd, &message, sizeof(struct msg));
                if (wres == 0) {
                        printf("write fail 1\n");
                        return -1;
                }
                if (wres == -1) {
                        if (errno == EAGAIN || errno == EINTR) {
                                continue;
                        }
                        printf("write fail 2\n");
                        return -1;
                }

                break;
        }

        return 0;
}


int sockread(int sockfd, struct msg *mptr) {
        // read server message
        while (1) {
                int rres = read(sockfd, mptr, sizeof(struct msg));
                if (rres == 0) {
                        close(sockfd);
                        printf("read fail 1\n");
                        return -1;
                }
                if (rres == -1) {
                        if (errno == EINTR) {
                                continue;
                        }

                        close(sockfd);
                        printf("read fail 2\n");
                        return -1;
                }

                break;
        }

        return 0;
}


/*
        int length = snprintf(NULL, 0, "%u%u", curr, left);
        char *concat = malloc(length * sizeof(char));
        sprintf(concat, "%u%u", curr, left);
*/




































