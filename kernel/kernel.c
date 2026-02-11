#include "vga.h"
#include "keyboard.h"
#include "terminal.h"
#include "fs.h"
#include "account.h"
#include "libc.h"
#include "cmos.h"
#include "graphics.h"
#include "realboot.h"

void kernel_main(uint32_t magic, void* mb_info) {
    (void)magic;
    (void)mb_info;
    
    // Загрузочный экран
    real_boot_start();
    real_check_multiboot(magic);
    real_check_memory(mb_info);
    
    // Инициализация
    real_boot_update(30, "Initializing filesystem...");
    fs_init();
    
    real_boot_update(50, "Loading keyboard...");
    keyboard_init();
    
    real_boot_update(60, "Loading mouse...");
    mouse_init();
    
    // ===== АККАУНТЫ ВРЕМЕННО ОТКЛЮЧЕНЫ =====
    // real_boot_update(70, "Loading user accounts...");
    // account_load_all();
    // 
    // if (acct_sys.boot_counter == 1) {
    //     real_boot_update(80, "First boot - running setup...");
    //     real_boot_complete();
    //     account_first_boot_setup();
    // }
    // 
    // real_boot_update(90, "Starting login manager...");
    // real_boot_complete();
    // account_show_login_manager();
    
    // Просто завершаем загрузку и запускаем терминал
    real_boot_update(100, "GooseOS ready!");
    real_boot_complete();
    
    // Запускаем терминал сразу
    terminal_init();
    
    // Главный цикл
    while (1) {
        char key = keyboard_getch();
        if (key) {
            terminal_handle_input(key);
        }

        if (inb(0x64) & 1) {
            uint8_t data = inb(0x60);
            // Обработка мыши
        }
        
        terminal_update();
        
        for (volatile int i = 0; i < 5000; i++);
    }
}