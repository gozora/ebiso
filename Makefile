CC=gcc
#OPTS=-g3 -O3 -m64 -std=gnu9x -Wall -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
OPTS=-g3 -m64 -std=gnu9x -Wall -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
LLIBDIR=lib
INCLUDEDIR=./include
INCLUDE=-I${INCLUDEDIR}
MAIN_LIB=${LLIBDIR}/libmain.a
LIB=-L${LLIBDIR} -lmain -lm
INSTDIR=/usr/local/bin
PROGNAME=ebiso

DEPS=${LLIBDIR}/list.o ${LLIBDIR}/iso9660.o ${LLIBDIR}/write_files.o ${LLIBDIR}/el_torito.o ${LLIBDIR}/filename.o
HEADERS=${INCLUDEDIR}/list.h ${INCLUDEDIR}/iso9660.h ${INCLUDEDIR}/write_files.h ${INCLUDEDIR}/el_torito.h ${INCLUDEDIR}/globals.h ${INCLUDEDIR}/filename.h

all: ${MAIN_LIB} iso 

${LLIBDIR}/%.o: ${LLIBDIR}/%.c
	${CC} ${OPTS} ${INCLUDE} -c -o $@ $^

${MAIN_LIB}: ${DEPS} ${HEADERS}
	ar rcs $@ ${DEPS}

iso: ${DEPS} ${PROGNAME}.c ${INCLUDEDIR}/${PROGNAME}.h
	${CC} ${OPTS} ${INCLUDE} -o ${PROGNAME} ${PROGNAME}.c ${LIB}

.PHONY: clean
clean:
	rm -rf ${LLIBDIR}/*.o ${LLIBDIR}/*.a
	rm -rf ${PROGNAME}

.PHONY: oclean
oclean:
	rm -rf ${LLIBDIR}/*.o

.PHONY: install
install: ${PROGNAME}
	install -m 0755 ${PROGNAME} ${INSTDIR}

.PHONY: uninstall
uninstall:
	-rm ${INSTDIR}/${PROGNAME}
