#include "diskfs.h"
#include "ata.h"
#include "libc.h"
#include "terminal.h"
#include <string.h>

// Глобальные переменные
static DiskSuperblock super;
static uint8_t block_bitmap[512];  // Битовая карта блоков (512 байт = 4096 бит)
static uint8_t inode_bitmap[32];   // Битовая карта inode (32 байта = 256 бит)
static DiskInode current_inode;    // Текущий inode (кэширован)
static uint32_t current_dir = 1;   // Текущая директория (inode 1 = корень)
static char current_path[256] = "/";

// Вспомогательные функции
static int read_block(uint32_t block, uint8_t* buffer) {
    return ata_read_sectors(block, buffer, 1);
}

static int write_block(uint32_t block, const uint8_t* buffer) {
    return ata_write_sectors(block, buffer, 1);
}

static int read_inode(uint32_t inode_num, DiskInode* inode) {
    if (inode_num == 0 || inode_num >= DISKFS_INODE_COUNT) return 0;
    
    // Inode хранятся в блоках 10-41 (32 блока * 4 inode/block = 128 inode)
    uint32_t block_num = 10 + (inode_num / 4);
    uint32_t offset = (inode_num % 4) * sizeof(DiskInode);
    
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(block_num, block)) return 0;
    
    memcpy(inode, block + offset, sizeof(DiskInode));
    return 1;
}

static int write_inode(uint32_t inode_num, const DiskInode* inode) {
    if (inode_num == 0 || inode_num >= DISKFS_INODE_COUNT) return 0;
    
    uint32_t block_num = 10 + (inode_num / 4);
    uint32_t offset = (inode_num % 4) * sizeof(DiskInode);
    
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(block_num, block)) return 0;
    
    memcpy(block + offset, inode, sizeof(DiskInode));
    return write_block(block_num, block);
}

static uint32_t find_free_inode(void) {
    for (uint32_t i = 1; i < DISKFS_INODE_COUNT; i++) {
        if (!(inode_bitmap[i / 8] & (1 << (i % 8)))) {
            inode_bitmap[i / 8] |= (1 << (i % 8));
            super.free_inodes--;
            return i;
        }
    }
    return 0;
}

static uint32_t find_free_block(void) {
    for (uint32_t i = 0; i < super.data_blocks; i++) {
        if (!(block_bitmap[i / 8] & (1 << (i % 8)))) {
            block_bitmap[i / 8] |= (1 << (i % 8));
            super.free_blocks--;
            return i + 100; // Блоки данных начинаются с 100
        }
    }
    return 0;
}

static void free_block(uint32_t block) {
    if (block >= 100) {
        uint32_t index = block - 100;
        block_bitmap[index / 8] &= ~(1 << (index % 8));
        super.free_blocks++;
    }
}

static void free_inode(uint32_t inode) {
    if (inode > 0 && inode < DISKFS_INODE_COUNT) {
        inode_bitmap[inode / 8] &= ~(1 << (inode % 8));
        super.free_inodes++;
    }
}

static int save_metadata(void) {
    // Суперблок в секторе 0
    if (write_block(0, (uint8_t*)&super)) return 0;
    
    // Битовая карта блоков в секторе 1
    if (write_block(1, block_bitmap)) return 0;
    
    // Битовая карта inode в секторе 2
    if (write_block(2, inode_bitmap)) return 0;
    
    return 1;
}

static int load_metadata(void) {
    // Читаем суперблок
    if (read_block(0, (uint8_t*)&super)) return 0;
    
    // Проверяем магическое число
    if (super.magic != DISKFS_MAGIC) {
        return 0; // Файловая система не отформатирована
    }
    
    // Читаем битовые карты
    if (read_block(1, block_bitmap)) return 0;
    if (read_block(2, inode_bitmap)) return 0;
    
    return 1;
}

static void update_path(void) {
    if (current_dir == 1) {
        strcpy(current_path, "/");
        return;
    }
    
    // Собираем путь от корня
    char temp_path[256] = "";
    uint32_t dir = current_dir;
    
    while (dir != 1) { // Пока не корень
        DiskInode dir_inode;
        if (!read_inode(dir, &dir_inode)) break;
        
        char new_path[256];
        strcpy(new_path, "/");
        strcat(new_path, dir_inode.name);
        strcat(new_path, temp_path);
        strcpy(temp_path, new_path);
        
        dir = dir_inode.parent;
    }
    
    strcpy(current_path, temp_path);
    if (current_path[0] == 0) strcpy(current_path, "/");
}

// Основные функции
int diskfs_init(void) {
    // Инициализируем ATA
    terminal_print("Initializing ATA... ", VGA_COLOR_CYAN);
    if (ata_init()) {
        terminal_print("FAILED\n", VGA_COLOR_RED);
        return 0;
    }
    terminal_print("OK\n", VGA_COLOR_GREEN);
    
    // Пытаемся смонтировать существующую ФС
    terminal_print("Mounting filesystem... ", VGA_COLOR_CYAN);
    if (diskfs_mount()) {
        terminal_print("MOUNTED\n", VGA_COLOR_GREEN);
        read_inode(current_dir, &current_inode);
        return 1;
    }
    
    terminal_print("NOT FOUND\n", VGA_COLOR_YELLOW);
    return 0;
}

int diskfs_format(void) {
    terminal_print("Formatting disk...\n", VGA_COLOR_YELLOW);
    
    // Инициализируем суперблок
    memset(&super, 0, sizeof(DiskSuperblock));
    super.magic = DISKFS_MAGIC;
    super.version = DISKFS_VERSION;
    super.block_size = DISKFS_BLOCK_SIZE;
    super.inode_count = DISKFS_INODE_COUNT;
    super.data_blocks = DISKFS_MAX_BLOCKS;
    super.free_inodes = DISKFS_INODE_COUNT - 1;
    super.free_blocks = DISKFS_MAX_BLOCKS - 100; // Минус системные блоки
    super.root_inode = 1;
    strcpy(super.label, "GooseOS Disk");
    
    // Инициализируем битовые карты
    memset(block_bitmap, 0, sizeof(block_bitmap));
    memset(inode_bitmap, 0, sizeof(inode_bitmap));
    
    // Помечаем системные блоки как занятые (0-99)
    for (int i = 0; i < 100; i++) {
        block_bitmap[i / 8] |= (1 << (i % 8));
    }
    
    // Помечаем inode 0 как занятый
    inode_bitmap[0] |= 1;
    
    // Создаем корневой inode
    DiskInode root;
    memset(&root, 0, sizeof(DiskInode));
    root.type = DISKFS_TYPE_DIR;
    strcpy(root.name, "/");
    root.parent = 1; // Сам себе родитель
    
    // Выделяем блок для корневого каталога
    uint32_t root_block = find_free_block();
    if (!root_block) {
        terminal_print("Format: No free blocks for root\n", VGA_COLOR_RED);
        return 0;
    }
    root.blocks[0] = root_block;
    
    // Создаем записи . и ..
    DiskDirEntry entries[8];
    memset(entries, 0, sizeof(entries));
    
    entries[0].inode = 1;
    strcpy(entries[0].name, ".");
    entries[0].type = DISKFS_TYPE_DIR;
    
    entries[1].inode = 1;
    strcpy(entries[1].name, "..");
    entries[1].type = DISKFS_TYPE_DIR;
    
    // Записываем корневой каталог
    uint8_t block[DISKFS_BLOCK_SIZE];
    memcpy(block, entries, sizeof(entries));
    if (write_block(root_block, block)) {
        terminal_print("Format: Failed to write root dir\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Сохраняем корневой inode
    inode_bitmap[0] |= (1 << 1); // Inode 1 занят
    super.free_inodes--;
    
    if (!write_inode(1, &root)) {
        terminal_print("Format: Failed to write root inode\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Сохраняем метаданные
    if (!save_metadata()) {
        terminal_print("Format: Failed to save metadata\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Устанавливаем текущий каталог
    current_dir = 1;
    strcpy(current_path, "/");
    memcpy(&current_inode, &root, sizeof(DiskInode));
    
    terminal_print("Disk formatted successfully!\n", VGA_COLOR_GREEN);
    return 1;
}

int diskfs_mount(void) {
    if (!load_metadata()) return 0;
    
    // Читаем корневой inode
    if (!read_inode(super.root_inode, &current_inode)) return 0;
    
    current_dir = super.root_inode;
    strcpy(current_path, "/");
    
    return 1;
}

int diskfs_umount(void) {
    return save_metadata();
}

// Файловые операции
int diskfs_create(const char* name, uint8_t type) {
    if (!name || name[0] == 0) return 0;
    
    // Читаем текущий каталог
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], block)) return 0;
    
    DiskDirEntry* entries = (DiskDirEntry*)block;
    
    // Проверяем, нет ли уже файла с таким именем
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, name) == 0) {
            return 0; // Уже существует
        }
    }
    
    // Находим свободный inode
    uint32_t new_inode_num = find_free_inode();
    if (!new_inode_num) {
        terminal_print("No free inodes\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Создаем новый inode
    DiskInode new_inode;
    memset(&new_inode, 0, sizeof(DiskInode));
    new_inode.type = type;
    new_inode.parent = current_dir;
    strncpy(new_inode.name, name, DISKFS_FILENAME_LEN - 1);
    new_inode.name[DISKFS_FILENAME_LEN - 1] = 0;
    
    // Для директории создаем блок с . и ..
    if (type == DISKFS_TYPE_DIR) {
        uint32_t dir_block = find_free_block();
        if (!dir_block) {
            free_inode(new_inode_num);
            terminal_print("No free blocks for directory\n", VGA_COLOR_RED);
            return 0;
        }
        new_inode.blocks[0] = dir_block;
        
        DiskDirEntry dir_entries[8];
        memset(dir_entries, 0, sizeof(dir_entries));
        
        dir_entries[0].inode = new_inode_num;
        strcpy(dir_entries[0].name, ".");
        dir_entries[0].type = DISKFS_TYPE_DIR;
        
        dir_entries[1].inode = current_dir;
        strcpy(dir_entries[1].name, "..");
        dir_entries[1].type = DISKFS_TYPE_DIR;
        
        uint8_t dir_block_data[DISKFS_BLOCK_SIZE];
        memcpy(dir_block_data, dir_entries, sizeof(dir_entries));
        
        if (write_block(dir_block, dir_block_data)) {
            free_block(dir_block);
            free_inode(new_inode_num);
            terminal_print("Failed to write directory block\n", VGA_COLOR_RED);
            return 0;
        }
    }
    
    // Сохраняем inode
    if (!write_inode(new_inode_num, &new_inode)) {
        if (type == DISKFS_TYPE_DIR) free_block(new_inode.blocks[0]);
        free_inode(new_inode_num);
        terminal_print("Failed to write inode\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Добавляем запись в текущий каталог
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode == 0) {
            entries[i].inode = new_inode_num;
            strncpy(entries[i].name, name, DISKFS_FILENAME_LEN - 1);
            entries[i].name[DISKFS_FILENAME_LEN - 1] = 0;
            entries[i].type = type;
            entries[i].size = 0;
            
            if (write_block(current_inode.blocks[0], block)) {
                free_inode(new_inode_num);
                if (type == DISKFS_TYPE_DIR) free_block(new_inode.blocks[0]);
                terminal_print("Failed to update directory\n", VGA_COLOR_RED);
                return 0;
            }
            
            save_metadata();
            return 1;
        }
    }
    
    // Нет места в каталоге
    free_inode(new_inode_num);
    if (type == DISKFS_TYPE_DIR) free_block(new_inode.blocks[0]);
    terminal_print("Directory full\n", VGA_COLOR_RED);
    return 0;
}

int diskfs_delete(const char* name) {
    // Читаем текущий каталог
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], block)) return 0;
    
    DiskDirEntry* entries = (DiskDirEntry*)block;
    uint32_t target_inode = 0;
    int entry_index = -1;
    
    // Ищем запись
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, name) == 0) {
            target_inode = entries[i].inode;
            entry_index = i;
            break;
        }
    }
    
    if (!target_inode) return 0;
    
    // Читаем inode
    DiskInode target;
    if (!read_inode(target_inode, &target)) return 0;
    
    // Если это каталог, проверяем что он пуст (кроме . и ..)
    if (target.type == DISKFS_TYPE_DIR) {
        // Читаем содержимое каталога
        uint8_t dir_block[DISKFS_BLOCK_SIZE];
        if (read_block(target.blocks[0], dir_block)) return 0;
        
        DiskDirEntry* dir_entries = (DiskDirEntry*)dir_block;
        for (int i = 2; i < 8; i++) {
            if (dir_entries[i].inode != 0) {
                terminal_print("Directory not empty\n", VGA_COLOR_RED);
                return 0;
            }
        }
        
        // Освобождаем блок каталога
        free_block(target.blocks[0]);
    } 
    // Если это файл, освобождаем все блоки
    else if (target.type == DISKFS_TYPE_FILE) {
        for (int i = 0; i < 12; i++) {
            if (target.blocks[i]) {
                free_block(target.blocks[i]);
            }
        }
    }
    
    // Освобождаем inode
    free_inode(target_inode);
    
    // Удаляем запись из каталога
    memset(&entries[entry_index], 0, sizeof(DiskDirEntry));
    
    // Сохраняем каталог
    if (write_block(current_inode.blocks[0], block)) return 0;
    
    // Сохраняем метаданные
    save_metadata();
    
    return 1;
}

int diskfs_read(const char* name, uint8_t* buffer, uint32_t size) {
    // Ищем файл в текущем каталоге
    uint8_t dir_block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], dir_block)) return 0;
    
    DiskDirEntry* entries = (DiskDirEntry*)dir_block;
    uint32_t target_inode = 0;
    
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0 && 
            entries[i].type == DISKFS_TYPE_FILE &&
            strcmp(entries[i].name, name) == 0) {
            target_inode = entries[i].inode;
            break;
        }
    }
    
    if (!target_inode) return 0;
    
    // Читаем inode файла
    DiskInode file_inode;
    if (!read_inode(target_inode, &file_inode)) return 0;
    
    uint32_t to_read = file_inode.size;
    if (to_read > size) to_read = size;
    
    uint32_t read_bytes = 0;
    for (int i = 0; i < 12 && read_bytes < to_read; i++) {
        if (file_inode.blocks[i]) {
            uint8_t data_block[DISKFS_BLOCK_SIZE];
            if (read_block(file_inode.blocks[i], data_block)) break;
            
            uint32_t block_read = to_read - read_bytes;
            if (block_read > DISKFS_BLOCK_SIZE) block_read = DISKFS_BLOCK_SIZE;
            
            memcpy(buffer + read_bytes, data_block, block_read);
            read_bytes += block_read;
        }
    }
    
    return read_bytes;
}

int diskfs_write(const char* name, const uint8_t* data, uint32_t size) {
    // Ищем существующий файл
    uint8_t dir_block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], dir_block)) return 0;
    
    DiskDirEntry* entries = (DiskDirEntry*)dir_block;
    uint32_t target_inode = 0;
    int entry_index = -1;
    
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0 && 
            entries[i].type == DISKFS_TYPE_FILE &&
            strcmp(entries[i].name, name) == 0) {
            target_inode = entries[i].inode;
            entry_index = i;
            break;
        }
    }
    
    // Если файла нет, создаем
    if (!target_inode) {
        if (!diskfs_create(name, DISKFS_TYPE_FILE)) return 0;
        
        // Ищем снова
        read_block(current_inode.blocks[0], dir_block);
        entries = (DiskDirEntry*)dir_block;
        
        for (int i = 0; i < 8; i++) {
            if (entries[i].inode != 0 && strcmp(entries[i].name, name) == 0) {
                target_inode = entries[i].inode;
                entry_index = i;
                break;
            }
        }
        
        if (!target_inode) return 0;
    }
    
    // Читаем существующий inode
    DiskInode file_inode;
    if (!read_inode(target_inode, &file_inode)) return 0;
    
    // Освобождаем старые блоки
    for (int i = 0; i < 12; i++) {
        if (file_inode.blocks[i]) {
            free_block(file_inode.blocks[i]);
            file_inode.blocks[i] = 0;
        }
    }
    
    // Выделяем новые блоки
    uint32_t blocks_needed = (size + DISKFS_BLOCK_SIZE - 1) / DISKFS_BLOCK_SIZE;
    if (blocks_needed > 12) blocks_needed = 12;
    
    uint32_t written = 0;
    for (int i = 0; i < blocks_needed; i++) {
        uint32_t block = find_free_block();
        if (!block) break;
        
        file_inode.blocks[i] = block;
        
        uint32_t to_write = size - written;
        if (to_write > DISKFS_BLOCK_SIZE) to_write = DISKFS_BLOCK_SIZE;
        
        uint8_t block_data[DISKFS_BLOCK_SIZE];
        memset(block_data, 0, DISKFS_BLOCK_SIZE);
        memcpy(block_data, data + written, to_write);
        
        if (write_block(block, block_data)) {
            free_block(block);
            file_inode.blocks[i] = 0;
            break;
        }
        
        written += to_write;
    }
    
    file_inode.size = written;
    
    // Сохраняем inode
    if (!write_inode(target_inode, &file_inode)) {
        // Откатываем
        for (int i = 0; i < 12; i++) {
            if (file_inode.blocks[i]) free_block(file_inode.blocks[i]);
        }
        return 0;
    }
    
    // Обновляем запись в каталоге
    if (entry_index >= 0) {
        entries[entry_index].size = written;
        write_block(current_inode.blocks[0], dir_block);
    }
    
    // Сохраняем метаданные
    save_metadata();
    
    return written;
}

int diskfs_exists(const char* name) {
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], block)) return 0;
    
    DiskDirEntry* entries = (DiskDirEntry*)block;
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

int diskfs_rename(const char* oldname, const char* newname) {
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], block)) return 0;
    
    DiskDirEntry* entries = (DiskDirEntry*)block;
    
    // Проверяем что новое имя не занято
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, newname) == 0) {
            return 0; // Имя уже существует
        }
    }
    
    // Ищем старое имя
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0 && strcmp(entries[i].name, oldname) == 0) {
            // Меняем имя
            strncpy(entries[i].name, newname, DISKFS_FILENAME_LEN - 1);
            entries[i].name[DISKFS_FILENAME_LEN - 1] = 0;
            
            // Обновляем имя в inode
            DiskInode file_inode;
            if (!read_inode(entries[i].inode, &file_inode)) return 0;
            
            strncpy(file_inode.name, newname, DISKFS_FILENAME_LEN - 1);
            file_inode.name[DISKFS_FILENAME_LEN - 1] = 0;
            
            if (!write_inode(entries[i].inode, &file_inode)) return 0;
            
            // Сохраняем каталог
            if (write_block(current_inode.blocks[0], block)) return 0;
            
            save_metadata();
            return 1;
        }
    }
    
    return 0;
}

int diskfs_copy(const char* src, const char* dst) {
    uint8_t buffer[DISKFS_BLOCK_SIZE * 12]; // Максимальный размер файла
    int size = diskfs_read(src, buffer, sizeof(buffer));
    if (size <= 0) return 0;
    
    return diskfs_write(dst, buffer, size) > 0;
}

// Операции с каталогами
int diskfs_cd(const char* path) {
    if (!path) return 0;
    
    if (strcmp(path, "/") == 0) {
        current_dir = 1;
        read_inode(1, &current_inode);
        strcpy(current_path, "/");
        return 1;
    }
    
    if (strcmp(path, "..") == 0) {
        if (current_dir == 1) return 1; // Уже в корне
        
        current_dir = current_inode.parent;
        read_inode(current_dir, &current_inode);
        update_path();
        return 1;
    }
    
    if (strcmp(path, ".") == 0) {
        return 1; // Уже в текущей
    }
    
    // Ищем подкаталог в текущем каталоге
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], block)) return 0;
    
    DiskDirEntry* entries = (DiskDirEntry*)block;
    
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0 && 
            entries[i].type == DISKFS_TYPE_DIR &&
            strcmp(entries[i].name, path) == 0) {
            
            current_dir = entries[i].inode;
            read_inode(current_dir, &current_inode);
            update_path();
            return 1;
        }
    }
    
    return 0;
}

int diskfs_mkdir(const char* name) {
    return diskfs_create(name, DISKFS_TYPE_DIR);
}

int diskfs_rmdir(const char* name) {
    return diskfs_delete(name); // delete уже проверяет пустоту
}

char* diskfs_pwd(void) {
    return current_path;
}

void diskfs_list(void) {
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], block)) {
        terminal_print("Error reading directory\n", VGA_COLOR_RED);
        return;
    }
    
    DiskDirEntry* entries = (DiskDirEntry*)block;
    
    terminal_print("\n=== ", VGA_COLOR_CYAN);
    terminal_print(current_path, VGA_COLOR_WHITE);
    terminal_print(" ===\n", VGA_COLOR_CYAN);
    
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (entries[i].inode != 0) {
            if (entries[i].type == DISKFS_TYPE_DIR) {
                terminal_print("  [DIR]  ", VGA_COLOR_LIGHT_BLUE);
            } else {
                terminal_print("         ", VGA_COLOR_LIGHT_GRAY);
            }
            
            terminal_print(entries[i].name, VGA_COLOR_WHITE);
            
            if (entries[i].type == DISKFS_TYPE_FILE) {
                char size_str[32];
                terminal_print(" (", VGA_COLOR_LIGHT_GRAY);
                itoa(entries[i].size, size_str, 10);
                terminal_print(size_str, VGA_COLOR_LIGHT_GREEN);
                terminal_print(" bytes)", VGA_COLOR_LIGHT_GRAY);
            }
            
            terminal_print("\n", VGA_COLOR_WHITE);
            count++;
        }
    }
    
    if (count == 0) {
        terminal_print("  Empty directory\n", VGA_COLOR_LIGHT_GRAY);
    }
    
    char info[64];
    terminal_print("\nTotal: ", VGA_COLOR_LIGHT_GRAY);
    itoa(count, info, 10);
    terminal_print(info, VGA_COLOR_GREEN);
    terminal_print(" items\n", VGA_COLOR_LIGHT_GRAY);
}

int diskfs_get_entries(DiskDirEntry* entries, int max) {
    uint8_t block[DISKFS_BLOCK_SIZE];
    if (read_block(current_inode.blocks[0], block)) return 0;
    
    DiskDirEntry* dir_entries = (DiskDirEntry*)block;
    int count = 0;
    
    for (int i = 0; i < 8 && count < max; i++) {
        if (dir_entries[i].inode != 0) {
            memcpy(&entries[count], &dir_entries[i], sizeof(DiskDirEntry));
            count++;
        }
    }
    
    return count;
}

// Информация
void diskfs_info(void) {
    terminal_print("\n=== Disk Filesystem Information ===\n", VGA_COLOR_YELLOW);
    
    char buffer[64];
    
    terminal_print("Label: ", VGA_COLOR_CYAN);
    terminal_print(super.label, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    terminal_print("Version: ", VGA_COLOR_CYAN);
    itoa(super.version, buffer, 10);
    terminal_print(buffer, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    terminal_print("Block size: ", VGA_COLOR_CYAN);
    itoa(super.block_size, buffer, 10);
    terminal_print(buffer, VGA_COLOR_WHITE);
    terminal_print(" bytes\n", VGA_COLOR_WHITE);
    
    terminal_print("Total inodes: ", VGA_COLOR_CYAN);
    itoa(super.inode_count, buffer, 10);
    terminal_print(buffer, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    terminal_print("Free inodes: ", VGA_COLOR_CYAN);
    itoa(super.free_inodes, buffer, 10);
    terminal_print(buffer, VGA_COLOR_GREEN);
    terminal_print("\n", VGA_COLOR_GREEN);
    
    terminal_print("Total blocks: ", VGA_COLOR_CYAN);
    itoa(super.data_blocks, buffer, 10);
    terminal_print(buffer, VGA_COLOR_WHITE);
    terminal_print(" (", VGA_COLOR_WHITE);
    itoa(super.data_blocks * DISKFS_BLOCK_SIZE / 1024, buffer, 10);
    terminal_print(buffer, VGA_COLOR_WHITE);
    terminal_print(" KB)\n", VGA_COLOR_WHITE);
    
    terminal_print("Free blocks: ", VGA_COLOR_CYAN);
    itoa(super.free_blocks, buffer, 10);
    terminal_print(buffer, VGA_COLOR_GREEN);
    terminal_print(" (", VGA_COLOR_GREEN);
    itoa(super.free_blocks * DISKFS_BLOCK_SIZE / 1024, buffer, 10);
    terminal_print(buffer, VGA_COLOR_GREEN);
    terminal_print(" KB free)\n", VGA_COLOR_GREEN);
    
    terminal_print("Current path: ", VGA_COLOR_CYAN);
    terminal_print(current_path, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
}

uint32_t diskfs_free_space(void) {
    return super.free_blocks * DISKFS_BLOCK_SIZE;
}