
UNAME_M := $(shell uname -m)

ifeq ($(UNAME_M),aarch64)
PREFIX:=i686-pc-linux-gnu-
BOOTIMG:=/usr/local/grub/lib/grub/i386-pc/boot.img
GRUBLOC:=/usr/local/grub/bin/
else
PREFIX:=
BOOTIMG:=/usr/lib/grub/i386-pc/boot.img
GRUBLOC :=
endif

CC := $(PREFIX)gcc
LD := $(PREFIX)ld
OBJDUMP := $(PREFIX)objdump
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
CONFIGS := -DCONFIG_HEAP_SIZE=4096
CFLAGS := -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall 

ODIR = obj
SDIR = src
APPDIR = apps_bin

OBJS = \
	kernel_main.o \
	putc.o \
	interrupt.o \
	rprintf.o \
	fat.o \
	page.o \
	ide.o \
	exec.o

# Make sure to keep a blank line here after OBJS list

OBJ = $(patsubst %,$(ODIR)/%,$(OBJS))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -c -g -o $@ $^

$(ODIR)/%.o: $(SDIR)/%.s
	$(CC) $(CFLAGS) -c -g -o $@ $^

all: rootfs.img

$(ODIR)/ide.o: $(SDIR)/ide.asm | obj
	nasm -f elf32 $< -o $@

kernel: obj $(OBJ)
	$(LD) -melf_i386  $(OBJ) -Tkernel.ld -o kernel
	$(SIZE) kernel

obj:
	mkdir -p obj

$(APPDIR):
	mkdir -p $(APPDIR)

rootfs.img: kernel
	dd if=/dev/zero of=rootfs.img bs=1M count=128
	$(GRUBLOC)grub-mkimage -p "(hd0,msdos1)/boot" -o grub.img -O i386-pc normal biosdisk multiboot multiboot2 configfile fat exfat part_msdos
	dd if=$(BOOTIMG) of=rootfs.img conv=notrunc
	dd if=grub.img of=rootfs.img conv=notrunc bs=512 seek=1 #########
	echo 'start=2048, type=83, bootable' | sfdisk rootfs.img
	mkfs.vfat --offset 2048 -F16 rootfs.img
	mcopy -i rootfs.img@@1M kernel ::/
	mmd -i rootfs.img@@1M boot
	mcopy -i rootfs.img@@1M grub.cfg ::/boot
	mcopy -i rootfs.img@@1M src/testfile.txt ::/
	@echo " -- BUILD COMPLETED SUCCESSFULLY --"

run: 
	qemu-system-i386 -hda rootfs.img

debug:
	./launch_qemu.sh -monitor stdio

clear_vga_memory: rootfs.img
	i686-pc-linux-gnu-gcc -m32 -ffreestanding -fno-pic \
		-fno-stack-protector -fno-asynchronous-unwind-tables \
		-fno-unwind-tables -nostdlib \
		-c clear_vga_memory.c -o obj/clear_vga_memory.o
	i686-pc-linux-gnu-ld -melf_i386 -T app.ld obj/clear_vga_memory.o -o clear_vga_memory.elf
	i686-pc-linux-gnu-objcopy -O binary clear_vga_memory.elf apps_bin/CLEARVGA.BIN
	mcopy -i rootfs.img@@1M apps_bin/CLEARVGA.BIN ::/
	@echo "CLEARVGA.BIN copied into filesystem"

clean:
	rm -f grub.img kernel rootfs.img obj/* bin/* apps_bin/*
