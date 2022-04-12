FLAGS = -Wall -g -pthread

all: operations_terminal stock_server clean

##############################################

operations_terminal: cliente.o
	gcc ${FLAGS} cliente.c -o $@

cliente.o: cliente.c

##############################################

stock_server: stock_server.o
	gcc ${FLAGS} stock_server.c -o $@

stock_server.o: stock_server.c

##############################################
clean:
	rm -f stock_cliente.o stock_server.o