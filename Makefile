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
DEFS = PROCDIR SYSCONFDIR PKGNAME
FLAGS = $(OPTIMISE) -std=$(STD) $(WARN) $(foreach D,$(DEFS),-D'$(D)="$($(D))"')


.PHONY: default
default: base info

.PHONY: all
all: base doc

.PHONY: base
base: bin/ponyguests-make-guest bin/ponyguests-login

bin/ponyguests-make-guest: src/ponyguests-make-guest
	@mkdir -p bin
	cp $< $@
	sed -i 's:@PROCDIR@:$(PROCDIR):g' $@
	sed -i 's:@DEVDIR@:$(DEVDIR):g' $@
	sed -i 's:@TMPDIR@:$(TMPDIR):g' $@
	sed -i 's:@PKGNAME@:$(PKGNAME):g' $@

bin/ponyguests-login: obj/ponyguests-login.o
	@mkdir -p bin
	$(CC) $(FLAGS) $(LDFLAGS) -o $@ $^

obj/ponyguests-login.o: src/ponyguests-login.c
	@mkdir -p obj
	$(CC) $(FLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

.PHONY: doc
doc: info pdf ps dvi

obj/fdl.texinfo: info/fdl.texinfo
	@mkdir -p obj
	cp $< $@

obj/ponyguests.texinfo: info/ponyguests.texinfo
	@mkdir -p obj
	cp $< $@
	sed -i 's:^\(@set SYSCONFDIR \).*$$:\1 $(SYSCONFDIR):' $@
	sed -i 's:^\(@set PKGNAME \).*$$:\1 $(PKGNAME):' $@

.PHONY: info
info: ponyguests.info
%.info: obj/%.texinfo obj/fdl.texinfo
	makeinfo $<

.PHONY: pdf
pdf: ponyguests.pdf
%.pdf: obj/%.texinfo obj/fdl.texinfo
	cd obj ; yes X | texi2pdf ../$<
	mv obj/$@ $@

.PHONY: dvi
dvi: ponyguests.dvi
%.dvi: obj/%.texinfo obj/fdl.texinfo
	cd obj ; yes X | $(TEXI2DVI) ../$<
	mv obj/$@ $@

.PHONY: ps
ps: ponyguests.ps
%.ps: obj/%.texinfo obj/fdl.texinfo
	cd obj ; yes X | texi2pdf --ps ../$<
	mv obj/$@ $@


.PHONY: install
install: install-base install-info

.PHONY: install-all
install-all: install-base install-doc

.PHONY: install-base
install-base: install-code install-license

.PHONY: install-code
install-code: bin/ponyguests-make-guest bin/ponyguests-login
	install -dm755 -- "$(DESTDIR)$(BINDIR)"
	install -m755 -- bin/ponyguests-login "$(DESTDIR)$(BINDIR)/ponyguests-login"
	install -m755 -- src/ponyguests-next-guest "$(DESTDIR)$(BINDIR)/ponyguests-next-guest"
	install -dm755 -- "$(DESTDIR)$(SYSCONFDIR)/$(PKGNAME)"
	install -m755 -- bin/ponyguests-make-guest "$(DESTDIR)$(SYSCONFDIR)/$(PKGNAME)/ponyguests-make-guest"
	install -m755 -- src/ponyguests-delete-guest "$(DESTDIR)$(SYSCONFDIR)/$(PKGNAME)/ponyguests-delete-guest"
	install -m644 -- src/gates-of-tartaros "$(DESTDIR)$(SYSCONFDIR)/$(PKGNAME)/gates-of-tartaros"

.PHONY: install-license
install-license:
	install -dm755 -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	install -m644 -- COPYING LICENSE "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"

.PHONY: install-doc
install-doc: install-info install-pdf install-ps install-dvi

.PHONY: install-info
install-info: ponyguests.info
	install -dm755 -- "$(DESTDIR)$(INFODIR)"
	install -m644 -- $< "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"

.PHONY: install-pdf
install-pdf: ponyguests.pdf
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 -- $< "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"

.PHONY: install-ps
install-ps: ponyguests.ps
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 -- $< "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"

.PHONY: install-dvi
install-dvi: ponyguests.dvi
	install -dm755 -- "$(DESTDIR)$(DOCDIR)"
	install -m644 -- $< "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"


.PHONY: uninstall
uninstall:
	-rm -- "$(DESTDIR)$(BINDIR)/ponyguests-login"
	-rm -- "$(DESTDIR)$(BINDIR)/ponyguests-next-guest"
	-rm -- "$(DESTDIR)$(SYSCONFDIR)/$(PKGNAME)/ponyguests-make-guest"
	-rm -- "$(DESTDIR)$(SYSCONFDIR)/$(PKGNAME)/ponyguests-delete-guest"
	-rm -- "$(DESTDIR)$(SYSCONFDIR)/$(PKGNAME)/gates-of-tartaros"
	-rmdir -- "$(DESTDIR)$(SYSCONFDIR)/$(PKGNAME)"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/COPYING"
	-rm -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)/LICENSE"
	-rmdir -- "$(DESTDIR)$(LICENSEDIR)/$(PKGNAME)"
	-rm -- "$(DESTDIR)$(INFODIR)/$(PKGNAME).info"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).pdf"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).ps"
	-rm -- "$(DESTDIR)$(DOCDIR)/$(PKGNAME).dvi"


.PHONY: clean
clean:
	-rm -r bin obj *.su src/*.su *.{info,pdf,ps,dvi}

