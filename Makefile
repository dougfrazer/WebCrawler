# Makefile for the socket programming example
#
 socklibs = -lnsl -lsocket
 client_objects = ClientSocket.o
# simple_server_objects = ServerSocket.o Socket.o simple_server_main.o
 simple_client_objects = ClientSocket.o Socket.o simple_client_main.o


 all : client spider 

 client:
   g++ -o clientsocket $(socklibs) $(client_objects)


 spider: 
     g++ -o spider $(socklibs) $(spider_objects)


 spider: spider.cc
 ClientSocket: ClientSocket.cc


 clean:
     rm -f *.o simple_server simple_client
