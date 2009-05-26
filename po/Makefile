PACKAGE = fbreader
 
POFILES = $(wildcard *.po)
MOFILES = $(patsubst %.po,%.mo,$(POFILES))
 
ROOTDIR = $(CURDIR)/..
MAKEFILESDIR = $(ROOTDIR)/makefiles
include $(MAKEFILESDIR)/config.mk

top_builddir = ..
localedir=$(SHAREDIR)/locale
 
MSGFMT = msgfmt
 
all: $(MOFILES)
 
%.mo: %.po
	@echo "$(MSGFMT) -o $@ $<"; $(MSGFMT) -o t-$@ $< && mv t-$@ $@
 
do_install: $(MOFILES)
	@for cat in "" $(MOFILES); do \
		if test -z "$$cat"; then continue; fi; \
		cat=`basename $$cat`; \
		lang=`echo $$cat | sed -e 's/\.mo$$//'`; \
		dir=$(localedir)/$$lang/LC_MESSAGES; \
		mkdir -p $(DEST_DIR)$$dir; \
		if test -r $$cat; then realcat=$$cat; else realcat=$(srcdir)/$$cat; fi; \
		install -m 0644 $$realcat $(DEST_DIR)$$dir/$(PACKAGE).mo; \
	done

clean:
	$(RM) $(MOFILES)
