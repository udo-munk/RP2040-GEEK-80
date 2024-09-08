Z80ASMDIR = ../../z80pack/z80asm
Z80ASM = $(Z80ASMDIR)/z80asm
Z80ASMFLAGS = -l -T -sn -p0

all: ../bootrom.c $(Z80ASM)

../bootrom.c: bootrom.asm
	$(Z80ASM) $(Z80ASMFLAGS) -fc -o../bootrom.c $<

$(Z80ASM): FORCE
	$(MAKE) -C $(Z80ASMDIR)

FORCE:
