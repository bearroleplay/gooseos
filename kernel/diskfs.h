#ifndef DISKFS_H
#define DISKFS_H

#include <stdint.h>

// Константы
#define DISKFS_MAGIC       0x474F4F53  // "GOOS"
#define DISKFS_VERSION     1
#define DISKFS_BLOCK_SIZE  512
#define DISKFS_INODE_COUNT 256
#define DISKFS_FILENAME_LEN 56
#define DISKFS_MAX_BLOCKS  4096        // ~2MB диска

// Типы файлов
#define DISKFS_TYPE_FREE   0
#define DISKFS_TYPE_FILE   1
#define DISKFS_TYPE_DIR    2

// Структуры на диске
#pragma pack(push, 1)

// Суперблок (512 байт)
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t block_size;
    uint32_t inode_count;
    uint32_t data_blocks;
    uint32_t free_inodes;
    uint32_t free_blocks;
    uint32_t root_inode;
    uint32_t checksum;
    char label[32];
    uint8_t reserved[464];
} DiskSuperblock;

// Inode (128 байт)
typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t blocks[12];      // Прямые блоки
    uint32_t parent;
    uint32_t ctime;
    uint32_t mtime;
    char name[DISKFS_FILENAME_LEN];
    uint8_t reserved[28];
} DiskInode;

// Запись в каталоге (64 байта)
typedef struct {
    uint32_t inode;
    char name[DISKFS_FILENAME_LEN];
    uint32_t type;
    uint32_t size;
    uint8_t reserved[12];
} DiskDirEntry;

#pragma pack(pop)

// Функции файловой системы
int diskfs_init(void);
int diskfs_format(void);
int diskfs_mount(void);
int diskfs_umount(void);

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
int diskfs_get_entries(DiskDirEntry* entries, int max);

// Информация
void diskfs_info(void);
uint32_t diskfs_free_space(void);

#endif