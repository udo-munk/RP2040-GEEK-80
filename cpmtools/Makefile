Z80ASMDIR = ../../z80pack/z80asm
Z80ASM = $(Z80ASMDIR)/z80asm
Z80ASMFLAGS = -8 -l -T -sn -p0

all: swlcd.com

swlcd.com: swlcd.asm $(Z80ASM)
	$(Z80ASM) $(Z80ASMFLAGS) -fb -o$@ $<

$(Z80ASM): FORCE
	$(MAKE) -C $(Z80ASMDIR)

FORCE:

install:

uninstall:

clean:
	rm -f swlcd.com swlcd.lis

distclean: clean

.PHONY: all FORCE install uninstall clean distclean
