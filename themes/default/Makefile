EDC = \
fbreader.edc \
fbreader_entrybox.edc \
fbreader_messagebox.edc

EDJ = $(EDC:%.edc=%.edj)

all: $(EDJ)
%.edj: %.edc
	edje_cc -id . -o $@ $<

clean:
	rm -f $(EDJ)
