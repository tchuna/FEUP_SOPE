all: client server

server: server.c 
	gcc server.c -o server -DREENTRANT -lpthread -Wall
client: client.c
	gcc client.c -o client -DREENTRANT -lpthread -Wall
clean:
	rm -f *.o server client
