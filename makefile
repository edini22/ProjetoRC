FLAGS	= -Wall -g -pthread
CC	    = gcc

PROG1   = stock_server
OBJS1   = stock_server.o func.o

PROG2	= operations_terminal
OBJS2	= cliente.o

# $@ nome do target
# $< primeiro pre requesito (?)

all:	${PROG1} ${PROG2} clean

${PROG1}:	${OBJS1}
	${CC} ${FLAGS} ${OBJS1} -o $@ 

${PROG2}:	${OBJS2}
	${CC} ${FLAGS} ${OBJS2} -o $@ 

.PHONY : clean
clean:
	rm -f ${OBJS1} ${OBJS2}

# Server

func.o: func.h func.c

# Cliente

cliente.o: cliente.c 

