# add .h files to dependencies: http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#combine
# FLAGS=-g3 -O3 -m64 -std=gnu9x -Wall -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
# $(info $$OBJ is [${OBJ}])

CC=gcc
LIBDIR=lib
INCLUDEDIR=./include
DESTDIR=
INSTDIR=/usr/bin
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

# some variables for building RPMs
bindir = $(INSTDIR)
specfile = packaging/$(PROGNAME).spec
distversion = $(VERSION)
rpmrelease = %nil
obsproject = home:gdha
obspackage = $(PROGNAME)-$(distversion)
# end of some variables for building RPMs

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
	@echo -e "\033[1m== Creating tar archive $(PROGNAME)-$(distversion).tar.gz ==\033[0;0m"
	make clean
	tar czf ../$(PROGNAME)-$(distversion).tar.gz \
	--exclude=.git \
	--exclude=.gitignore \
	--exclude=README.md \
	--transform='s,^\./,ebiso-0.1.1/,S' .
	@mv ../$(PROGNAME)-$(distversion).tar.gz ./

.PHONY: clean
clean:
	rm -f ${LIBDIR}/*.o ${LIBDIR}/*.a
	rm -f ${PROGNAME}
	rm -f $(PROGNAME)*.tar.gz
	rm -rf ${BASE_DEPDIR}

.PHONY: rpm
rpm: dist $(specfile)
	@echo -e "\033[1m== Building RPM package $(PROGNAME)-$(distversion) ==\033[0;0m"
	rpmbuild -v --clean  \
		--define "_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm" \
		--define "debug_package %{nil}" \
		--define "_rpmdir %(pwd)" \
		-tb  $(PROGNAME)-$(distversion).tar.gz

.PHONY: install
install: ${PROGNAME}
	@echo -e "\033[1m== Installing $(PROGNAME)-$(distversion) ==\033[0;0m"
	[ ! -d $(DESTDIR)$(INSTDIR) ] && mkdir -m 755 -p $(DESTDIR)$(INSTDIR)
	install -m 0755 ${PROGNAME} $(DESTDIR)$(INSTDIR)

.PHONY: uninstall
uninstall:
	-rm $(DESTDIR)$(INSTDIR)/${PROGNAME}

-include $(patsubst %,$(BASE_DEPDIR)/%.d,$(basename $(SRC)))
