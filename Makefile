PROG = main
SRC = ${PROG}.c
OBJ = ${SRC:.c=.o}

CC = cc
INCS = -l/usr/include/X11
LIBS = -lX11

LDFLAGS = ${LIBS}
CFLAGS = -Wall -Wextra ${INCS}

${PROG}: ${OBJ}
	${CC} ${OBJ} -o ${PROG} ${LIBS}

${OBJ}: ${SRC}
	${CC} -c ${SRC}

clean:
	rm *.o ${PROG}

.PHONY: clean