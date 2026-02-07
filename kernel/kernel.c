#include "libc.c"
#include "types.h"
/**
 * kernel_main - Главная точка входа ядра
 * 
 * Инициализирует все подсистемы ОС и запускает основной цикл.
 * Вызывается загрузчиком после перехода в защищенный режим.
 *
 * @param magic Магическое число от Multiboot загрузчика
 * @param mb_info Указатель на структуру информации Multiboot
 * 
 * @note Функция никогда не возвращает управление
 */
void kernel_main(uint32_t magic, void* mb_info) {
    // НАЧАЛО РЕАЛЬНОЙ ЗАГРУЗКИ
    real_boot_start();
    
    // 1. Проверка bootloader
    if (!real_check_multiboot(magic)) {
        // Ошибка уже показана в real_check_multiboot
        // TODO: нужна безопасная остановка системы
        panic("Multiboot verification failed");
        return;
    }
    
    // 2. Проверка памяти
    if (!real_check_memory(mb_info)) {
        panic("Memory check failed");
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
    // CMOS уже инициализирован в real_boot_start()
    
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
    // ВАЖНО: Проверяем существует ли fs и поле mounted
    // TODO: Добавить проверку extern struct filesystem fs;
    terminal_print("✓ Disk filesystem ready\n", VGA_COLOR_GREEN);
    
    // Показать приглашение
    terminal_show_prompt();
    
    // Главный цикл ядра
    while (1) {
        char key = keyboard_getch();
        if (key) {
            terminal_handle_input(key);
        }
        terminal_update();
        
        // Простая задержка (busy wait)
        // TODO: Заменить на прерывания таймера
        for (volatile int i = 0; i < 5000; i++) {
            // Пустое тело цикла для задержки
        }
    }
    
    // Сюда выполнение никогда не дойдет
    // panic("Kernel main loop exited unexpectedly");
}
