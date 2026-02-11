#include "fs.h"
#include "diskfs.h"
#include "account.h"
#include "libc.h"
#include "terminal.h"
#include <string.h>

FileSystem fs;

// ========== ОСНОВНЫЕ ФУНКЦИИ ==========
void fs_init(void) {
    memset(&fs, 0, sizeof(FileSystem));
    
    if (diskfs_init()) {
        strcpy(fs.current_path, "/");
        fs.mounted = 1;
        terminal_print("✓ Filesystem ready\n", VGA_COLOR_GREEN);
    } else {
        terminal_print("⚠ Filesystem not available - using memory\n", VGA_COLOR_YELLOW);
        fs.mounted = 0;
    }
}

int fs_format(void) {
    terminal_print("Formatting disk...\n", VGA_COLOR_YELLOW);
    
    if (diskfs_format()) {
        fs.mounted = 1;
        strcpy(fs.current_path, "/");
        
        // Создаём структуру директорий для аккаунтов
        fs_cd("/");
        fs_mkdir("home");
        fs_mkdir("etc");
        
        return 1;
    }
    return 0;
}

void fs_info(void) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return;
    }
    diskfs_info();
}

// ========== ПРОВЕРКА ПРАВ ДОСТУПА ==========
static int fs_check_permission(const char* path, uint8_t required_op) {
    // Если система аккаунтов не инициализирована - разрешаем
    if (acct_sys.account_count == 0) return 1;
    
    // Если не залогинен - только чтение корня
    if (!acct_sys.logged_in) {
        if (strcmp(path, "/") == 0 || strcmp(path, "/etc") == 0 || 
            strcmp(path, "/home") == 0) {
            return (required_op == PERM_READ);
        }
        return 0;
    }
    
    return account_check_permission(path, required_op);
}

// ========== ФАЙЛОВЫЕ ОПЕРАЦИИ ==========
int fs_create(const char* name, const uint8_t* data, uint32_t size, uint8_t type) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на создание
    if (!fs_check_permission(name, PERM_WRITE)) {
        terminal_print("Permission denied: Cannot create file\n", VGA_COLOR_RED);
        return 0;
    }
    
    if (diskfs_create(name, type)) {
        if (type == FS_TYPE_FILE && data && size > 0) {
            return diskfs_write(name, data, size) > 0;
        }
        return 1;
    }
    return 0;
}

int fs_delete(const char* name) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на удаление
    if (!fs_check_permission(name, PERM_DELETE)) {
        terminal_print("Permission denied: Cannot delete\n", VGA_COLOR_RED);
        return 0;
    }
    
    return diskfs_delete(name);
}

int fs_read(const char* name, uint8_t* buffer, uint32_t size) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на чтение
    if (!fs_check_permission(name, PERM_READ)) {
        terminal_print("Permission denied: Cannot read\n", VGA_COLOR_RED);
        return 0;
    }
    
    return diskfs_read(name, buffer, size);
}

int fs_write(const char* name, const uint8_t* data, uint32_t size) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на запись
    if (!fs_check_permission(name, PERM_WRITE)) {
        terminal_print("Permission denied: Cannot write\n", VGA_COLOR_RED);
        return 0;
    }
    
    return diskfs_write(name, data, size);
}

int fs_exists(const char* name) {
    if (!fs.mounted) return 0;
    return diskfs_exists(name);
}

int fs_rename(const char* oldname, const char* newname) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на оба файла
    if (!fs_check_permission(oldname, PERM_DELETE) || 
        !fs_check_permission(newname, PERM_WRITE)) {
        terminal_print("Permission denied: Cannot rename\n", VGA_COLOR_RED);
        return 0;
    }
    
    return diskfs_rename(oldname, newname);
}

int fs_copy(const char* src, const char* dst) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права: чтение исходника, запись в назначение
    if (!fs_check_permission(src, PERM_READ) || 
        !fs_check_permission(dst, PERM_WRITE)) {
        terminal_print("Permission denied: Cannot copy\n", VGA_COLOR_RED);
        return 0;
    }
    
    return diskfs_copy(src, dst);
}

// ========== ДИРЕКТОРИИ ==========
int fs_cd(const char* path) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Особые случаи: ~ (домашняя директория)
    if (strcmp(path, "~") == 0 || strcmp(path, "~/") == 0) {
        if (acct_sys.logged_in && acct_sys.current_account >= 0) {
            char* home = account_get_home_dir();
            if (home) {
                int result = diskfs_cd(home);
                if (result) strcpy(fs.current_path, home);
                return result;
            }
        }
        return 0;
    }
    
    if (diskfs_cd(path)) {
        char* pwd = diskfs_pwd();
        if (pwd) strcpy(fs.current_path, pwd);
        return 1;
    }
    return 0;
}

int fs_mkdir(const char* name) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на создание директории
    if (!fs_check_permission(name, PERM_WRITE)) {
        terminal_print("Permission denied: Cannot create directory\n", VGA_COLOR_RED);
        return 0;
    }
    
    return diskfs_mkdir(name);
}

int fs_rmdir(const char* name) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на удаление директории
    if (!fs_check_permission(name, PERM_DELETE)) {
        terminal_print("Permission denied: Cannot remove directory\n", VGA_COLOR_RED);
        return 0;
    }
    
    return diskfs_rmdir(name);
}

char* fs_pwd(void) {
    return fs.current_path;
}

void fs_list(void) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return;
    }
    diskfs_list();
}

// ========== ДЛЯ РЕДАКТОРА ==========
int fs_editor_save(const char* name, const char* data, uint32_t size) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на запись
    if (!fs_check_permission(name, PERM_WRITE)) {
        terminal_print("Permission denied: Cannot save file\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем расширение .goo для GooseScript файлов
    const char* dot = strrchr(name, '.');
    if (dot && strcmp(dot, ".goo") == 0) {
        terminal_print("Saving GooseScript file...\n", VGA_COLOR_CYAN);
    }
    
    // Сохраняем файл
    int result = diskfs_write(name, (const uint8_t*)data, size);
    
    if (result > 0) {
        terminal_print("✓ Saved ", VGA_COLOR_GREEN);
        terminal_print(name, VGA_COLOR_WHITE);
        terminal_print(" (", VGA_COLOR_GREEN);
        char size_str[16];
        itoa(result, size_str, 10);
        terminal_print(size_str, VGA_COLOR_WHITE);
        terminal_print(" bytes)\n", VGA_COLOR_GREEN);
        return 1;
    }
    
    terminal_print("✗ Save failed\n", VGA_COLOR_RED);
    return 0;
}

int fs_editor_load(const char* name, char* buffer, uint32_t max_size) {
    if (!fs.mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем права на чтение
    if (!fs_check_permission(name, PERM_READ)) {
        terminal_print("Permission denied: Cannot read file\n", VGA_COLOR_RED);
        
        // Возвращаем пустой буфер для нового файла, но с ошибкой
        buffer[0] = 0;
        return 0;
    }
    
    int size = diskfs_read(name, (uint8_t*)buffer, max_size - 1);
    
    if (size > 0) {
        buffer[size] = 0; // Null terminator
        return size;
    }
    
    // Файл не существует - возвращаем пустой буфер для нового файла
    buffer[0] = 0;
    return 0;
}

uint32_t fs_get_file_size(const char* name) {
    if (!fs.mounted) return 0;
    
    // Проверяем права на чтение
    if (!fs_check_permission(name, PERM_READ)) {
        return 0;
    }
    
    // Простая заглушка - в реальности нужно из diskfs
    return 4096;
}

// ========== ДЛЯ VM И КОМПИЛЯТОРА ==========
int fs_save_goosebin(const char* source_name, const char* binary_name, 
                     const uint8_t* binary, uint32_t size) {
    if (!fs.mounted) return 0;
    
    (void)source_name;
    
    // Проверяем права на запись
    if (!fs_check_permission(binary_name, PERM_WRITE)) {
        terminal_print("Permission denied: Cannot save binary\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Сохраняем бинарник
    if (diskfs_write(binary_name, binary, size) > 0) {
        terminal_print("✓ Compiled to: ", VGA_COLOR_GREEN);
        terminal_print(binary_name, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_GREEN);
        return 1;
    }
    return 0;
}

int fs_load_goosebin(const char* name, uint8_t* buffer, uint32_t size) {
    if (!fs.mounted) return 0;
    
    // Проверяем права на чтение
    if (!fs_check_permission(name, PERM_READ)) {
        terminal_print("Permission denied: Cannot load binary\n", VGA_COLOR_RED);
        return 0;
    }
    
    return diskfs_read(name, buffer, size);
}

// ========== ФУНКЦИИ ДЛЯ РАБОТЫ С HOME ==========
int fs_chdir_home(void) {
    if (!acct_sys.logged_in || acct_sys.current_account < 0) {
        return 0;
    }
    
    char* home = account_get_home_dir();
    if (home) {
        return fs_cd(home);
    }
    return 0;
}

int fs_create_user_dirs(const char* username) {
    if (!fs.mounted) return 0;
    
    // Сохраняем текущий путь
    char old_path[256];
    strcpy(old_path, fs.current_path);
    
    // Создаём структуру /home/username
    fs_cd("/");
    if (!fs_exists("home")) {
        fs_mkdir("home");
    }
    
    fs_cd("home");
    
    char home_dir[64];
    sprintf(home_dir, "%s", username);
    
    if (!fs_exists(home_dir)) {
        fs_mkdir(home_dir);
    }
    
    // Создаём стандартные папки пользователя
    fs_cd(home_dir);
    if (!fs_exists("Documents")) fs_mkdir("Documents");
    if (!fs_exists("Projects")) fs_mkdir("Projects");
    if (!fs_exists(".config")) fs_mkdir(".config");
    
    // Возвращаемся
    fs_cd(old_path);
    
    return 1;
}

// ========== ДЛЯ ОТЛАДКИ ==========
void fs_print_permission_error(const char* op) {
    terminal_print("Permission denied: ", VGA_COLOR_RED);
    terminal_print(op, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_RED);
}