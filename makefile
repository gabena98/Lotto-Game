all: client_lotto server_lotto

client_lotto:client_lotto.o
	     gcc -Wall client_lotto.o -o client_lotto

server_lotto:server_lotto.o
	     gcc -Wall server_lotto.o -o server_lotto -lpthread -lrt

client_lotto.o:client_lotto.c
	     gcc -c -Wall client_lotto.c

server_lotto.o:server_lotto.c
	     gcc -c -Wall server_lotto.c -lpthread -lrt
clean:
	     rm *.o client_lotto server_lotto
