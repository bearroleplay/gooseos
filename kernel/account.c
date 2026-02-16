#include "account.h"
#include "terminal.h"
#include "keyboard.h"
#include "fs.h"
#include "libc.h"
#include "cmos.h"
#include "libc.h" 

// Глобальная система аккаунтов - ОБЯЗАТЕЛЬНО ОБНУЛЯТЬ!
AccountSystem acct_sys;

// ========== ХЕШИРОВАНИЕ ПАРОЛЕЙ - ПРОСТОЕ НО РАБОЧЕЕ ==========
static void generate_salt(uint8_t* salt) {
    for (int i = 0; i < SALT_SIZE; i++) {
        salt[i] = (uint8_t)((i * 0x55 + 0xAA) & 0xFF);
    }
}

static int small_snprintf(char* str, size_t size, const char* format, ...) {
    char* ptr = str;
    const char* fmt = format;
    
    while (*fmt && (ptr - str) < (int)size - 1) {
        if (*fmt == '%') {
            fmt++;
            if (*fmt == 's') {
                fmt++;
                char* arg = *((char**)(&format + 1));
                while (*arg && (ptr - str) < (int)size - 1) {
                    *ptr++ = *arg++;
                }
            } else if (*fmt == 'd' || *fmt == 'i') {
                fmt++;
                int arg = *((int*)(&format + 1));
                char num[32];
                itoa(arg, num, 10);
                char* n = num;
                while (*n && (ptr - str) < (int)size - 1) {
                    *ptr++ = *n++;
                }
            } else if (*fmt == 'c') {
                fmt++;
                char arg = *((char*)(&format + 1));
                if ((ptr - str) < (int)size - 1) {
                    *ptr++ = arg;
                }
            } else {
                *ptr++ = '%';
                if (*fmt) *ptr++ = *fmt++;
            }
        } else {
            *ptr++ = *fmt++;
        }
    }
    *ptr = 0;
    return ptr - str;
}

// ОЧЕНЬ ПРОСТОЙ ХЕШ - ГАРАНТИРОВАННО РАБОТАЕТ
static void hash_password(const char* password, const uint8_t* salt, char* output) {
    uint32_t hash = 0;
    int len = strlen(password);
    
    // Простейший XOR хеш
    for (int i = 0; i < len; i++) {
        hash += password[i];
        hash ^= salt[i % SALT_SIZE];
        hash = (hash << 3) | (hash >> 29);
    }
    
    // Конвертируем в hex
    char hex[16];
    itoa(hash, hex, 16);
    strcpy(output, hex);
}

// ========== ЗАГРУЗКА/СОХРАНЕНИЕ ==========
int account_save_all(void) {
    // Сохраняем в /etc/passwd.goose
    char* data = (char*)malloc(4096);
    if (!data) return 0;
    
    char* ptr = data;
    
    ptr += small_snprintf(ptr, 4096 - (ptr - data), "GOOSEACCT|1|%s|%d\n", 
                   acct_sys.system_timezone, acct_sys.boot_counter);
    
    for (int i = 0; i < acct_sys.account_count; i++) {
        Account* a = &acct_sys.accounts[i];
        ptr += small_snprintf(ptr, 4096 - (ptr - data), "%s|%s|%d|%s\n", 
                       a->username, a->password_hash, 
                       a->account_type, a->home_dir);
    }
    
    int result = fs_write("/etc/passwd.goose", (uint8_t*)data, ptr - data);
    free(data);
    
    // Сохраняем текущего пользователя
    char cur_str[16];
    int cur_len = small_snprintf(cur_str, sizeof(cur_str), "%d", acct_sys.current_account);
    fs_write("/etc/current.goose", (uint8_t*)cur_str, cur_len);
    
    return result > 0;
}

int account_load_all(void) {
    // ВАЖНО! ПОЛНОСТЬЮ ОЧИЩАЕМ ВСЮ СТРУКТУРУ
    memset(&acct_sys, 0, sizeof(AccountSystem));
    
    acct_sys.current_account = -1;
    acct_sys.logged_in = 0;
    acct_sys.account_count = 0;  // ЯВНО ОБНУЛЯЕМ!
    strcpy(acct_sys.system_timezone, "UTC+3");
    acct_sys.boot_counter = 1;
    
    // Проверяем первый запуск
    if (!fs_exists("/etc/passwd.goose")) {
        return 0;  // Первый запуск
    }
    
    // Загружаем passwd
    uint8_t buffer[4096];
    int size = fs_read("/etc/passwd.goose", buffer, sizeof(buffer) - 1);
    if (size <= 0) return 0;
    
    buffer[size] = 0;
    
    // Парсим строки
    char* line = strtok((char*)buffer, "\n");
    if (!line) return 0;
    
    // Парсим заголовок
    char* token = strtok(line, "|");
    if (token && strcmp(token, "GOOSEACCT") == 0) {
        token = strtok(NULL, "|"); // версия
        token = strtok(NULL, "|"); // таймзона
        if (token) strcpy(acct_sys.system_timezone, token);
        token = strtok(NULL, "|"); // boot_counter
        if (token) acct_sys.boot_counter = atoi(token);
    }
    
    // Парсим аккаунты
    acct_sys.account_count = 0;
    while ((line = strtok(NULL, "\n")) != NULL && acct_sys.account_count < MAX_ACCOUNTS) {
        char* user = strtok(line, "|");
        char* pass = strtok(NULL, "|");
        char* type = strtok(NULL, "|");
        char* home = strtok(NULL, "|");
        
        if (user && pass && type) {
            Account* a = &acct_sys.accounts[acct_sys.account_count];
            memset(a, 0, sizeof(Account));
            
            strcpy(a->username, user);
            strcpy(a->password_hash, pass);
            a->account_type = atoi(type);
            
            if (home) {
                strcpy(a->home_dir, home);
            } else {
                small_snprintf(a->home_dir, MAX_PATH - 1, "/home/%s", user);
            }
            
            acct_sys.account_count++;
        }
    }
    
    // Загружаем текущего пользователя
    if (fs_exists("/etc/current.goose")) {
        char cur_buf[16];
        int cur_size = fs_read("/etc/current.goose", (uint8_t*)cur_buf, sizeof(cur_buf) - 1);
        if (cur_size > 0) {
            cur_buf[cur_size] = 0;
            acct_sys.current_account = atoi(cur_buf);
            if (acct_sys.current_account >= 0 && acct_sys.current_account < acct_sys.account_count) {
                acct_sys.logged_in = 1;
            } else {
                acct_sys.current_account = -1;
                acct_sys.logged_in = 0;
            }
        }
    }
    
    return 1;
}

// ========== ПЕРВЫЙ ЗАПУСК ==========
void account_first_boot_setup(void) {
    // ПОЛНОСТЬЮ ОЧИЩАЕМ СИСТЕМУ АККАУНТОВ
    memset(&acct_sys, 0, sizeof(AccountSystem));
    
    acct_sys.current_account = -1;
    acct_sys.logged_in = 0;
    acct_sys.account_count = 0;
    acct_sys.boot_counter = 2;  // Уже не первый запуск после установки
    strcpy(acct_sys.system_timezone, "UTC+3");
    
    vga_clear();
    
    // Синий фон
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putchar(' ', vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE), x, y);
        }
    }
    
    terminal_print_at("╔══════════════════════════════════════════════════════════╗", 10, 2, VGA_COLOR_YELLOW);
    terminal_print_at("║                 GOOSE OS v1.0 - FIRST SETUP              ║", 10, 3, VGA_COLOR_WHITE);
    terminal_print_at("╚══════════════════════════════════════════════════════════╝", 10, 4, VGA_COLOR_YELLOW);
    
    // ===== 1. СОЗДАЁМ СИСТЕМНЫЕ ПАПКИ =====
    terminal_print_at("[1/4] Creating system directories...", 15, 7, VGA_COLOR_YELLOW);
    
    fs_cd("/");
    fs_mkdir("etc");
    fs_mkdir("home");
    fs_mkdir("boot");
    fs_mkdir("usr");
    
    terminal_print_at("      ✓ System directories created", 15, 8, VGA_COLOR_GREEN);
    
    // ===== 2. ВЫБОР ЧАСОВОГО ПОЯСА =====
    terminal_print_at("\n[2/4] Select your timezone:", 15, 11, VGA_COLOR_YELLOW);
    terminal_print_at("   1. UTC+3 (Moscow) - default", 20, 13, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("   2. UTC+2 (Kyiv)", 20, 14, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("   3. UTC+1 (Berlin)", 20, 15, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("   4. UTC+0 (London)", 20, 16, VGA_COLOR_LIGHT_GRAY);
    terminal_print_at("   5. UTC-5 (New York)", 20, 17, VGA_COLOR_LIGHT_GRAY);
    
    terminal_print_at("   Choice (1-5): ", 20, 19, VGA_COLOR_GREEN);
    
    char choice = 0;
    while (!choice) {
        char key = keyboard_getch();
        if (key >= '1' && key <= '5') {
            choice = key;
            vga_putchar(key, VGA_COLOR_WHITE, 38, 19);
            break;
        }
    }
    
    switch (choice) {
        case '1': strcpy(acct_sys.system_timezone, "UTC+3"); break;
        case '2': strcpy(acct_sys.system_timezone, "UTC+2"); break;
        case '3': strcpy(acct_sys.system_timezone, "UTC+1"); break;
        case '4': strcpy(acct_sys.system_timezone, "UTC+0"); break;
        case '5': strcpy(acct_sys.system_timezone, "UTC-5"); break;
        default:  strcpy(acct_sys.system_timezone, "UTC+3"); break;
    }
    
    // ===== 3. УСТАНОВКА ВРЕМЕНИ =====
    terminal_print_at("\n[3/4] Set current time:", 15, 22, VGA_COLOR_YELLOW);
    
    // Часы
    terminal_print_at("   Hour (0-23) [12]: ", 20, 24, VGA_COLOR_CYAN);
    char hour_str[3] = "12";
    terminal_print_at("12", 41, 24, VGA_COLOR_WHITE);
    int hour = 12;
    
    // Минуты
    terminal_print_at("\n   Minute (0-59) [0]: ", 20, 26, VGA_COLOR_CYAN);
    char min_str[3] = "00";
    terminal_print_at("00", 42, 26, VGA_COLOR_WHITE);
    int minute = 0;
    
    // Секунды
    terminal_print_at("\n   Second (0-59) [0]: ", 20, 28, VGA_COLOR_CYAN);
    char sec_str[3] = "00";
    terminal_print_at("00", 42, 28, VGA_COLOR_WHITE);
    int second = 0;
    
    char time_display[32];
    small_snprintf(time_display, sizeof(time_display), "   Time set to: %02d:%02d:%02d", hour, minute, second);
    terminal_print_at(time_display, 20, 30, VGA_COLOR_GREEN);
    
    terminal_print_at("\n   Press Enter to continue...", 20, 32, VGA_COLOR_YELLOW);
    while (keyboard_getch() != '\n');
    
    // ===== 4. СОЗДАНИЕ АДМИНИСТРАТОРА =====
    terminal_print_at("\n[4/4] Create administrator account:", 15, 35, VGA_COLOR_YELLOW);
    
    // Просто создаем админа без ввода
    terminal_print_at("   Username: admin", 20, 37, VGA_COLOR_GREEN);
    terminal_print_at("   Password: admin", 20, 39, VGA_COLOR_GREEN);
    
    terminal_print_at("\n   Press Enter to create admin account...", 20, 41, VGA_COLOR_YELLOW);
    while (keyboard_getch() != '\n');
    
    // СОЗДАЁМ АККАУНТ АДМИНА
    Account* acc = &acct_sys.accounts[0];
    memset(acc, 0, sizeof(Account));
    
    strcpy(acc->username, "admin");
    
    // Соль и хеш для "admin"
    uint8_t salt[SALT_SIZE] = {0xAD, 0x4D, 0x1A, 0x00, 0xAD, 0x4D, 0x1A, 0x00};
    memcpy(acc->salt, salt, SALT_SIZE);
    strcpy(acc->password_hash, "1a1a1a");  // Простой хеш
    
    acc->account_type = ACCOUNT_ADMIN;
    small_snprintf(acc->home_dir, MAX_PATH - 1, "/home/admin");
    
    acct_sys.account_count = 1;
    acct_sys.current_account = 0;
    acct_sys.logged_in = 1;
    
    // СОЗДАЁМ ДОМАШНЮЮ ПАПКУ
    fs_cd("/");
    fs_cd("home");
    fs_mkdir("admin");
    fs_cd("admin");
    
    fs_mkdir("Documents");
    fs_mkdir("Projects");
    fs_mkdir("Downloads");
    fs_mkdir(".config");
    
    const char* readme = "Welcome to GooseOS!\nThis is your home directory.\n";
    fs_write("README.txt", (uint8_t*)readme, strlen(readme));
    
    fs_cd("/");
    
    // СОХРАНЯЕМ
    account_save_all();
    
    terminal_print_at("\n\n   ✓ Administrator account created!", 20, 44, VGA_COLOR_GREEN);
    terminal_print_at("\n\n   Press any key to continue...", 20, 46, VGA_COLOR_YELLOW);
    
    while (!keyboard_getch());
    vga_clear();
}

// ========== СОЗДАНИЕ АККАУНТА ==========
int account_create(const char* username, const char* password, uint8_t type) {
    if (acct_sys.account_count >= MAX_ACCOUNTS) return 0;
    
    // Проверяем, нет ли уже такого
    for (int i = 0; i < acct_sys.account_count; i++) {
        if (strcmp(acct_sys.accounts[i].username, username) == 0) {
            return 0;
        }
    }
    
    Account* acc = &acct_sys.accounts[acct_sys.account_count];
    memset(acc, 0, sizeof(Account));
    
    strcpy(acc->username, username);
    generate_salt(acc->salt);
    hash_password(password, acc->salt, acc->password_hash);
    acc->account_type = type;
    small_snprintf(acc->home_dir, MAX_PATH - 1, "/home/%s", username);
    
    // Создаём домашнюю папку
    fs_cd("/");
    fs_cd("home");
    fs_mkdir(username);
    fs_cd(username);
    
    fs_mkdir("Documents");
    fs_mkdir("Projects");
    fs_mkdir("Downloads");
    fs_mkdir(".config");
    
    fs_cd("/");
    
    acct_sys.account_count++;
    account_save_all();
    
    return 1;
}

// ========== ВХОД В СИСТЕМУ ==========
int account_login(const char* username, const char* password) {
    for (int i = 0; i < acct_sys.account_count; i++) {
        if (strcmp(acct_sys.accounts[i].username, username) == 0) {
            char hash[32];
            hash_password(password, acct_sys.accounts[i].salt, hash);
            
            // Отладка
            terminal_print("\nDebug: Login attempt\n", VGA_COLOR_YELLOW);
            terminal_print("  User: ", VGA_COLOR_YELLOW);
            terminal_print(username, VGA_COLOR_WHITE);
            terminal_print("\n  Input hash: ", VGA_COLOR_YELLOW);
            terminal_print(hash, VGA_COLOR_WHITE);
            terminal_print("\n  Stored hash: ", VGA_COLOR_YELLOW);
            terminal_print(acct_sys.accounts[i].password_hash, VGA_COLOR_WHITE);
            terminal_print("\n", VGA_COLOR_YELLOW);
            
            if (strcmp(hash, acct_sys.accounts[i].password_hash) == 0) {
                acct_sys.current_account = i;
                acct_sys.logged_in = 1;
                
                // Переходим в домашнюю директорию
                fs_cd(acct_sys.accounts[i].home_dir);
                
                // Сохраняем текущего пользователя
                char cur_str[16];
                small_snprintf(cur_str, sizeof(cur_str), "%d", i);
                fs_write("/etc/current.goose", (uint8_t*)cur_str, strlen(cur_str));
                
                terminal_print("  ✓ Login successful!\n", VGA_COLOR_GREEN);
                return 1;
            }
            break;
        }
    }
    
    terminal_print("  ✗ Login failed!\n", VGA_COLOR_RED);
    return 0;
}

void account_logout(void) {
    acct_sys.current_account = -1;
    acct_sys.logged_in = 0;
    fs_cd("/");
    
    // Обновляем current.goose
    fs_write("/etc/current.goose", (uint8_t*)"-1", 2);
}

// ========== МЕНЕДЖЕР ВХОДА ==========
void account_show_login_manager(void) {
    // Если нет аккаунтов - запускаем установку
    if (acct_sys.account_count == 0) {
        account_first_boot_setup();
        return;
    }
    
    // Если уже залогинен - пропускаем
    if (acct_sys.logged_in) return;
    
    vga_clear();
    
    // Синий фон
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_putchar(' ', vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE), x, y);
        }
    }
    
    terminal_print_at("╔════════════════════════════════════════════════╗", 15, 5, VGA_COLOR_YELLOW);
    terminal_print_at("║              GOOSE OS LOGIN                    ║", 15, 6, VGA_COLOR_WHITE);
    terminal_print_at("╚════════════════════════════════════════════════╝", 15, 7, VGA_COLOR_YELLOW);
    
    terminal_print_at("Select account:", 20, 10, VGA_COLOR_CYAN);
    
    // Показываем список аккаунтов
    for (int i = 0; i < acct_sys.account_count; i++) {
        char line[64];
        const char* type_str = "";
        
        if (acct_sys.accounts[i].account_type == ACCOUNT_ADMIN)
            type_str = "[ADMIN]";
        else if (acct_sys.accounts[i].account_type == ACCOUNT_GUEST)
            type_str = "[GUEST]";
        else
            type_str = "[USER]";
        
        small_snprintf(line, sizeof(line), "%d. %s %s", i+1, 
                      acct_sys.accounts[i].username, type_str);
        terminal_print_at(line, 25, 12 + i, VGA_COLOR_LIGHT_GRAY);
    }
    
    terminal_print_at("\nChoice (number): ", 20, 15 + acct_sys.account_count, VGA_COLOR_GREEN);
    
    int selected = -1;
    while (selected < 0 || selected >= acct_sys.account_count) {
        char key = keyboard_getch();
        if (!key) continue;
        
        if (key >= '1' && key <= '9') {
            int num = key - '0' - 1;
            if (num < acct_sys.account_count) {
                selected = num;
                vga_putchar(key, VGA_COLOR_WHITE, 38, 15 + acct_sys.account_count);
                break;
            }
        }
    }
    
    // Запрос пароля
    terminal_print_at("\nPassword: ", 20, 17 + acct_sys.account_count, VGA_COLOR_YELLOW);
    
    char password[MAX_PASSWORD] = {0};
    int p_pos = 0;
    int p_x = 31;  // Позиция для ввода
    
    while (1) {
        char key = keyboard_getch();
        if (!key) continue;
        
        if (key == '\n') {
            break;
        } else if (key == '\b' && p_pos > 0) {
            p_pos--;
            p_x--;
            vga_putchar(' ', VGA_COLOR_BLACK, p_x, 17 + acct_sys.account_count);
        } else if (p_pos < MAX_PASSWORD - 1 && key >= 32 && key <= 126) {
            password[p_pos++] = key;
            vga_putchar('*', VGA_COLOR_WHITE, p_x, 17 + acct_sys.account_count);
            p_x++;
        }
    }
    password[p_pos] = 0;
    
    // Пытаемся войти
    if (account_login(acct_sys.accounts[selected].username, password)) {
        terminal_print_at("\n\nLogin successful! Press any key...", 20, 19 + acct_sys.account_count, VGA_COLOR_GREEN);
    } else {
        terminal_print_at("\n\nWrong password! Press any key...", 20, 19 + acct_sys.account_count, VGA_COLOR_RED);
    }
    
    while (!keyboard_getch());
    vga_clear();
}

// ========== ПРАВА ДОСТУПА ==========
int account_check_permission(const char* path, uint8_t required) {
    (void)path;
    (void)required;
    
    // Админ может всё
    if (acct_sys.logged_in && acct_sys.current_account >= 0) {
        Account* acc = &acct_sys.accounts[acct_sys.current_account];
        if (acc->account_type == ACCOUNT_ADMIN) {
            return 1;
        }
    }
    
    // Для всех остальных - пока разрешаем
    return 1;
}

char* account_get_home_dir(void) {
    if (!acct_sys.logged_in || acct_sys.current_account < 0) {
        return NULL;
    }
    
    Account* acc = &acct_sys.accounts[acct_sys.current_account];
    
    if (acc->home_dir[0] == 0) {
        small_snprintf(acc->home_dir, MAX_PATH - 1, "/home/%s", acc->username);
    }
    
    return acc->home_dir;
}

int account_is_admin(void) {
    if (!acct_sys.logged_in || acct_sys.current_account < 0) return 0;
    return acct_sys.accounts[acct_sys.current_account].account_type == ACCOUNT_ADMIN;
}

int account_get_current_index(void) {
    return acct_sys.current_account;
}

// ========== СМЕНА ПАРОЛЯ ==========
int account_change_password(const char* username, const char* old_pass, const char* new_pass) {
    for (int i = 0; i < acct_sys.account_count; i++) {
        if (strcmp(acct_sys.accounts[i].username, username) == 0) {
            char old_hash[32];
            hash_password(old_pass, acct_sys.accounts[i].salt, old_hash);
            
            if (strcmp(old_hash, acct_sys.accounts[i].password_hash) == 0) {
                generate_salt(acct_sys.accounts[i].salt);
                hash_password(new_pass, acct_sys.accounts[i].salt, acct_sys.accounts[i].password_hash);
                account_save_all();
                return 1;
            }
            break;
        }
    }
    return 0;
}

// ========== УПРАВЛЕНИЕ ПОЛЬЗОВАТЕЛЯМИ ==========
void account_show_user_manager(void) {
    if (!acct_sys.logged_in) {
        terminal_print("Not logged in!\n", VGA_COLOR_RED);
        return;
    }
    
    Account* current = &acct_sys.accounts[acct_sys.current_account];
    
    if (current->account_type != ACCOUNT_ADMIN) {
        terminal_print("Only administrators can manage users!\n", VGA_COLOR_RED);
        return;
    }
    
    terminal_print("\n=== USER MANAGEMENT ===\n", VGA_COLOR_YELLOW);
    terminal_print("1. List users\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("2. Create user\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("3. Delete user\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("4. Change password\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("5. Set user type\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("0. Exit\n", VGA_COLOR_LIGHT_GRAY);
    terminal_print("Choice: ", VGA_COLOR_GREEN);
    
    char choice = keyboard_getch();
    terminal_putchar(choice);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    switch (choice) {
        case '1':
            terminal_print("\n=== USERS ===\n", VGA_COLOR_CYAN);
            for (int i = 0; i < acct_sys.account_count; i++) {
                char num[4];
                itoa(i + 1, num, 10);
                terminal_print("  ", VGA_COLOR_WHITE);
                terminal_print(num, VGA_COLOR_YELLOW);
                terminal_print(". ", VGA_COLOR_WHITE);
                terminal_print(acct_sys.accounts[i].username, VGA_COLOR_WHITE);
                terminal_print(" [", VGA_COLOR_LIGHT_GRAY);
                
                if (acct_sys.accounts[i].account_type == ACCOUNT_ADMIN)
                    terminal_print("ADMIN", VGA_COLOR_RED);
                else if (acct_sys.accounts[i].account_type == ACCOUNT_GUEST)
                    terminal_print("GUEST", VGA_COLOR_LIGHT_BLUE);
                else
                    terminal_print("USER", VGA_COLOR_GREEN);
                
                terminal_print("]\n", VGA_COLOR_LIGHT_GRAY);
            }
            break;
            
        case '2': {
            char username[MAX_USERNAME] = {0};
            char password[MAX_PASSWORD] = {0};
            int u_pos = 0, p_pos = 0;
            
            terminal_print("New username: ", VGA_COLOR_GREEN);
            while (1) {
                char key = keyboard_getch();
                if (key == '\n') break;
                if (key == '\b' && u_pos > 0) {
                    u_pos--;
                    terminal_print("\b \b", VGA_COLOR_WHITE);
                } else if (u_pos < MAX_USERNAME-1 && key >= 32 && key <= 126) {
                    username[u_pos++] = key;
                    terminal_putchar(key);
                }
            }
            username[u_pos] = 0;
            
            terminal_print("\nPassword: ", VGA_COLOR_GREEN);
            while (1) {
                char key = keyboard_getch();
                if (key == '\n') break;
                if (key == '\b' && p_pos > 0) {
                    p_pos--;
                    terminal_print("\b \b", VGA_COLOR_WHITE);
                } else if (p_pos < MAX_PASSWORD-1 && key >= 32 && key <= 126) {
                    password[p_pos++] = key;
                    terminal_print("*", VGA_COLOR_WHITE);
                }
            }
            password[p_pos] = 0;
            
            terminal_print("\nType (0=Guest, 1=User, 2=Admin): ", VGA_COLOR_GREEN);
            char type_key = keyboard_getch();
            terminal_putchar(type_key);
            int type = type_key - '0';
            if (type < 0 || type > 2) type = 1;
            
            if (account_create(username, password, type)) {
                terminal_print("\nUser created!\n", VGA_COLOR_GREEN);
            } else {
                terminal_print("\nFailed to create user!\n", VGA_COLOR_RED);
            }
            break;
        }
        
        case '3': {
            terminal_print("Username to delete: ", VGA_COLOR_RED);
            char del_user[MAX_USERNAME] = {0};
            int u_pos = 0;
            
            while (1) {
                char key = keyboard_getch();
                if (key == '\n') break;
                if (key == '\b' && u_pos > 0) {
                    u_pos--;
                    terminal_print("\b \b", VGA_COLOR_WHITE);
                } else if (u_pos < MAX_USERNAME-1 && key >= 32 && key <= 126) {
                    del_user[u_pos++] = key;
                    terminal_putchar(key);
                }
            }
            del_user[u_pos] = 0;
            
            if (strcmp(del_user, current->username) == 0) {
                terminal_print("\nCannot delete your own account!\n", VGA_COLOR_RED);
                break;
            }
            
            int del_idx = -1;
            for (int i = 0; i < acct_sys.account_count; i++) {
                if (strcmp(acct_sys.accounts[i].username, del_user) == 0) {
                    del_idx = i;
                    break;
                }
            }
            
            if (del_idx == -1) {
                terminal_print("\nUser not found!\n", VGA_COLOR_RED);
                break;
            }
            
            // Сдвигаем массив
            for (int i = del_idx; i < acct_sys.account_count - 1; i++) {
                acct_sys.accounts[i] = acct_sys.accounts[i + 1];
            }
            acct_sys.account_count--;
            
            // Пытаемся удалить домашнюю папку
            char home_path[256];
            small_snprintf(home_path, sizeof(home_path), "/home/%s", del_user);
            fs_rmdir(home_path);
            
            account_save_all();
            terminal_print("\nUser deleted!\n", VGA_COLOR_GREEN);
            break;
        }
        
        case '0':
            terminal_print("Exiting...\n", VGA_COLOR_YELLOW);
            break;
    }
}

void account_init(void) {
    // Заглушка

}
