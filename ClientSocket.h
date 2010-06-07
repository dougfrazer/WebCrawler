// clientsocket.h

#ifndef ClientSocket_class
#define ClientSocket_class

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <stdlib.h>
#include <netdb.h> 

class ClientSocket 
{
    public:
        ClientSocket (const char *host, int port, int protocol, bool unknown );
        void close (void);
        void write (std::string data);
        void setTimeout (int timeout);  
        int read (char *buffer, int length);
        bool conn ();
    private:
        int sockfd;
        struct sockaddr_in sock;
        int timeout;
};

#endif
