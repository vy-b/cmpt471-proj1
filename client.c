#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#define MAXLINE 4096
#define DAYTIME_PORT 3333

struct message {
	int addrlen, timelen, msglen;
	char addr[MAXLINE];
	char currtime[MAXLINE];
	char payload[MAXLINE];
};

void handleRequest(char* tunnel_name_or_ip, char* tunnel_port, char* server_name_or_ip, char* server_port) {
	char* sendToHost = server_name_or_ip;
	if (tunnel_name_or_ip != NULL) {
		sendToHost = tunnel_name_or_ip;
	}

	char* sendToPort = server_port;
	if (tunnel_port != NULL) {
		sendToPort = tunnel_port;
	}

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
	servaddr.sin_port = htons(atoi(sendToPort)); 
	

	if (tunnel_name_or_ip != NULL) {
		int write_sockfd;
		struct sockaddr_in servaddr;

		// generate address
		if ( (write_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("socket error\n");
			exit(1);
		}
		bzero(&servaddr, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(atoi(sendToPort)); 
		

		// send server ip and port to tunnel
                struct addrinfo *result;
                if (getaddrinfo(sendToHost, sendToPort, NULL, &result) != 0) {
			printf("getaddrinfo error\n");
			exit(1);
		}

		    // Loop through the results and print the IP addresses
		    void *addr;

		    char ipstr[INET6_ADDRSTRLEN];
		    if (result->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)result->ai_addr;
			addr = &(ipv4->sin_addr);
		    } else { // IPv6
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)result->ai_addr;
			addr = &(ipv6->sin6_addr);
		    }

		    // Convert the IP to a string and print it
		    if (inet_ntop(result->ai_family, addr, ipstr, sizeof(ipstr)) == NULL) {
			perror("inet_ntop");
		    }

		    freeaddrinfo(result);

		if (inet_pton(AF_INET, ipstr, &servaddr.sin_addr) <= 0) {
			printf("inet_pton error for %s\n", sendToHost);
			exit(1); 
		}
		if (connect(write_sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
			perror("Error connecting to server");
			exit(EXIT_FAILURE);
		}
		char buffer[MAXLINE];
	        char servport[MAXLINE];	
		// Send a message to the server
		strcpy(buffer, server_name_or_ip);
		strcat(buffer, " ");
		strcpy(servport, server_port);
		strcat(buffer, servport);
		
		write(write_sockfd, buffer, strlen(buffer));
		// write(write_sockfd, servport, strlen(servport)); 
		close(write_sockfd);
	}

	struct addrinfo *serv_result;
	if (getaddrinfo(server_name_or_ip, server_port, NULL, &serv_result) != 0) {
		printf("getaddrinfo error\n");
		exit(1);
	}

	// Loop through the serv_results and print the IP addresses
	void *serveraddr;

	char serverIpStr[INET6_ADDRSTRLEN];
	if (serv_result->ai_family == AF_INET) { // IPv4
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)serv_result->ai_addr;
		serveraddr = &(ipv4->sin_addr);
	} else { // IPv6
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)serv_result->ai_addr;
		serveraddr = &(ipv6->sin6_addr);
	}

	// Convert the IP to a string and print it
	if (inet_ntop(serv_result->ai_family, serveraddr, serverIpStr, sizeof(serverIpStr)) == NULL) {
		perror("inet_ntop");
	}

	freeaddrinfo(serv_result);

	struct sockaddr_in serv_sa;
	char serverName[MAXLINE];

	memset(&serv_sa, 0, sizeof(struct sockaddr_in));
	serv_sa.sin_family = AF_INET;
	serv_sa.sin_addr.s_addr = inet_addr(serverIpStr);

	// get serverName name from ip address
	if (getnameinfo((struct sockaddr *)&serv_sa, sizeof(struct sockaddr_in), serverName, MAXLINE, NULL, 0, 0) != 0) {
		printf("getnameinfo: failed to resolve ip address\n");
		exit(1);
	}


	printf("Server name: %s\n", serverName);
	printf("IP address: %s\n", serverIpStr);
	printf("Time: ");
	struct addrinfo *result, *rp;
	if ( getaddrinfo(sendToHost, sendToPort, NULL, &result) != 0 ) {
		printf("getaddrinfo error\n");
		exit(1);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0){
			break; 
		}		
	}	
	if (rp == NULL) {
		printf("could not connect to remote host\n");
		exit(1);
	}
	
	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0; 

		if (fputs(recvline, stdout) == EOF) {
			printf("fputs error\n");
			exit(1);
		}
	}
	if (n < 0) {
		printf("read error\n");
		exit(1);
	}
	close(sockfd);
	if (tunnel_name_or_ip != NULL) {
		struct addrinfo *tunnel_result;
		if (getaddrinfo(tunnel_name_or_ip, tunnel_port, NULL, &tunnel_result) != 0) {
			printf("getaddrinfo error\n");
			exit(1);
		}

	    // Loop through the tunnel_results and print the IP addresses
	    void *tunneladdr;

	    char tunnelIpStr[INET6_ADDRSTRLEN];
	    if (tunnel_result->ai_family == AF_INET) { // IPv4
		struct sockaddr_in *ipv4 = (struct sockaddr_in *)tunnel_result->ai_addr;
		tunneladdr = &(ipv4->sin_addr);
	    } else { // IPv6
		struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)tunnel_result->ai_addr;
		tunneladdr = &(ipv6->sin6_addr);
	    }

	    // Convert the IP to a string and print it
	    if (inet_ntop(tunnel_result->ai_family, tunneladdr, tunnelIpStr, sizeof(tunnelIpStr)) == NULL) {
		perror("inet_ntop");
	    }

	    freeaddrinfo(tunnel_result);

	struct sockaddr_in tunn_sa;
	char tunnelName[MAXLINE];

	memset(&tunn_sa, 0, sizeof(struct sockaddr_in));
	tunn_sa.sin_family = AF_INET;
	tunn_sa.sin_addr.s_addr = inet_addr(tunnelIpStr);

	// get tunnelName name from ip address
	if (getnameinfo((struct sockaddr *)&tunn_sa, sizeof(struct sockaddr_in), tunnelName, MAXLINE, NULL, 0, 0) != 0) {
		printf("getnameinfo: failed to resolve ip address\n");
		exit(1);
	}



		printf("Via tunnel: %s\n", tunnelName);
		printf("IP address: %s\n", tunnelIpStr);
		printf("Port number: %s\n", tunnel_port);
	}

	
	exit(0);

}


int main(int argc, char **argv)
{
	// checks if there are 4 arguments when the program is initialized in terminal
	if (argc == 3) {
	    handleRequest(NULL, NULL, argv[1], argv[2]);
	} else if (argc == 5) {
	    handleRequest(argv[1], argv[2], argv[3], argv[4]);
	} else {
	    printf("usage: ./client [tunnel_host] [tunnel_port] server_host server_port\n");
	    exit(1);
	}

	 
}
