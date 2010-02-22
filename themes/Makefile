EDC = \
fbreader_cb_header_footer.edc \
fbreader_entrybox.edc \
fbreader_messagebox.edc \
fbreader_oitheme.edc
EDJ = $(EDC:%.edc=%.edj)

all: $(EDJ)
%.edj: %.edc
	edje_cc -id . -o $@ $<

clean:
	rm -f $(EDJ)
