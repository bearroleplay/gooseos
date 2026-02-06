#include "fs.h"
#include "diskfs.h"
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
        // Можно добавить fallback на память
        fs.mounted = 0;
    }
}

int fs_format(void) {
    terminal_print("Formatting disk...\n", VGA_COLOR_YELLOW);
    
    if (diskfs_format()) {
        fs.mounted = 1;
        strcpy(fs.current_path, "/");
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

// Файловые операции
int fs_create(const char* name, const uint8_t* data, uint32_t size, uint8_t type) {
    if (!fs.mounted) return 0;
    
    if (diskfs_create(name, type)) {
        if (type == FS_TYPE_FILE && data && size > 0) {
            return diskfs_write(name, data, size) > 0;
        }
        return 1;
    }
    return 0;
}

int fs_delete(const char* name) {
    if (!fs.mounted) return 0;
    return diskfs_delete(name);
}

int fs_read(const char* name, uint8_t* buffer, uint32_t size) {
    if (!fs.mounted) return 0;
    return diskfs_read(name, buffer, size);
}

int fs_write(const char* name, const uint8_t* data, uint32_t size) {
    if (!fs.mounted) return 0;
    return diskfs_write(name, data, size);
}

int fs_exists(const char* name) {
    if (!fs.mounted) return 0;
    return diskfs_exists(name);
}

int fs_rename(const char* oldname, const char* newname) {
    if (!fs.mounted) return 0;
    return diskfs_rename(oldname, newname);
}

int fs_copy(const char* src, const char* dst) {
    if (!fs.mounted) return 0;
    return diskfs_copy(src, dst);
}

// Директории
int fs_cd(const char* path) {
    if (!fs.mounted) return 0;
    if (diskfs_cd(path)) {
        char* pwd = diskfs_pwd();
        if (pwd) strcpy(fs.current_path, pwd);
        return 1;
    }
    return 0;
}

int fs_mkdir(const char* name) {
    if (!fs.mounted) return 0;
    return diskfs_mkdir(name);
}

int fs_rmdir(const char* name) {
    if (!fs.mounted) return 0;
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
    if (!fs.mounted) return 0;
    
    // Проверяем расширение .goo для GooseScript файлов
    const char* dot = strrchr(name, '.');
    if (dot && strcmp(dot, ".goo") == 0) {
        terminal_print("Saving GooseScript file...\n", VGA_COLOR_CYAN);
    }
    
    // Сохраняем файл
    int result = diskfs_write(name, (const uint8_t*)data, size);
    
    if (result > 0) {
        char msg[64];
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
    if (!fs.mounted) return 0;
    
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
    
    // Читаем первые байты чтобы получить размер из директории
    uint8_t buffer[1];
    int size = diskfs_read(name, buffer, 1);
    return (size >= 0) ? 4096 : 0; // Заглушка - реальный размер нужно получить из inode
}

// ========== ДЛЯ VM И КОМПИЛЯТОРА ==========
int fs_save_goosebin(const char* source_name, const char* binary_name, 
                     const uint8_t* binary, uint32_t size) {
    if (!fs.mounted) return 0;
    
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
    return diskfs_read(name, buffer, size);
}