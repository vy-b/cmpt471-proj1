#include <netinet/in.h>
#include <time.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#define MAXLINE     4096    /* max text line length */
#define LISTENQ     1024    /* 2nd argument to listen() */
#define DAYTIME_PORT 3333

struct message {
	int addrlen, timelen, msglen;
	char addr[MAXLINE];
	char currtime[MAXLINE];
	char payload[MAXLINE];
};

int
main(int argc, char **argv)
{
    int     listenfd, connfd;
    struct sockaddr_in servaddr;
    time_t ticks;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (argc != 2) {
    	perror("usage: ./server <myPortNumber> \n");
    	exit(1);
    }
    char* portNumberInput = argv[1];
    int portNumber = atoi(portNumberInput);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    servaddr.sin_port = htons(portNumber); 
    
    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

    listen(listenfd, LISTENQ);

    for ( ; ; ) {
        connfd = accept(listenfd, (struct sockaddr *) NULL, NULL);
	// Retrieve client IP address
	struct sockaddr_in clientaddr;
	socklen_t clientaddrlen = sizeof(clientaddr);
	getpeername(connfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
	char *clientIP = inet_ntoa(clientaddr.sin_addr);

	struct sockaddr_in sa;
	char host[MAXLINE];

	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = inet_addr(clientIP);

	// Get host name from IP address
	if (getnameinfo((struct sockaddr *)&sa, sizeof(struct sockaddr_in), host, MAXLINE, NULL, 0, 0) != 0) {
		fprintf(stderr, "getnameinfo: failed to resolve IP address\n");
		exit(EXIT_FAILURE);
	}

	printf("Client name: %s\n", host);
	printf("IP address: %s\n", clientIP);

	struct message buff;

        ticks = time(NULL);
	char* time_c = ctime(&ticks);
	char timebuff[MAXLINE];
        snprintf(timebuff, sizeof(timebuff), "%.24s", time_c);
	strcat(timebuff, "\n");
	strcpy(buff.currtime,timebuff);
	buff.timelen = strlen(timebuff);

	// get "who" output
	FILE *fp;
	char path[MAXLINE];

	// Run the "who" command and open a pipe to capture its output
	fp = popen("who", "r");
	if (fp == NULL) {
	    fprintf(stderr, "Error opening pipe.\n");
	    return EXIT_FAILURE;
	}

	// Read the output of the command into the 'path' variable
	if (fgets(path, MAXLINE, fp) == NULL) {
	    fprintf(stderr, "Error reading from pipe.\n");
	    pclose(fp);
	    exit(1); 
	}

	// Close the pipe
	pclose(fp);
	strcpy(buff.payload, "Who: ");
	strcat(buff.payload, path);	
	buff.msglen = strlen(buff.payload);

	// parse message buff field by field
	
	write(connfd, buff.currtime, buff.timelen);
	write(connfd, buff.payload, buff.msglen);

        close(connfd);
    }
}

