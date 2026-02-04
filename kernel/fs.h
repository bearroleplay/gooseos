#ifndef FS_H
#define FS_H

#include <stdint.h>

#define FS_MAX_FILES 128
#define FS_MAX_NAME 32
#define FS_MAX_PATH 128

typedef struct {
    char name[FS_MAX_NAME];
    uint32_t size;
    uint32_t offset;
    uint8_t type;        // 0=free, 1=file, 2=directory
    uint32_t parent;     // Индекс родительской папки
} FileEntry;

typedef struct {
    FileEntry entries[FS_MAX_FILES];
    uint32_t file_count;
    uint8_t* data;
    uint32_t data_size;
    uint32_t current_dir; // Индекс текущей папки
    char current_path[FS_MAX_PATH];
} FileSystem;

// Функции
void fs_init(void);
int fs_create(const char* name, const uint8_t* data, uint32_t size, uint8_t type);
int fs_read(const char* name, uint8_t* buffer, uint32_t size);
int fs_exists(const char* name);
void fs_list(void);

// Функции для папок
int fs_cd(const char* path);
int fs_mkdir(const char* name);
int fs_delete(const char* name);
int fs_rmdir(const char* name);
char* fs_pwd(void);

#endif