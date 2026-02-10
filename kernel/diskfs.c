#include "diskfs.h"
#include "libc.h"
#include "terminal.h"
#include <string.h>

// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ==========
static int fs_mounted = 0;
static char current_path[256] = "/";
static char current_dir_name[256] = "/";

// Временное хранилище файлов в памяти
typedef struct {
    char name[DISKFS_FILENAME_LEN];
    uint8_t type;           // FILE или DIR
    uint32_t size;
    char* data;             // Для файлов
    char parent[256];       // Родительский путь
    int in_use;
} FileEntry;

static FileEntry file_table[DISKFS_MAX_FILES];
static int file_count = 0;

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========
static int find_file(const char* name) {
    char full_path[512];
    
    if(name[0] == '/') {
        // Абсолютный путь
        strcpy(full_path, name);
    } else {
        // Относительный путь
        strcpy(full_path, current_dir_name);
        if(strcmp(current_dir_name, "/") != 0) {
            strcat(full_path, "/");
        }
        strcat(full_path, name);
    }
    
    // Ищем файл
    for(int i = 0; i < DISKFS_MAX_FILES; i++) {
        if(file_table[i].in_use) {
            char file_path[512];
            strcpy(file_path, file_table[i].parent);
            if(strcmp(file_table[i].parent, "/") != 0) {
                strcat(file_path, "/");
            }
            strcat(file_path, file_table[i].name);
            
            if(strcmp(file_path, full_path) == 0) {
                return i;
            }
        }
    }
    
    return -1;
}

static int add_file(const char* name, uint8_t type) {
    if(file_count >= DISKFS_MAX_FILES) {
        terminal_print("Maximum files reached\n", VGA_COLOR_RED);
        return -1;
    }
    
    // Находим свободный слот
    int index = -1;
    for(int i = 0; i < DISKFS_MAX_FILES; i++) {
        if(!file_table[i].in_use) {
            index = i;
            break;
        }
    }
    
    if(index == -1) return -1;
    
    // Извлекаем имя из пути
    const char* basename = name;
    const char* slash = strrchr(name, '/');
    if(slash) {
        basename = slash + 1;
    }
    
    // Инициализируем файл
    memset(&file_table[index], 0, sizeof(FileEntry));
    strcpy(file_table[index].name, basename);
    file_table[index].type = type;
    strcpy(file_table[index].parent, current_dir_name);
    file_table[index].in_use = 1;
    
    if(type == DISKFS_TYPE_DIR) {
        // Создаем записи . и ..
        // (в этой упрощенной версии пропускаем)
    }
    
    file_count++;
    return index;
}

static void remove_file(int index) {
    if(index < 0 || index >= DISKFS_MAX_FILES || !file_table[index].in_use) {
        return;
    }
    
    // Освобождаем память данных
    if(file_table[index].data) {
        free(file_table[index].data);
    }
    
    file_table[index].in_use = 0;
    file_count--;
}

static void normalize_path(char* path) {
    // Убираем двойные слеши
    int len = strlen(path);
    int j = 0;
    
    for(int i = 0; i < len; i++) {
        if(path[i] == '/' && (i > 0 && path[i-1] == '/')) {
            continue;
        }
        path[j++] = path[i];
    }
    path[j] = 0;
    
    // Убираем завершающий слеш (если не корень)
    if(j > 1 && path[j-1] == '/') {
        path[j-1] = 0;
    }
}

// ========== ОСНОВНЫЕ ФУНКЦИИ ФС ==========
int diskfs_init(void) {
    terminal_print("Initializing filesystem... ", VGA_COLOR_CYAN);
    
    // Очищаем таблицу файлов
    memset(file_table, 0, sizeof(file_table));
    file_count = 0;
    
    // Создаем корневой каталог
    int root_index = add_file("/", DISKFS_TYPE_DIR);
    if(root_index >= 0) {
        strcpy(file_table[root_index].parent, "");
    }
    
    // Создаем несколько тестовых файлов
    add_file("readme.txt", DISKFS_TYPE_FILE);
    int readme_idx = find_file("/readme.txt");
    if(readme_idx >= 0) {
        const char* text = "Welcome to GooseOS!\n\n"
                          "Available commands:\n"
                          "  ls          - List files\n"
                          "  cd <dir>    - Change directory\n"
                          "  edit <file> - Edit file\n"
                          "  mkdir <dir> - Create directory\n"
                          "  rm <file>   - Delete file\n"
                          "  help        - Show all commands\n";
        file_table[readme_idx].size = strlen(text);
        file_table[readme_idx].data = malloc(file_table[readme_idx].size + 1);
        strcpy(file_table[readme_idx].data, text);
    }
    
    add_file("test.goo", DISKFS_TYPE_FILE);
    int test_idx = find_file("/test.goo");
    if(test_idx >= 0) {
        const char* code = "print \"Hello from GooseOS!\"\nexit 0";
        file_table[test_idx].size = strlen(code);
        file_table[test_idx].data = malloc(file_table[test_idx].size + 1);
        strcpy(file_table[test_idx].data, code);
    }
    
    add_file("projects", DISKFS_TYPE_DIR);
    
    fs_mounted = 1;
    strcpy(current_path, "/");
    strcpy(current_dir_name, "/");
    
    terminal_print("OK\n", VGA_COLOR_GREEN);
    return 1;
}

int diskfs_format(void) {
    terminal_print("Formatting filesystem... ", VGA_COLOR_YELLOW);
    
    // Освобождаем всю память
    for(int i = 0; i < DISKFS_MAX_FILES; i++) {
        if(file_table[i].in_use && file_table[i].data) {
            free(file_table[i].data);
        }
    }
    
    // Очищаем таблицу
    memset(file_table, 0, sizeof(file_table));
    file_count = 0;
    
    // Создаем корень
    int root_index = add_file("/", DISKFS_TYPE_DIR);
    if(root_index >= 0) {
        strcpy(file_table[root_index].parent, "");
    }
    
    // Создаем readme файл
    add_file("readme.txt", DISKFS_TYPE_FILE);
    int readme_idx = find_file("/readme.txt");
    if(readme_idx >= 0) {
        const char* text = "Newly formatted filesystem\n";
        file_table[readme_idx].size = strlen(text);
        file_table[readme_idx].data = malloc(file_table[readme_idx].size + 1);
        strcpy(file_table[readme_idx].data, text);
    }
    
    strcpy(current_path, "/");
    strcpy(current_dir_name, "/");
    
    terminal_print("OK\n", VGA_COLOR_GREEN);
    return 1;
}

void diskfs_info(void) {
    terminal_print("\n=== Filesystem Information ===\n", VGA_COLOR_YELLOW);
    
    char buf[32];
    terminal_print("Files: ", VGA_COLOR_CYAN);
    itoa(file_count, buf, 10);
    terminal_print(buf, VGA_COLOR_WHITE);
    terminal_print(" / ", VGA_COLOR_WHITE);
    itoa(DISKFS_MAX_FILES, buf, 10);
    terminal_print(buf, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    terminal_print("Current path: ", VGA_COLOR_CYAN);
    terminal_print(current_path, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    // Считаем статистику
    int dirs = 0, files = 0;
    uint32_t total_size = 0;
    
    for(int i = 0; i < DISKFS_MAX_FILES; i++) {
        if(file_table[i].in_use) {
            if(file_table[i].type == DISKFS_TYPE_DIR) {
                dirs++;
            } else {
                files++;
                total_size += file_table[i].size;
            }
        }
    }
    
    terminal_print("Directories: ", VGA_COLOR_CYAN);
    itoa(dirs, buf, 10);
    terminal_print(buf, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    terminal_print("Files: ", VGA_COLOR_CYAN);
    itoa(files, buf, 10);
    terminal_print(buf, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_WHITE);
    
    terminal_print("Total size: ", VGA_COLOR_CYAN);
    itoa(total_size, buf, 10);
    terminal_print(buf, VGA_COLOR_WHITE);
    terminal_print(" bytes\n", VGA_COLOR_WHITE);
}

// ========== ФАЙЛОВЫЕ ОПЕРАЦИИ ==========
int diskfs_create(const char* name, uint8_t type) {
    if(!fs_mounted) return 0;
    
    // Проверяем, нет ли уже файла
    if(find_file(name) >= 0) {
        terminal_print("File already exists: ", VGA_COLOR_RED);
        terminal_print(name, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Создаем файл
    int index = add_file(name, type);
    if(index < 0) {
        terminal_print("Cannot create file\n", VGA_COLOR_RED);
        return 0;
    }
    
    terminal_print("Created: ", VGA_COLOR_GREEN);
    terminal_print(name, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_GREEN);
    return 1;
}

int diskfs_delete(const char* name) {
    if(!fs_mounted) return 0;
    
    int index = find_file(name);
    if(index < 0) {
        terminal_print("File not found: ", VGA_COLOR_RED);
        terminal_print(name, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем что это не каталог с файлами
    if(file_table[index].type == DISKFS_TYPE_DIR) {
        // Ищем файлы в этом каталоге
        char dir_path[512];
        if(strcmp(current_dir_name, "/") == 0) {
            sprintf(dir_path, "/%s", file_table[index].name);
        } else {
            sprintf(dir_path, "%s/%s", current_dir_name, file_table[index].name);
        }
        
        int has_files = 0;
        for(int i = 0; i < DISKFS_MAX_FILES; i++) {
            if(file_table[i].in_use && strcmp(file_table[i].parent, dir_path) == 0) {
                has_files = 1;
                break;
            }
        }
        
        if(has_files) {
            terminal_print("Directory not empty\n", VGA_COLOR_RED);
            return 0;
        }
    }
    
    // Удаляем файл
    const char* filename = file_table[index].name;
    remove_file(index);
    
    terminal_print("Deleted: ", VGA_COLOR_GREEN);
    terminal_print(filename, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_GREEN);
    return 1;
}

int diskfs_read(const char* name, uint8_t* buffer, uint32_t size) {
    if(!fs_mounted) return 0;
    
    int index = find_file(name);
    if(index < 0 || file_table[index].type != DISKFS_TYPE_FILE) {
        return 0;
    }
    
    uint32_t to_copy = file_table[index].size;
    if(to_copy > size) to_copy = size;
    
    if(file_table[index].data && to_copy > 0) {
        memcpy(buffer, file_table[index].data, to_copy);
    }
    
    return to_copy;
}

int diskfs_write(const char* name, const uint8_t* data, uint32_t size) {
    if(!fs_mounted) return 0;
    
    int index = find_file(name);
    
    if(index < 0) {
        // Создаем новый файл
        if(!diskfs_create(name, DISKFS_TYPE_FILE)) {
            return 0;
        }
        index = find_file(name);
        if(index < 0) return 0;
    }
    
    if(file_table[index].type != DISKFS_TYPE_FILE) {
        terminal_print("Not a file\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Освобождаем старые данные
    if(file_table[index].data) {
        free(file_table[index].data);
    }
    
    // Выделяем новую память
    file_table[index].data = malloc(size + 1);
    if(!file_table[index].data) {
        terminal_print("Memory error\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Копируем данные
    memcpy(file_table[index].data, data, size);
    file_table[index].data[size] = 0; // Null terminator
    file_table[index].size = size;
    
    return size;
}

int diskfs_exists(const char* name) {
    if(!fs_mounted) return 0;
    return find_file(name) >= 0;
}

int diskfs_rename(const char* oldname, const char* newname) {
    if(!fs_mounted) return 0;
    
    int index = find_file(oldname);
    if(index < 0) {
        terminal_print("File not found: ", VGA_COLOR_RED);
        terminal_print(oldname, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Проверяем что новое имя не занято
    if(find_file(newname) >= 0) {
        terminal_print("File already exists: ", VGA_COLOR_RED);
        terminal_print(newname, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Меняем имя
    strcpy(file_table[index].name, newname);
    
    terminal_print("Renamed: ", VGA_COLOR_GREEN);
    terminal_print(oldname, VGA_COLOR_WHITE);
    terminal_print(" -> ", VGA_COLOR_GREEN);
    terminal_print(newname, VGA_COLOR_WHITE);
    terminal_print("\n", VGA_COLOR_GREEN);
    return 1;
}

int diskfs_copy(const char* src, const char* dst) {
    if(!fs_mounted) return 0;
    
    // Читаем исходный файл
    uint8_t buffer[4096];
    int size = diskfs_read(src, buffer, sizeof(buffer));
    
    if(size <= 0) {
        terminal_print("Source file not found\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Записываем в целевой файл
    return diskfs_write(dst, buffer, size) > 0;
}

// ========== ОПЕРАЦИИ С КАТАЛОГАМИ ==========
int diskfs_cd(const char* path) {
    if(!fs_mounted) return 0;
    
    if(strcmp(path, "/") == 0) {
        strcpy(current_path, "/");
        strcpy(current_dir_name, "/");
        return 1;
    }
    
    if(strcmp(path, "..") == 0) {
        // Переход на уровень выше
        if(strcmp(current_dir_name, "/") == 0) {
            return 1; // Уже в корне
        }
        
        // Находим последний слеш
        char* last_slash = strrchr(current_dir_name, '/');
        if(last_slash) {
            if(last_slash == current_dir_name) {
                // Переходим в корень
                strcpy(current_dir_name, "/");
            } else {
                *last_slash = 0;
            }
        }
        
        // Обновляем путь для отображения
        if(strcmp(current_dir_name, "/") == 0) {
            strcpy(current_path, "/");
        } else {
            strcpy(current_path, current_dir_name);
        }
        
        return 1;
    }
    
    if(strcmp(path, ".") == 0) {
        return 1; // Остаемся в текущей
    }
    
    // Строим новый путь
    char new_path[512];
    if(strcmp(current_dir_name, "/") == 0) {
        sprintf(new_path, "/%s", path);
    } else {
        sprintf(new_path, "%s/%s", current_dir_name, path);
    }
    normalize_path(new_path);
    
    // Проверяем что каталог существует
    int index = find_file(new_path);
    if(index < 0 || file_table[index].type != DISKFS_TYPE_DIR) {
        terminal_print("Directory not found: ", VGA_COLOR_RED);
        terminal_print(path, VGA_COLOR_WHITE);
        terminal_print("\n", VGA_COLOR_RED);
        return 0;
    }
    
    // Обновляем текущий каталог
    strcpy(current_dir_name, new_path);
    strcpy(current_path, new_path);
    
    return 1;
}

int diskfs_mkdir(const char* name) {
    return diskfs_create(name, DISKFS_TYPE_DIR);
}

int diskfs_rmdir(const char* name) {
    return diskfs_delete(name);
}

char* diskfs_pwd(void) {
    return current_path;
}

void diskfs_list(void) {
    if(!fs_mounted) {
        terminal_print("Filesystem not mounted\n", VGA_COLOR_RED);
        return;
    }
    
    terminal_print("\n=== ", VGA_COLOR_CYAN);
    terminal_print(current_path, VGA_COLOR_WHITE);
    terminal_print(" ===\n", VGA_COLOR_CYAN);
    
    // Ищем все файлы в текущем каталоге
    int count = 0;
    
    for(int i = 0; i < DISKFS_MAX_FILES; i++) {
        if(file_table[i].in_use) {
            // Проверяем что файл в текущем каталоге
            if(strcmp(file_table[i].parent, current_dir_name) == 0) {
                if(file_table[i].type == DISKFS_TYPE_DIR) {
                    terminal_print("  [DIR]  ", VGA_COLOR_LIGHT_BLUE);
                } else {
                    terminal_print("         ", VGA_COLOR_LIGHT_GRAY);
                }
                
                terminal_print(file_table[i].name, VGA_COLOR_WHITE);
                
                if(file_table[i].type == DISKFS_TYPE_FILE) {
                    char size_str[32];
                    terminal_print(" (", VGA_COLOR_LIGHT_GRAY);
                    itoa(file_table[i].size, size_str, 10);
                    terminal_print(size_str, VGA_COLOR_LIGHT_GREEN);
                    terminal_print(" bytes)", VGA_COLOR_LIGHT_GRAY);
                }
                
                terminal_print("\n", VGA_COLOR_WHITE);
                count++;
            }
        }
    }
    
    if(count == 0) {
        terminal_print("  Empty directory\n", VGA_COLOR_LIGHT_GRAY);
    }
    
    char info[32];
    terminal_print("\nTotal: ", VGA_COLOR_LIGHT_GRAY);
    itoa(count, info, 10);
    terminal_print(info, VGA_COLOR_GREEN);
    terminal_print(" items\n", VGA_COLOR_LIGHT_GRAY);
}

uint32_t diskfs_free_space(void) {
    // В памяти все всегда есть место :)
    return 1024 * 1024; // 1MB условно
}