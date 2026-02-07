#ifndef _TYPES_H
#define _TYPES_H

/* ==================== БАЗОВЫЕ ТИПЫ ==================== */

/* Стандартные целочисленные типы фиксированного размера */
typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef signed short        int16_t;
typedef unsigned short      uint16_t;
typedef signed int          int32_t;
typedef unsigned int        uint32_t;
typedef signed long long    int64_t;
typedef unsigned long long  uint64_t;

/* Типы для указателей (зависит от архитектуры) */
#ifdef __i386__
typedef uint32_t uintptr_t;
typedef int32_t  intptr_t;
typedef uint32_t size_t;
typedef int32_t  ssize_t;
#else
typedef uint64_t uintptr_t;
typedef int64_t  intptr_t;
typedef uint64_t size_t;
typedef int64_t  ssize_t;
#endif

/* Логический тип */
typedef enum {
    false = 0,
    true = 1
} bool;

/* ==================== ТИПЫ ДЛЯ ЯДРА ==================== */

/* Статус операции */
typedef enum {
    STATUS_OK       = 0,
    STATUS_ERROR    = -1,
    STATUS_INVALID  = -2,
    STATUS_TIMEOUT  = -3,
    STATUS_BUSY     = -4,
    STATUS_EOF      = -5
} status_t;

/* Коды ошибок ядра */
typedef enum {
    ERR_NONE        = 0,
    ERR_MEMORY      = 1,
    ERR_IO          = 2,
    ERR_INVALID_ARG = 3,
    ERR_NOT_FOUND   = 4,
    ERR_PERMISSION  = 5,
    ERR_TIMEOUT     = 6,
    ERR_BUSY        = 7,
    ERR_NO_SPACE    = 8,
    ERR_NOT_SUPPORT = 9
} error_t;

/* ==================== СТРУКТУРЫ ЯДРА ==================== */

/* Дескриптор файла/устройства */
typedef struct {
    uint32_t id;
    uint32_t type;
    uint32_t flags;
    uint64_t position;
    uint32_t ref_count;
} handle_t;

/* Запись в каталоге */
typedef struct {
    char name[256];
    uint32_t inode;
    uint32_t type;
    uint64_t size;
    uint32_t permissions;
    uint64_t created;
    uint64_t modified;
} dir_entry_t;

/* Информация о системе */
typedef struct {
    uint32_t kernel_version;
    uint32_t total_memory;
    uint32_t free_memory;
    uint32_t tasks_count;
    uint32_t uptime;
    char hostname[64];
} sysinfo_t;

/* ==================== МАКРОСЫ ==================== */

/* Макросы для выравнивания */
#define ALIGN_UP(x, align)    (((x) + (align) - 1) & ~((align) - 1))
#define ALIGN_DOWN(x, align)  ((x) & ~((align) - 1))

/* Макросы для упаковки структур */
#define PACKED __attribute__((packed))

/* Проверка на NULL */
#define IS_NULL(ptr) ((ptr) == NULL)
#define NOT_NULL(ptr) ((ptr) != NULL)

/* Минимум/максимум */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Битовая манипуляция */
#define BIT(n) (1UL << (n))
#define SET_BIT(var, bit) ((var) |= BIT(bit))
#define CLEAR_BIT(var, bit) ((var) &= ~BIT(bit))
#define TOGGLE_BIT(var, bit) ((var) ^= BIT(bit))
#define TEST_BIT(var, bit) ((var) & BIT(bit))

/* ==================== КОНСТАНТЫ ==================== */

/* Размеры страниц памяти */
#define PAGE_SIZE_4K  4096
#define PAGE_SIZE_2M  2097152
#define PAGE_SIZE_1G  1073741824

/* Стандартные права доступа */
#define PERM_READ     0x01
#define PERM_WRITE    0x02
#define PERM_EXECUTE  0x04
#define PERM_ALL      0x07

/* Флаги открытия файлов */
#define O_READ        0x01
#define O_WRITE       0x02
#define O_CREATE      0x04
#define O_TRUNC       0x08
#define O_APPEND      0x10

#endif /* _TYPES_H */
