# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.


PREFIX = /usr
BIN = /bin
DATA = /share
BINDIR = $(PREFIX)$(BIN)
DATADIR = $(PREFIX)$(DATA)
DOCDIR = $(DATADIR)/doc
INFODIR = $(DATADIR)/info
LICENSEDIR = $(DATADIR)/licenses
SYSCONFDIR = /etc
PROCDIR = /proc
DEVDIR = /dev
TMPDIR = /tmp

PKGNAME = ponyguests


OPTIMISE = -Os
STD = gnu99
WARN = -Wall -Wextra -pedantic -Wdouble-promotion -Wformat=2 -Winit-self -Wmissing-include-dirs      \
       -Wtrampolines -Wmissing-prototypes -Wmissing-declarations -Wnested-externs                    \
       -Wno-variadic-macros -Wsync-nand -Wunsafe-loop-optimizations -Wcast-align                     \
       -Wdeclaration-after-statement -Wundef -Wbad-function-cast -Wwrite-strings -Wlogical-op        \
       -Wstrict-prototypes -Wold-style-definition -Wpacked -Wvector-operation-performance            \
       -Wunsuffixed-float-constants -Wsuggest-attribute=const -Wsuggest-attribute=noreturn           \
       -Wsuggest-attribute=format -Wnormalized=nfkc -fstrict-aliasing -fipa-pure-const -ftree-vrp    \
       -fstack-usage -funsafe-loop-optimizations -Wshadow -Wredundant-decls -Winline -Wcast-qual     \
       -Wsign-conversion -Wstrict-overflow=5 -Wconversion -Wsuggest-attribute=pure -Wswitch-default  \
       -Wstrict-aliasing=1 -fstrict-overflow -Wfloat-equal -Waggregate-return
DEFS = SYSCONFDIR PKGNAME
FLAGS = $(OPTIMISE) -std=$(STD) $(WARN) $(foreach D,$(DEFS),-D'$(D)="$($(D))"')


.PHONY: all
all: bin/ponyguests-make-guest bin/ponyguests-login

bin/ponyguests-make-guest: src/ponyguests-make-guest
	@mkdir -p bin
	cp $< $@
	sed -i 's:@PROCDIR@:$(PROCDIR):g' $@
	sed -i 's:@DEVDIR@:$(DEVDIR):g' $@
	sed -i 's:@TMPDIR@:$(TMPDIR):g' $@

bin/ponyguests-login: obj/ponyguests-login.o
	@mkdir -p bin
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $^

obj/ponyguests-login.o: src/ponyguests-login.c
	@mkdir -p obj
	$(CC) $(FLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<


.PHONY: clean
clean:
	-rm -r bin obj *.su src/*.su *.{info,pdf,ps,dvi}

