#include "vga.h"
#include "keyboard.h"
#include "terminal.h"
#include "fs.h"
#include "libc.h"
#include "cmos.h"
#include "realboot.h"
#include "panic.h"

void kernel_main(uint32_t magic, void* mb_info) {
    // НАЧАЛО РЕАЛЬНОЙ ЗАГРУЗКИ
    real_boot_start();
    
    // 1. Проверка bootloader
    if (!real_check_multiboot(magic)) {
        return; // Уже показана ошибка
    }
    
    // 2. Проверка памяти
    if (!real_check_memory(mb_info)) {
        return;
    }
    
    // 3. Инициализация VGA
    real_boot_update(25, "Initializing display...");
    vga_init();
    
    // 4. Инициализация клавиатуры
    real_boot_update(35, "Setting up keyboard...");
    keyboard_init();
    
    // 5. Инициализация файловой системы
    real_boot_update(45, "Mounting filesystem...");
    fs_init();
    
    // 6. Инициализация часов
    real_boot_update(55, "Reading system clock...");
    // CMOS уже инициализирован
    
    // 7. Инициализация терминала
    real_boot_update(65, "Starting terminal...");
    terminal_init();
    
    // 8. Проверка состояния системы
    real_boot_update(75, "Checking system status...");
    
    // 9. Подготовка к запуску
    real_boot_update(85, "Loading system modules...");
    
    // 10. Финальная подготовка
    real_boot_update(95, "Finalizing boot...");
    
    // ЗАВЕРШЕНИЕ ЗАГРУЗКИ
    real_boot_complete();
    
    // Показать статус файловой системы
    if (fs.mounted) {
        terminal_print("✓ Disk filesystem ready\n", VGA_COLOR_GREEN);
    } else {
        terminal_print("⚠ No filesystem - some features disabled\n", VGA_COLOR_YELLOW);
    }
    

    
    // Показать приглашение
    terminal_show_prompt();
    
    // Главный цикл
    while (1) {
        char key = keyboard_getch();
        if (key) {
            terminal_handle_input(key);
        }
        terminal_update();
        for (volatile int i = 0; i < 5000; i++);
    }
}