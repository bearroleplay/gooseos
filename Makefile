# Основные цели
.PHONY: all build clean format check help

# Основная цель по умолчанию - сборка
all: build

# Сборка ОС
build: clean
	@echo "🔨 Сборка GooseOS..."
	nasm -f elf32 kernel/kernel.asm -o kernel/kernel.asm.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/kernel.c -o kernel/kernel.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/vga.c -o kernel/vga.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/keyboard.c -o kernel/keyboard.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/terminal.c -o kernel/terminal.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/libc.c -o kernel/libc.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/fs.c -o kernel/fs.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/cmos.c -o kernel/cmos.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/graphics.c -o kernel/graphics.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/calc.c -o kernel/calc.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/ata.c -o kernel/ata.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/diskfs.c -o kernel/diskfs.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/gooc_simple.c -o kernel/gooc_simple.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/goovm.c -o kernel/goovm.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/panic.c -o kernel/panic.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/bootanim.c -o kernel/bootanim.o
	gcc -m32 -std=c99 -ffreestanding -O2 -Wall -Wextra -nostdlib -fno-builtin -U_FORTIFY_SOURCE -I. -c kernel/realboot.c -o kernel/realboot.o
	ld -m elf_i386 -T kernel/linker.ld -nostdlib -o kernel.bin kernel/kernel.asm.o kernel/kernel.o kernel/vga.o kernel/keyboard.o kernel/terminal.o kernel/fs.o kernel/libc.o kernel/cmos.o kernel/graphics.o kernel/calc.o kernel/ata.o kernel/diskfs.o kernel/gooc_simple.o kernel/goovm.o kernel/panic.o kernel/bootanim.o kernel/realboot.o
	@echo "✅ GooseOS собран!"

# Очистка
clean:
	@echo "🧹 Очистка проекта..."
	rm -f kernel/kernel.asm.o kernel/kernel.o kernel/vga.o kernel/keyboard.o kernel/fs.o kernel/terminal.o  kernel/libc.o kernel/cmos.o kernel/graphics.o kernel/calc.o kernel/ata.o kernel/diskfs.o kernel/gooc_simple.o kernel/goovm.o kernel/panic.o kernel/bootanim.o kernel/realboot.o kernel.bin gooseos.iso gooseos.img

# Форматирование кода (ОТДЕЛЬНАЯ команда)
format:
	@echo "🔧 Форматирование кода..."
	@bash scripts/format_code.sh

# Проверка кода (ОТДЕЛЬНАЯ команда)
check:
	@echo "🔍 Проверка качества кода..."
	@bash scripts/check_comments.sh

iso: build
	@echo "📀 Создание образа ISO..."
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/kernel.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o gooseos.iso isodir
	@echo "✅ Образ gooseos.iso создан!"

# Настройка окружения
setup:
	@echo "⚙️  Настройка окружения..."
	@which clang-format >/dev/null 2>&1 || (echo "Установка clang-format..." && sudo apt-get install -y clang-format)
	@chmod +x scripts/*.sh
	@echo "✅ Готово! Доступные команды:"
	@echo "   make build  - собрать ОС"
	@echo "   make clean  - очистить"
	@echo "   make format - форматировать код"
	@echo "   make check  - проверить комментарии"

# Справка
help:
	@echo "Доступные команды для GooseOS:"
	@echo ""
	@echo "  Сборка:"
	@echo "    make build   - Собрать операционную систему"
	@echo "    make clean   - Очистить скомпилированные файлы"
	@echo ""
	@echo "  Качество кода:"
	@echo "    make format  - Отформатировать код и добавить комментарии"
	@echo "    make check   - Проверить наличие комментариев у функций"
	@echo "    make setup   - Настроить окружение (clang-format + права)"
	@echo ""
	@echo "  По умолчанию: make = make build"




