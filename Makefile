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
                kernel/goovm.c\
				kernel/cmos.c \
				kernel/gooc_simple.c\
				kernel/graphics.c

KERNEL_ASM_OBJS = $(KERNEL_ASM_SRCS:.asm=.asm.o)
KERNEL_C_OBJS = $(KERNEL_C_SRCS:.c=.o)
KERNEL_OBJS = $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

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
	echo 'menuentry "GooseOS v2.0" { multiboot /boot/kernel.bin boot }' > iso/boot/grub/grub.cfg
	$(GRUB) -o $@ iso 2>/dev/null

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

clean:
	rm -f $(KERNEL_OBJS) $(KERNEL_BIN) $(ISO)
	rm -rf iso


.PHONY: all run clean
