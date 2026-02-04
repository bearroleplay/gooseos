#include "fs.h"
#include "libc.h"
#include "terminal.h"
#include "vga.h"

// Простейшая ФС - массив файлов в памяти
static FileSystem fs;

void fs_init(void) {
    memset(&fs, 0, sizeof(FileSystem));
    fs.data = (uint8_t*)malloc(32768);
    fs.data_size = 32768;
    
    // Создаём корневую папку
    fs.current_dir = 0;
    strcpy(fs.current_path, "Z:\\");
    
    // Создаём корневой каталог (Z:)
    fs.entries[0].type = 2;  // 2 = директория
    strcpy(fs.entries[0].name, "Z:");
    fs.entries[0].parent = 0;  // Сам себе родитель
    fs.file_count = 1;
    
    // Создаём несколько тестовых файлов
    const char* hello = "print \"Hello GooseOS!\"\nexit 0\n";
    fs_create("hello.goo", (uint8_t*)hello, strlen(hello), 1);  // 1 = файл
    
    const char* welcome = "Welcome to GooseOS!\n";
    fs_create("welcome.txt", (uint8_t*)welcome, strlen(welcome), 1);  // 1 = файл
}

int fs_create(const char* name, const uint8_t* data, uint32_t size, uint8_t type) {
    // ПРОВЕРКА ИМЕНИ
    if (name == NULL || name[0] == 0) {
        return 0;
    }
    
    // Ищем свободный слот
    int free_index = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs.entries[i].type == 0) {
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1) return 0; // Нет места
    
    // Ищем файл с таким же именем (перезапись)
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs.entries[i].type != 0 && 
            fs.entries[i].parent == fs.current_dir &&  // В той же папке!
            strcmp(fs.entries[i].name, name) == 0) {
            free_index = i;
            // Освобождаем старое место в данных
            break;
        }
    }
    
    // Вычисляем смещение в данных
    uint32_t offset = fs.file_count * 512;
    
    if (offset + size > fs.data_size) {
        // Пробуем найти свободное место
        offset = 0;
        // Простая реализация - ищем первый подходящий блок
        // TODO: улучшить аллокацию
    }
    
    if (offset + size > fs.data_size) return 0;
    
    // Сохраняем метаданные
    strncpy(fs.entries[free_index].name, name, FS_MAX_NAME - 1);
    fs.entries[free_index].name[FS_MAX_NAME - 1] = 0; // Гарантируем терминирование
    fs.entries[free_index].size = size;
    fs.entries[free_index].offset = offset;
    fs.entries[free_index].type = type;
    fs.entries[free_index].parent = fs.current_dir;
    
    // Копируем данные
    if (size > 0 && data != NULL) {
        memcpy(fs.data + offset, data, size);
    }
    
    // Увеличиваем счётчик если новый файл
    if (free_index >= (int)fs.file_count) {
        fs.file_count++;
    }
    
    return 1;
}

int fs_create(const char* name, const uint8_t* data, uint32_t size, uint8_t type) {
    // Ищем свободный слот
    int free_index = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs.entries[i].type == 0) {
            free_index = i;
            break;
        }
    }
    
    if (free_index == -1) return 0; // Нет места
    
    // Ищем файл с таким же именем (перезапись)
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs.entries[i].type != 0 && 
            fs.entries[i].parent == fs.current_dir &&  // В той же папке!
            strcmp(fs.entries[i].name, name) == 0) {
            free_index = i;
            break;
        }
    }
    
    // Записываем данные
    uint32_t offset = fs.file_count * 512;
    
    if (offset + size > fs.data_size) return 0;
    
    // Сохраняем метаданные
    strncpy(fs.entries[free_index].name, name, FS_MAX_NAME - 1);
    fs.entries[free_index].size = size;
    fs.entries[free_index].offset = offset;
    fs.entries[free_index].type = type;  // Используем переданный тип!
    fs.entries[free_index].parent = fs.current_dir;  // Родитель = текущая папка
    
    // Копируем данные
    if (size > 0 && data != NULL) {
        memcpy(fs.data + offset, data, size);
    }
    
    // Увеличиваем счётчик если новый файл
    if (free_index >= (int)fs.file_count) {
        fs.file_count++;
    }
    
    return 1;
}

int fs_read(const char* name, uint8_t* buffer, uint32_t size) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs.entries[i].type == 1 && strcmp(fs.entries[i].name, name) == 0) {
            uint32_t to_copy = fs.entries[i].size;
            if (to_copy > size) to_copy = size;
            
            memcpy(buffer, fs.data + fs.entries[i].offset, to_copy);
            return to_copy;
        }
    }
    return 0; // Файл не найден
}

int fs_exists(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs.entries[i].type == 1 && strcmp(fs.entries[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}


int fs_cd(const char* path) {
    // Пока простая заглушка
    if (strcmp(path, "\\") == 0 || strcmp(path, "/") == 0) {
        fs.current_dir = 0;
        strcpy(fs.current_path, "Z:\\");
        return 1;
    }
    return 0; // Пока не реализовано
}

int fs_mkdir(const char* name) {
    return fs_create(name, NULL, 0, 2);  // 2 = директория
}

int fs_delete(const char* name) {
    // Пока простая заглушка
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs.entries[i].type != 0 && 
            fs.entries[i].parent == fs.current_dir &&
            strcmp(fs.entries[i].name, name) == 0) {
            // Помечаем как свободный
            fs.entries[i].type = 0;
            return 1;
        }
    }
    return 0;
}

int fs_rmdir(const char* name) {
    // Пока то же самое что удаление
    return fs_delete(name);
}

char* fs_pwd(void) {
    return fs.current_path;
}

// И функция для списка файлов
void fs_list(void) {
    // Используй терминал для вывода
    // Пока простой вывод всех файлов
    terminal_print("\n=== Files in ", 3);  // CYAN
    terminal_print(fs.current_path, 15);   // WHITE
    terminal_print(" ===\n", 3);
    
    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (fs.entries[i].type != 0 && 
            fs.entries[i].parent == fs.current_dir) {
            
            if (fs.entries[i].type == 2) {  // Директория
                terminal_print("  [DIR] ", 9);  // LIGHT_BLUE
            } else {  // Файл
                terminal_print("        ", 7);  // LIGHT_GRAY
            }
            
            terminal_print(fs.entries[i].name, 15);  // WHITE
            
            // Размер для файлов
            if (fs.entries[i].type == 1) {
                char size_str[32];
                itoa(fs.entries[i].size, size_str, 10);
                terminal_print(" (", 7);
                terminal_print(size_str, 10);  // LIGHT_GREEN
                terminal_print(" bytes)", 7);
            }
            
            terminal_print("\n", 7);
            count++;
        }
    }
    
    if (count == 0) {
        terminal_print("  Empty\n", 7);
    }
}