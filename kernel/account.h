#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stdint.h>

// Константы для аккаунтов - ВСЁ В ОДНОМ МЕСТЕ
#define MAX_USERNAME     32
#define MAX_PASSWORD     32
#define MAX_ACCOUNTS     10
#define SALT_SIZE        8
#define MAX_PATH         256     // ← ЭТО БЫЛО ПРОПУЩЕНО!

// Права доступа
#define PERM_READ        0x01
#define PERM_WRITE       0x02
#define PERM_DELETE      0x04
#define PERM_ADMIN       0x80

// Типы учётных записей
#define ACCOUNT_GUEST    0
#define ACCOUNT_USER     1
#define ACCOUNT_ADMIN    2

typedef struct {
    char username[MAX_USERNAME];
    char password_hash[32];     // Простой XOR-хеш (для демо)
    uint8_t salt[SALT_SIZE];
    uint8_t account_type;       // 0=гость, 1=пользователь, 2=админ
    uint32_t flags;
    uint32_t created_time;
    char home_dir[MAX_PATH];    // /home/username/   ← ТУТ ИСПОЛЬЗУЕТСЯ MAX_PATH
} Account;

typedef struct {
    Account accounts[MAX_ACCOUNTS];
    int account_count;
    int current_account;        // индекс текущего пользователя
    int logged_in;
    char system_timezone[32];
    uint8_t boot_counter;       // 1 = первый запуск
} AccountSystem;

// Глобальная переменная
extern AccountSystem acct_sys;

// ========== ИНИЦИАЛИЗАЦИЯ ==========
void account_init(void);
void account_first_boot_setup(void);
int account_load_all(void);
int account_save_all(void);

// ========== УПРАВЛЕНИЕ АККАУНТАМИ ==========
int account_create(const char* username, const char* password, uint8_t type);
int account_delete(const char* username);
int account_login(const char* username, const char* password);
void account_logout(void);
int account_change_password(const char* username, const char* old_pass, const char* new_pass);

// ========== ПРАВА ДОСТУПА ==========
int account_check_permission(const char* path, uint8_t required);
char* account_get_home_dir(void);
int account_is_admin(void);
int account_get_current_index(void);

// ========== МЕНЕДЖЕР ВХОДА ==========
void account_show_login_manager(void);
void account_show_setup_wizard(void);
void account_show_user_manager(void);  // setuser command

// ========== ВРЕМЯ И ЧАСОВОЙ ПОЯС ==========
void account_set_timezone(const char* tz);
char* account_get_timezone(void);
void account_apply_timezone(void);  // коррекция времени

#endif