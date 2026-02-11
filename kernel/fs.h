#ifndef FS_H
#define FS_H

#include <stdint.h>

// Типы файлов (совместимость)
#define FS_TYPE_NONE 0
#define FS_TYPE_FILE 1
#define FS_TYPE_DIR  2

// Максимальные размеры
#define FS_MAX_NAME 56
#define FS_MAX_PATH 256

// Структура для совместимости
typedef struct {
    uint32_t current_dir;
    char current_path[FS_MAX_PATH];
    uint8_t mounted;
} FileSystem;

// Глобальная переменная
extern FileSystem fs;

// ========== ОСНОВНЫЕ ФУНКЦИИ ==========
void fs_init(void);
int fs_format(void);
void fs_info(void);

// Файловые операции
int fs_create(const char* name, const uint8_t* data, uint32_t size, uint8_t type);
int fs_delete(const char* name);
int fs_read(const char* name, uint8_t* buffer, uint32_t size);
int fs_write(const char* name, const uint8_t* data, uint32_t size);
int fs_exists(const char* name);
int fs_rename(const char* oldname, const char* newname);
int fs_copy(const char* src, const char* dst);

// Директории
int fs_cd(const char* path);
int fs_mkdir(const char* name);
int fs_rmdir(const char* name);
char* fs_pwd(void);
void fs_list(void);

// ========== ДЛЯ РЕДАКТОРА ==========
int fs_editor_save(const char* name, const char* data, uint32_t size);
int fs_editor_load(const char* name, char* buffer, uint32_t max_size);
uint32_t fs_get_file_size(const char* name);

// ========== ДЛЯ VM И КОМПИЛЯТОРА ==========
int fs_save_goosebin(const char* source_name, const char* binary_name, 
                     const uint8_t* binary, uint32_t size);
int fs_load_goosebin(const char* name, uint8_t* buffer, uint32_t size);

// ========== НОВЫЕ ФУНКЦИИ ДЛЯ АККАУНТОВ ==========
int fs_chdir_home(void);
int fs_create_user_dirs(const char* username);
void fs_print_permission_error(const char* op);

#endif