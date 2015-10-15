# add .h files to dependencies: http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#combine
# FLAGS=-g3 -O3 -m64 -std=gnu9x -Wall -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
# $(info $$OBJ is [${OBJ}])

CC=gcc
LIBDIR=lib
INCLUDEDIR=./include
INSTDIR=/usr/local/bin
PROGNAME=ebiso

BASE_DEPDIR=.d
DEPDIR=.d/${LIBDIR}
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td
POSTCOMPILE=mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d

LIBS=-L${LIBDIR} -lmain -lm
MAIN_LIB=${LIBDIR}/libmain.a

FLAGS=-g3 -m64 -std=gnu9x -Wall -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -I${INCLUDEDIR}
VERSION=$(shell grep "\#define VERSION" ${INCLUDEDIR}/${PROGNAME}.h | awk '{ print $$NF }' | sed s/\"//g)

SRC=$(wildcard ${LIBDIR}/*.c)
HEADERS=$(wildcard ${INCLUDEDIR}/*.h)
OBJ=$(addprefix ${LIBDIR}/,$(notdir $(SRC:.c=.o)))

all: ${MAIN_LIB} ${PROGNAME}

${LIBDIR}/%.o: ${LIBDIR}/%.c $(DEPDIR)/%.d
	${CC} ${FLAGS} ${DEPFLAGS} -c -o $@ $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;

${MAIN_LIB}: ${OBJ} ${HEADERS}
	ar rcs $@ ${OBJ}

${PROGNAME}: ${OBJ} ${PROGNAME}.c ${HEADERS}
	${CC} ${FLAGS} -o ${PROGNAME} ${PROGNAME}.c ${LIBS}

.PHONY: dist
dist:
	make clean
	tar czf ../$(PROGNAME)-$(VERSION).tgz --transform='s,^${PROGNAME},$(PROGNAME)-$(VERSION),S' \
	--exclude=.git \
	--exclude=.gitignore \
	--exclude=README.md \
	-C .. ${PROGNAME}
	@mv ../$(PROGNAME)-$(VERSION).tgz ./

.PHONY: clean
clean:
	rm -f ${LIBDIR}/*.o ${LIBDIR}/*.a
	rm -f ${PROGNAME}
	rm -f $(PROGNAME)*.tgz
	rm -rf ${BASE_DEPDIR}

.PHONY: install
install: ${PROGNAME}
	install -m 0755 ${PROGNAME} ${INSTDIR}

.PHONY: uninstall
uninstall:
	-rm ${INSTDIR}/${PROGNAME}

-include $(patsubst %,$(BASE_DEPDIR)/%.d,$(basename $(SRC)))
