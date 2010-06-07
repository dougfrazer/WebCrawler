// ClientSocket.c

#include "ClientSocket.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>

ClientSocket::ClientSocket (const char *host, int port, int protocol, bool unknown) {
    struct hostent *hostEntity;
    sock.sin_family = AF_INET;
    sock.sin_port = htons(port);
    hostEntity = gethostbyname(host);
    if( hostEntity == NULL) {
        printf("Failed to look up hostname %s\n", host);
        exit(1);
    }
    memcpy(&sock.sin_addr.s_addr, hostEntity->h_addr, hostEntity->h_length);
}


void
ClientSocket::close (void) {
   shutdown(sockfd, SHUT_RDWR); 
}

void
ClientSocket::setTimeout (int value) {
    this->timeout = value;
}

bool
ClientSocket::conn (void) {
    const sockaddr *sockAddrCast = (const sockaddr *)&sock;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd < 0) {
       printf("ERROR opening socket\n");
       return false;
    }
    if (connect(sockfd,sockAddrCast,sizeof(sock)) < 0) { 
       printf("ERROR connecting\n");
       return false;
    }
    return true;
}

void
ClientSocket::write (std::string data) {
    int n;
    data += '\n';
    if (sockfd <= 0) 
       printf("ERROR socket not open yet\n");

    n = send(sockfd, data.c_str(), data.length(), 0);
    if (n < 0)
        printf("ERROR writing to the socket\n");
}

int
ClientSocket::read (char *buffer, int length) {
    int n;
    
    if (sockfd <= 0)
        printf("ERROR socket not open yet\n");

    n = recv(sockfd, buffer, length, 0);
    printf("%s", buffer);
    if (n < 0) 
         printf("ERROR reading from socket\n");

    return length;
}
