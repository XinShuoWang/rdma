CC=gcc
CXX_FLAGS=-g -O2
C_FLAGS=-g -O2
LD_LIBRARY=-libverbs
SRC=service.c

service: service.c
	$(CC) $(SRC) -o service $(C_FLAGS) $(LD_LIBRARY)
clean:
	rm -rf ./*.o
	rm -rf ./service