AS = nasm
CC = gcc
LD = ld
GRUB = grub-mkrescue

CFLAGS = -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I.
LDFLAGS = -m elf_i386 -T kernel/linker.ld -nostdlib

KERNEL_ASM_SRCS = kernel/kernel.asm
KERNEL_C_SRCS = kernel/kernel.c \
		kernel/vga.c \
		kernel/keyboard.c \
		kernel/terminal.c \
		kernel/fs.c \
		kernel/libc.c \
		kernel/cmos.c \
		kernel/graphics.c \
		kernel/calc.c \
		kernel/ata.c \
		kernel/diskfs.c \
		kernel/gooc_simple.c \
		kernel/goovm.c \
		kernel/panic.c\
                kernel/bootanim.c\
                kernel/realboot.c

KERNEL_ASM_OBJS = $(KERNEL_ASM_SRCS:.asm=.asm.o)
KERNEL_C_OBJS = $(KERNEL_C_SRCS:.c=.o)
KERNEL_OBJS = $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

DISK_IMG = gooseos.img
KERNEL_BIN = kernel.bin
ISO = gooseos.iso

all: $(ISO)

# Ассемблерные файлы
kernel/%.asm.o: kernel/%.asm
	$(AS) -f elf32 $< -o $@

# Си файлы
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Ядро
$(KERNEL_BIN): $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# ISO образ
$(ISO): $(KERNEL_BIN)
	mkdir -p iso/boot/grub
	cp $(KERNEL_BIN) iso/boot/
	echo 'menuentry "GooseOS v1.0 Beta" { multiboot /boot/kernel.bin boot }' > iso/boot/grub/grub.cfg
	$(GRUB) -o $@ iso 2>/dev/null

# Создаём пустой диск если его нет
$(DISK_IMG):
	dd if=/dev/zero of=$@ bs=1M count=10 2>/dev/null

run: $(ISO) $(DISK_IMG)
	qemu-system-i386 -cdrom $(ISO) -hda $(DISK_IMG) -m 128M

clean:
	rm -f $(KERNEL_OBJS) $(KERNEL_BIN) $(ISO) $(DISK_IMG)
	rm -rf iso

.PHONY: all run clean