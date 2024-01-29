#include <pthread.h>
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

static void *doit(void *,void*);          /* each thread executes this function */
static void str_echo(void*,void*, char*, char*);
struct message {
	int addrlen, timelen, msglen;
	char addr[MAXLINE];
	char currtime[MAXLINE];
	char payload[MAXLINE];
};

int
main(int argc, char **argv)
{
    if (argc != 2) {
	 printf("usage: ./tunnel port\n");
	 exit(1);
    }
    int *clientSocket;
    int *serverSocket;
    serverSocket = malloc(sizeof(int));
    struct sockaddr_in serverAddr;

    // Create socket
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }


    int tunnelPort = atoi(argv[1]);
    // Set up server address structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(tunnelPort);

    // Bind the socket
    if (bind(*serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(*serverSocket, LISTENQ) == -1) {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Tunnel listening on port %d...\n", tunnelPort);
    for (; ;) {
        clientSocket = malloc(sizeof(int));
        *clientSocket = accept(*serverSocket, (struct sockaddr*)NULL, NULL);
        if (*clientSocket == -1) {
	    perror("Error accepting connection");
	    exit(EXIT_FAILURE);
        }
        struct sockaddr_in clientaddr;
        socklen_t clientaddrlen = sizeof(clientaddr);
        getpeername(*clientSocket, (struct sockaddr *)&clientaddr, &clientaddrlen);
        char *clientIP = inet_ntoa(clientaddr.sin_addr);
        printf("Client IP: %s\n", clientIP);
	

	doit(clientSocket, serverSocket);
    }    
}

static void *
doit(void *arg, void* server)
{
	int     connfd;
        connfd = *((int *) arg);
	int serverSocket;
	serverSocket = *((int *) server);
	char server_addr[MAXLINE];
     	char server_port[MAXLINE];
	
	int n;
	char recvline[MAXLINE + 1];
	while ( (n = read(connfd, recvline, MAXLINE)) > 0) {
	    recvline[n] = 0;
	    char* delimiter = strchr(recvline, ' '); 
	    if (delimiter != NULL) {
		printf("%s\n", recvline);
		*delimiter = '\0'; 
		strcpy(server_addr, recvline);
		strcpy(server_port, delimiter + 1); 
	    } else {
		fprintf(stderr, "Invalid data format from client\n");
	    }
	}
	if (n < 0) {
		printf("read error\n");
		exit(1);
	}
	int *clientSocket_write;
        clientSocket_write = malloc(sizeof(int));
        *clientSocket_write = accept(serverSocket, (struct sockaddr*)NULL, NULL);
        if (*clientSocket_write == -1) {
	    perror("Error accepting connection");
	    close(serverSocket);
	    exit(EXIT_FAILURE);
        }
	str_echo(server, clientSocket_write, server_addr, server_port);
     	return (NULL);
}

void
str_echo(void* tunnel, void *arg, char* hostName_or_ip, char* portNumberInput)
{
	int connfd;
        connfd = *((int *) arg);
	int tunnelSocket;
	tunnelSocket = *((int *) tunnel);
	int portNumber = atoi(portNumberInput);


	int sockfd, n;

	char recvline[MAXLINE + 1];
	struct sockaddr_in servaddr;
	// generate address
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("socket error\n");
		exit(1);
	}
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(portNumber); 
	struct addrinfo *result, *rp;
	if ( getaddrinfo(hostName_or_ip, portNumberInput, NULL, &result) != 0 ) {
		close(connfd);
		close(tunnelSocket);
		printf("getaddrinfo error\n");
		exit(1);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0){
			break; /* Success */
		}		
	}	
		
	if (rp == NULL) {
		printf("could not connect to remote host\n");
		close(connfd);
		close(tunnelSocket);
		exit(1);
	}

	// get server ip
	void *addr;
        char ipstr[INET6_ADDRSTRLEN];
        if (rp->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)rp->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)rp->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
	// Convert the IP to a string and print it
        if (inet_ntop(rp->ai_family, addr, ipstr, sizeof(ipstr)) == NULL) {
            perror("inet_ntop");
        }

        printf("Server IP: %s\n", ipstr);
	freeaddrinfo(result);
	
	// client does not need to send anything;
	// server should send back time and who output in the payload
	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0; /* null terminate */

	        write(connfd, recvline, strlen(recvline));
	
		if (fputs(recvline, stdout) == EOF) {
			printf("fputs error\n");
			close(connfd);
			exit(1);
		}
	}
	close(connfd);
	if (n < 0) {
		printf("read error\n");
		close(connfd);
		exit(1);
	}
}

