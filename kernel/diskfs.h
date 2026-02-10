#ifndef DISKFS_H
#define DISKFS_H

#include <stdint.h>

// Константы ФС
#define DISKFS_MAGIC       0x474F4F53  // "GOOS"
#define DISKFS_VERSION     1
#define DISKFS_BLOCK_SIZE  512
#define DISKFS_MAX_FILES   100
#define DISKFS_FILENAME_LEN 56

// Типы файлов
#define DISKFS_TYPE_FREE   0
#define DISKFS_TYPE_FILE   1
#define DISKFS_TYPE_DIR    2

// Структура inode в памяти
typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t blocks[12];
    char name[DISKFS_FILENAME_LEN];
} DiskInode;

// Запись в каталоге
typedef struct {
    uint32_t inode;
    char name[DISKFS_FILENAME_LEN];
    uint32_t type;
    uint32_t size;
} DiskDirEntry;

// Основные функции ФС
int diskfs_init(void);
int diskfs_format(void);  // БЕЗ ПАРАМЕТРОВ!
void diskfs_info(void);

// Файловые операции
int diskfs_create(const char* name, uint8_t type);
int diskfs_delete(const char* name);
int diskfs_read(const char* name, uint8_t* buffer, uint32_t size);
int diskfs_write(const char* name, const uint8_t* data, uint32_t size);
int diskfs_exists(const char* name);
int diskfs_rename(const char* oldname, const char* newname);
int diskfs_copy(const char* src, const char* dst);

// Операции с каталогами
int diskfs_cd(const char* path);
int diskfs_mkdir(const char* name);
int diskfs_rmdir(const char* name);
char* diskfs_pwd(void);
void diskfs_list(void);

// Информация
uint32_t diskfs_free_space(void);

#endif