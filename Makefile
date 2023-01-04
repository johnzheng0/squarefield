PROG = squarefield
SRC = ${PROG}.c
OBJ = ${SRC:.c=.o}

BINDIR = /usr/bin

CC = cc
INCS = -l/usr/include/X11
LIBS = -lX11

LDFLAGS = ${LIBS}
CFLAGS = -Wall -Wextra ${INCS}

${PROG}: ${OBJ}
	${CC} ${OBJ} -o ${PROG} ${LIBS}

${OBJ}: ${SRC}
	${CC} -c ${SRC}

install: ${PROG}
	cp ${PROG} ${BINDIR}/
	chmod 755 ${BINDIR}/${PROG}

uninstall:
	rm ${BINDIR}/${PROG}

clean:
	rm *.o ${PROG}

.PHONY: clean
