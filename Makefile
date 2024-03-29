# FIXME func.ovl requires -O1 optimization for some reason (thanks, rankaisija!)
# XXX it has been a while and -Os seems to be fine now; just keep -O1 in mind if things ever break again

# toolchain
CC      = mips64-gcc
LD      = mips64-ld
OBJCOPY = mips64-objcopy
OBJDUMP = mips64-objdump
NOVL    = novl

# entry point
ADDRESS = 0x80800000

# default compilation flags
CFLAGS = -DNDEBUG -Wall -Wno-main -fno-reorder-blocks -fno-common -mno-gpopt -Wno-unused-function -Wno-strict-aliasing -fomit-frame-pointer -G 0 --std=gnu99 -mtune=vr4300 -mabi=32 -mips3 -mno-check-zero-division -mno-explicit-relocs -mno-memcpy
LDFLAGS = -L$(Z64OVL_LD) -T z64ovl.ld --emit-relocs
NOVLFLAGS = -v -c -A $(ADDRESS) -o func.ovl

default: func.ovl mod.bin

mod.bin:
	mkdir -p bin
	@echo -n "ENTRY_POINT = " > entry.ld
	@echo -n $(ADDRESS) >> entry.ld
	@echo -n ";" >> entry.ld
	@$(CC) -c src/main.c $(CFLAGS) -Os
	@mv main.o bin/main.o
	@$(LD) -o bin/main.elf bin/main.o $(LDFLAGS) $(LDFILE)
	@$(OBJCOPY) -R .MIPS.abiflags -O binary bin/main.elf mod.bin
	@$(OBJDUMP) -t bin/main.elf | grep main
	@echo "WARNING: ensure main is at 80800000; it won't work otherwise"

func.ovl:
	mkdir -p bin
	@echo -n "ENTRY_POINT = " > entry.ld
	@echo -n $(ADDRESS) >> entry.ld
	@echo -n ";" >> entry.ld
	#@$(CC) -c src/func.c $(CFLAGS) -O1
	@$(CC) -c src/func.c $(CFLAGS) -Os
	@mv func.o bin/func.o
	@$(LD) -o bin/func.elf bin/func.o $(LDFLAGS) $(LDFILE)
	$(NOVL) $(NOVLFLAGS) bin/func.elf
	@echo "func.ovl:"
	@$(OBJDUMP) -t bin/func.elf | grep renderinit

clean:
	rm -f *.bin *.elf *.ovl

