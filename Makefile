# Makefile для GooseOS
# Версия 2.0 - с автоматическим созданием диска

.PHONY: all clean run debug qemu format help setup

# Компиляторы
CC = gcc
ASM = nasm
LD = ld

# Флаги
CFLAGS = -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin
ASMFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# Исходные файлы ядра
C_SOURCES = $(wildcard *.c)
C_OBJECTS = $(C_SOURCES:.c=.o)
ASM_SOURCES = kernel.asm
ASM_OBJECTS = kernel_asm.o

# Основная цель
all: gooseos.iso disk.img
	@echo "✅ GooseOS собран! Используй 'make run' для запуска"

# Создание образа диска (10MB)
disk.img:
	@echo "💾 Создаю образ диска..."
	@dd if=/dev/zero of=disk.img bs=1M count=10 2>/dev/null
	@echo "✅ Диск создан: disk.img"

# Компиляция C файлов
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Компиляция ассемблера
kernel_asm.o: kernel.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Линковка ядра
kernel.bin: $(C_OBJECTS) $(ASM_OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^
	@echo "✅ Ядро слинковано: kernel.bin"

# Создание ISO
gooseos.iso: kernel.bin
	@echo "📀 Создаю загрузочный ISO..."
	@mkdir -p isodir/boot/grub
	@cp kernel.bin isodir/boot/
	@echo 'set timeout=0' > isodir/boot/grub/grub.cfg
	@echo 'set default=0' >> isodir/boot/grub/grub.cfg
	@echo 'menuentry "GooseOS" {' >> isodir/boot/grub/grub.cfg
	@echo '  multiboot /boot/kernel.bin' >> isodir/boot/grub/grub.cfg
	@echo '  boot' >> isodir/boot/grub/grub.cfg
	@echo '}' >> isodir/boot/grub/grub.cfg
	@grub-mkrescue -o gooseos.iso isodir 2>/dev/null
	@echo "✅ ISO создан: gooseos.iso"

# ЗАПУСК В QEMU (главная команда!)
run: gooseos.iso disk.img
	@echo "🚀 Запускаю GooseOS в QEMU..."
	@echo "=================================================="
	@echo "  В терминале выполни:"
	@echo "    1. format    - отформатировать диск"
	@echo "    2. fsinfo    - посмотреть информацию о ФС"
	@echo "    3. help      - список всех команд"
	@echo ""
	@echo "  Горячие клавиши:"
	@echo "    Alt+Shift    - переключение раскладки EN/RU"
	@echo "    Ctrl+Alt+G   - выход из QEMU"
	@echo "=================================================="
	@qemu-system-i386 \
		-cdrom gooseos.iso \
		-hda disk.img \
		-serial stdio \
		-m 256 \
		-no-reboot \
		-no-shutdown \
		-name "GooseOS v1.0"

# Быстрый запуск (без сообщений)
qemu: gooseos.iso disk.img
	@qemu-system-i386 \
		-cdrom gooseos.iso \
		-hda disk.img \
		-serial stdio \
		-m 256

# Отладка
debug: gooseos.iso disk.img
	@echo "🐛 Запуск с отладкой (gdb)..."
	@echo "Подключись: target remote localhost:1234"
	@qemu-system-i386 \
		-cdrom gooseos.iso \
		-hda disk.img \
		-serial stdio \
		-m 256 \
		-s -S

# Очистка
clean:
	@echo "🧹 Очистка..."
	@rm -f *.o *.bin *.iso *.img
	@rm -rf isodir
	@echo "✅ Очистка завершена"

# Форматирование кода
format:
	@if command -v clang-format >/dev/null; then \
		clang-format -i *.c *.h; \
		echo "✅ Код отформатирован"; \
	else \
		echo "⚠️ Установи clang-format: sudo apt install clang-format"; \
	fi

# Установка зависимостей
setup:
	@echo "⚙️ Установка зависимостей..."
	@which nasm >/dev/null || (echo "Установка nasm..." && sudo apt-get install -y nasm)
	@which gcc >/dev/null || (echo "Установка gcc..." && sudo apt-get install -y gcc-multilib)
	@which grub-mkrescue >/dev/null || (echo "Установка grub..." && sudo apt-get install -y grub-pc-bin)
	@which qemu-system-i386 >/dev/null || (echo "Установка qemu..." && sudo apt-get install -y qemu-system-x86)
	@echo "✅ Зависимости установлены!"

# Справка
help:
	@echo "🐧 GooseOS - Makefile команды:"
	@echo ""
	@echo "  make           - Собрать ОС и создать диск"
	@echo "  make run       - Запустить в QEMU (рекомендуется)"
	@echo "  make qemu      - Быстрый запуск"
	@echo "  make debug     - Запуск с отладкой"
	@echo "  make clean     - Очистить все файлы"
	@echo "  make format    - Форматировать код"
	@echo "  make setup     - Установить зависимости"
	@echo ""
	@echo "Пример:"
	@echo "  make clean     # очистить"
	@echo "  make           # собрать"
	@echo "  make run       # запустить"