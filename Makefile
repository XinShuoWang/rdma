CC=gcc
C_FLAGS=-Wall
LD_LIBRARY=-lrdmacm -libverbs

client:client.c
	$(CC) $(C_FLAGS) client.c $(LD_LIBRARY)

server:server.c
	$(CC) $(C_FLAGS) server.c $(LD_LIBRARY)