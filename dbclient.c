/*
        Written by Zachary Leach for CS 3377.004 - Project 3
        12.14.2022
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"

void getline2(char **dst);
int sockwrite(int sockfd, char *str);
int sockread(int sockfd, struct msg *mptr);
int sockwrite2(int sockfd, struct msg message);
void printmsg(struct msg m);


/*
        parse command line args
        create client socket
        get server host
        get server address
        connect to server address

        loop:
                read put, get, or quit 

                if put:
                        create message
                        write message to socket
                        read success/fail message from socket

                if get:
                        create message
                        write message to socket
                        read success/fail message from socket

        if any socket read/write operations fail:
                terminate program
*/
int main(int argc, char *argv[]) {


        // parsing the command line args
        if (argc != 3) {
                printf("Usage: ./dbclient <hostname> <port>\n");
                exit(1);
        }

        int portno = atoi(argv[2]);
        if (portno < 1024) {
                printf("Port must be greater than 1024.\n");
        }


        // create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
                printf("Create socket fail.\n");
                exit(0);
        }


        // get server host
        struct hostent *server = gethostbyname(argv[1]);
        if (server == NULL) {
                printf("Get host fail.\n");
                exit(0);
        }


        // get server address
        struct sockaddr_in saddr;
        memset(&saddr, 0, sizeof(struct sockaddr_in));
        saddr.sin_family = AF_INET;
        bcopy((char *) server->h_addr, (char *) &saddr.sin_addr.s_addr, server->h_length);
        // memcpy(&saddr.sin_addr.s_addr, server->h_addr, server->h_length);
        saddr.sin_port = htons(portno);


        // connect to server address
        int cres = connect(sockfd, (struct sockaddr *) &saddr, sizeof(saddr));
        if (cres == -1) {
                printf("Connect fail.");
                exit(0);
        }

        
        // infinite write to server
        while (69) {
                char *line;

                printf("Enter your choice (1 to put, 2 to get, 0 to quit): ");
                getline2(&line);

                int choice = atoi(line);
                if (choice == 0) {
                        printf("Closing client.\n");
                        return 0;
                }

                // put
                if (choice == 1) {
                        // get name
                        char *name;
                        printf("Enter the name: ");
                        getline2(&name);


                        // get id (string)
                        char *id;
                        printf("Enter the id: ");
                        getline2(&id);


                        // get id (number)
                        uint32_t idnum = atoi(id);
                        if (strcmp(id, "0") != 0 && idnum == 0) {
                                printf("Inavlid ID.\n");
                                continue;
                        }


                        // make record
                        struct record rec;
                        strncpy(rec.name, name, 128);
                        rec.id = idnum;


                        // make message
                        struct msg wmsg;
                        wmsg.type = PUT;
                        wmsg.rd = rec;


                        // write wmsg to socket
                        int wres = sockwrite2(sockfd, wmsg);
                        if (wres == -1) {
                                printf("Client socket write fail.\n");
                                exit(0);
                        }


                        // read message from socket
                        struct msg rmsg;
                        int rres = sockread(sockfd, &rmsg);
                        if (rres == -1) {
                                printf("Client socket read fail.\n");
                                exit(-1);
                        }

                        
                        // put success / fail notification
                        if (rmsg.type == SUCCESS) {
                                printf("Put success.\n");
                        }
                        else if (rmsg.type == FAIL) {
                                printf("Put failed.\n");
                        }


                        /*
                        printf("read message: ");
                        printmsg(rmsg);
                        */


                        free(name);
                        free(id);
                }

                
                // get
                if (choice == 2) {
                        // get id (string)
                        char *id;
                        printf("Enter the id: ");
                        getline2(&id);


                        // get id (number)
                        uint32_t idnum = atoi(id);
                        if (strcmp(id, "0") != 0 && idnum == 0) {
                                printf("Inavlid ID.\n");
                                continue;
                        }


                        // make record
                        struct record rec;
                        rec.id = idnum;


                        // make message
                        struct msg message;
                        message.type = GET;
                        message.rd = rec;


                        // write message to socket
                        sockwrite2(sockfd, message);


                        // read message from socket
                        struct msg rmsg;
                        int rres = sockread(sockfd, &rmsg);
                        if (rres == -1) {
                                printf("Client socket read fail.\n");
                                exit(0);
                        }


                        /*
                        printf("read message: ");
                        printmsg(rmsg);
                        */

                        
                        // get success / fail notification
                        if (rmsg.type == SUCCESS) {
                                printf("Name: %s\n", rmsg.rd.name);
                                printf("ID: %d\n", rmsg.rd.id);
                        }
                        else if (rmsg.type == FAIL) {
                                printf("Get failed.\n");
                        }



                        free(id);
                }


        }



}



void printmsg(struct msg m) {
        printf("type: %d name: %s id: %d\n", m.type, m.rd.name, m.rd.id);
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



/**
 * reads a line of input into dst.
 * does not include the trailing '\n' character like getline().
 * 
 * uses malloc and realloc internally (dst should be freed after use).
 */
void getline2(char **dst) {
        size_t size = 0;
        ssize_t n = getline(dst, &size, stdin);
        if ((*dst)[n - 1] == '\n') {
                (*dst)[n - 1] = '\0';
        }
}


