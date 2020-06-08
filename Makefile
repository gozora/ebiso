# add .h files to dependencies: http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/#combine
# FLAGS=-g3 -O3 -m64 -std=gnu9x -Wall -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
# $(info $$OBJ is [${OBJ}])

CC ?= gcc
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
MANDIR=/usr/share/man

#FLAGS=-fmessage-length=0 -grecord-gcc-switches -O2 -Wall -D_FORTIFY_SOURCE=2 -fstack-protector -funwind-tables -fasynchronous-unwind-tables -I${INCLUDEDIR}
CFLAGS=-fcommon
FLAGS=$(CFLAGS) -g3 -std=gnu9x -Wall -Wshadow -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -I${INCLUDEDIR}
VERSION=$(shell grep "\#define EBISO_VERSION" ${INCLUDEDIR}/${PROGNAME}.h | awk '{ print $$NF }' | sed s/\"//g)

SRC=$(sort $(wildcard ${LIBDIR}/*.c))
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
dist: clean rewrite $(PROGNAME)-$(distversion).tar.gz restore

.PHONY: $(PROGNAME)-$(distversion).tar.gz
$(PROGNAME)-$(distversion).tar.gz:
	@echo -e "\033[1m== Creating tar archive $(PROGNAME)-$(distversion).tar.gz ==\033[0;0m"
	tar czf ../$(PROGNAME)-$(distversion).tar.gz \
	--exclude=.git \
	--exclude=.gitignore \
	--exclude=README.md \
	--transform='s,^\.,ebiso-$(distversion),S' .
	@mv ../$(PROGNAME)-$(distversion).tar.gz ./

.PHONY: clean
clean:
	@echo -e "\033[1m== Cleaning up ==\033[0;0m"
	rm -f ${LIBDIR}/*.o ${LIBDIR}/*.a
	rm -f $(PROGNAME)
	rm -f $(PROGNAME)*.tar.gz
	rm -f $(PROGNAME)*.rpm
	rm -rf ${BASE_DEPDIR}

.PHONY: rewrite
rewrite: 
	@echo -e "\033[1m== Rewriting $(specfile) (updating version) ==\033[0;0m"
	sed -i.orig -e 's#^Version:.*#Version:\t\t$(distversion)#' $(specfile)

.PHONY: restore
restore:
	@echo -e "\033[1m== Restore original $(specfile) ==\033[0;0m"
	mv -f $(specfile).orig $(specfile)

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
	if [ ! -d $(DESTDIR)$(INSTDIR) ]; then mkdir -m 755 -p $(DESTDIR)$(INSTDIR); fi
	install -m 0755 ${PROGNAME} $(DESTDIR)$(INSTDIR)
	if [ ! -d $(DESTDIR)${MANDIR}/man1 ]; then mkdir -m 755 -p $(DESTDIR)${MANDIR}/man1; fi
	install -m 0644 man/man1/$(PROGNAME).1.gz $(DESTDIR)${MANDIR}/man1/
	strip $(DESTDIR)$(INSTDIR)/$(PROGNAME)

.PHONY: uninstall
uninstall:
	-rm $(DESTDIR)$(INSTDIR)/${PROGNAME}
	-rm $(DESTDIR)${MANDIR}/man1/$(PROGNAME).1.gz

-include $(patsubst %,$(BASE_DEPDIR)/%.d,$(basename $(SRC)))
