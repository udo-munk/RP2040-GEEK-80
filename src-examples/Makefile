Z80ASMDIR = ../../z80pack/z80asm
Z80ASM = $(Z80ASMDIR)/z80asm
Z80ASMFLAGS = -fb -l -T -sn -p0

all: life.bin micro80.bin serial.bin tb.bin test8080.bin

life.bin: life.asm $(Z80ASM)
	$(Z80ASM) $(Z80ASMFLAGS) -8 $<

micro80.bin: micro80.asm $(Z80ASM)
	$(Z80ASM) $(Z80ASMFLAGS) -8 $<

serial.bin: serial.asm $(Z80ASM)
	$(Z80ASM) $(Z80ASMFLAGS) $<

tb.bin: tb.asm $(Z80ASM)
	$(Z80ASM) $(Z80ASMFLAGS) -8 -x $<

test8080.bin: test8080.asm $(Z80ASM)
	$(Z80ASM) $(Z80ASMFLAGS) -8 -dONCPM=0 $<

$(Z80ASM): FORCE
	$(MAKE) -C $(Z80ASMDIR)

FORCE:

install:

uninstall:

clean:
	rm -f life.bin micro80.bin serial.bin tb.bin test8080.bin

distclean: clean

.PHONY: all FORCE install uninstall clean distclean
