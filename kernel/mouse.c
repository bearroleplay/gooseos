#include "mouse.h"
#include "vga.h"
#include "keyboard.h"
#include "libc.h"
#include "terminal.h"
#include "graphics.h"
#include <string.h>

// Порты PS/2 мыши
#define MOUSE_DATA    0x60
#define MOUSE_STATUS  0x64
#define MOUSE_COMMAND 0x64

// Команды мыши
#define MOUSE_RESET     0xFF
#define MOUSE_RESEND    0xFE
#define MOUSE_SET_DEFAULTS 0xF6
#define MOUSE_DISABLE   0xF5
#define MOUSE_ENABLE    0xF4
#define MOUSE_SET_SAMPLE_RATE 0xF3
#define MOUSE_GET_ID    0xF2
#define MOUSE_SET_REMOTE_MODE 0xF0
#define MOUSE_READ_DATA 0xEB
#define MOUSE_SET_STREAM_MODE 0xEA
#define MOUSE_STATUS_REQUEST 0xE9

// Глобальное состояние мыши
static MouseState mouse = {0};
static int mouse_phase = 0;
static uint8_t mouse_packet[3];
static uint8_t mouse_cycle = 0;

// ========== ПОРТЫ ВВОДА/ВЫВОДА ==========

// Ожидание готовности мыши
static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) { // Ожидаем готовности к отправке
        while (timeout--) {
            if ((inb(MOUSE_STATUS) & 2) == 0) return;
        }
    } else { // Ожидаем данных
        while (timeout--) {
            if ((inb(MOUSE_STATUS) & 1) == 1) return;
        }
    }
}

// Отправка команды мыши
static void mouse_write(uint8_t cmd) {
    mouse_wait(0);
    outb(MOUSE_COMMAND, 0xD4); // Сказать контроллеру, что команда для мыши
    mouse_wait(0);
    outb(MOUSE_DATA, cmd);
}

// Чтение ответа мыши
static uint8_t mouse_read(void) {
    mouse_wait(1);
    return inb(MOUSE_DATA);
}

// ========== ИНИЦИАЛИЗАЦИЯ МЫШИ ==========

void mouse_init(void) {
    terminal_print("Initializing PS/2 mouse... ", VGA_COLOR_CYAN);
    
    // Включить вспомогательное устройство (мышь)
    mouse_wait(0);
    outb(MOUSE_COMMAND, 0xA8);
    
    // Включить прерывания
    mouse_wait(0);
    outb(MOUSE_COMMAND, 0x20);
    mouse_wait(1);
    uint8_t status = inb(MOUSE_DATA) | 2;
    mouse_wait(0);
    outb(MOUSE_COMMAND, 0x60);
    mouse_wait(0);
    outb(MOUSE_DATA, status);
    
    // Сброс мыши
    mouse_write(MOUSE_RESET);
    mouse_read(); // Должен вернуть 0xFA (ACK)
    mouse_read(); // Должен вернуть 0xAA (Self-test passed)
    mouse_read(); // Должен вернуть ID (обычно 0x00)
    
    // Установить стандартные настройки
    mouse_write(MOUSE_SET_DEFAULTS);
    mouse_read();
    
    // Включить мышь
    mouse_write(MOUSE_ENABLE);
    mouse_read();
    
    // Установить скорость опроса
    mouse_write(MOUSE_SET_SAMPLE_RATE);
    mouse_read();
    mouse_write(200); // 200Hz
    mouse_read();
    
    mouse.present = 1;
    mouse.enabled = 1;
    mouse.x = 160; // Центр экрана
    mouse.y = 100;
    
    terminal_print("OK\n", VGA_COLOR_GREEN);
}

void mouse_enable(void) {
    if (!mouse.present) return;
    mouse_write(MOUSE_ENABLE);
    mouse_read();
    mouse.enabled = 1;
}

void mouse_disable(void) {
    if (!mouse.present) return;
    mouse_write(MOUSE_DISABLE);
    mouse_read();
    mouse.enabled = 0;
}

// ========== ОБРАБОТКА ПАКЕТОВ ==========

void mouse_handle_packet(uint8_t packet[3]) {
    if (!mouse.enabled) return;
    
    // Проверка первого байта
    uint8_t flags = packet[0];
    
    // Движение
    int dx = packet[1];
    int dy = packet[2];
    
    // Преобразование относительных координат
    if (dx & 0x80) dx |= 0xFFFFFF00;
    if (dy & 0x80) dy |= 0xFFFFFF00;
    
    // Инвертируем Y (мышь движется вниз = увеличение Y)
    dy = -dy;
    
    // Обновляем состояние
    mouse.dx = dx;
    mouse.dy = dy;
    mouse.x += dx;
    mouse.y += dy;
    
    // Ограничиваем координаты экраном (режим 13h: 320x200)
    if (mouse.x < 0) mouse.x = 0;
    if (mouse.x >= 320) mouse.x = 319;
    if (mouse.y < 0) mouse.y = 0;
    if (mouse.y >= 200) mouse.y = 199;
    
    // Кнопки
    mouse.buttons = 0;
    if (flags & 0x01) mouse.buttons |= 1; // ЛКМ
    if (flags & 0x02) mouse.buttons |= 2; // ПКМ
    if (flags & 0x04) mouse.buttons |= 4; // Средняя кнопка
}

// Получить текущее состояние
MouseState mouse_get_state(void) {
    return mouse;
}

// ========== ТЕСТОВАЯ ПРОГРАММА ==========

void mouse_test_program(void) {
    if (!mouse.present) {
        terminal_print("Mouse not detected! Trying to initialize...\n", VGA_COLOR_RED);
        mouse_init();
        if (!mouse.present) {
            terminal_print("Failed to initialize mouse\n", VGA_COLOR_RED);
            return;
        }
    }
    
    terminal_print("Starting mouse test...\n", VGA_COLOR_CYAN);
    terminal_print("ESC - Exit | R - Reset cursor | C - Clear screen\n", VGA_COLOR_LIGHT_GRAY);
    
    // Переходим в графический режим
    vga_set_mode_13h();
    
    // Очищаем экран
    clear_screen(MOUSE_COLOR_BG);
    
    // Рисуем сетку для удобства
    for (int x = 0; x < 320; x += 20) {
        for (int y = 0; y < 200; y++) {
            draw_pixel(x, y, 0x08); // Тёмно-серый
        }
    }
    for (int y = 0; y < 200; y += 20) {
        for (int x = 0; x < 320; x++) {
            draw_pixel(x, y, 0x08); // Тёмно-серый
        }
    }
    
    // Матрица пикселей для отслеживания кликов
    static uint8_t pixel_colors[320][200];
    memset(pixel_colors, 0, sizeof(pixel_colors));
    
    // Главный цикл теста
    int last_x = mouse.x;
    int last_y = mouse.y;
    
    while (1) {
        // Читаем PS/2 порт
        if (inb(0x64) & 1) {
            uint8_t data = inb(0x60);
            
            // Обработка пакетов мыши (3 байта)
            mouse_packet[mouse_cycle++] = data;
            
            if (mouse_cycle == 3) {
                mouse_handle_packet(mouse_packet);
                mouse_cycle = 0;
            }
        }
        
        // Обработка клавиатуры
        char key = keyboard_getch();
        if (key == 27) break; // ESC - выход
        if (key == 'r' || key == 'R') {
            mouse.x = 160;
            mouse.y = 100;
        }
        if (key == 'c' || key == 'C') {
            clear_screen(MOUSE_COLOR_BG);
            memset(pixel_colors, 0, sizeof(pixel_colors));
            
            // Перерисовываем сетку
            for (int x = 0; x < 320; x += 20) {
                for (int y = 0; y < 200; y++) {
                    draw_pixel(x, y, 0x08);
                }
            }
            for (int y = 0; y < 200; y += 20) {
                for (int x = 0; x < 320; x++) {
                    draw_pixel(x, y, 0x08);
                }
            }
        }
        
        // Стираем старый курсор (если он был нарисован)
        if (last_x != mouse.x || last_y != mouse.y) {
            // Восстанавливаем пиксель под курсором
            if (pixel_colors[last_x][last_y] == 0) {
                // Проверяем, не сетка ли это
                if (last_x % 20 == 0 || last_y % 20 == 0) {
                    draw_pixel(last_x, last_y, 0x08);
                } else {
                    draw_pixel(last_x, last_y, MOUSE_COLOR_BG);
                }
            } else {
                draw_pixel(last_x, last_y, pixel_colors[last_x][last_y]);
            }
        }
        
        // Обработка кнопок мыши
        MouseState state = mouse_get_state();
        
        if (state.buttons & 1) { // ЛКМ
            pixel_colors[mouse.x][mouse.y] = MOUSE_COLOR_LMB;
            draw_pixel(mouse.x, mouse.y, MOUSE_COLOR_LMB);
            // Рисуем перекрестие для ЛКМ
            for (int i = -3; i <= 3; i++) {
                if (mouse.x + i >= 0 && mouse.x + i < 320) {
                    draw_pixel(mouse.x + i, mouse.y, MOUSE_COLOR_LMB);
                    pixel_colors[mouse.x + i][mouse.y] = MOUSE_COLOR_LMB;
                }
                if (mouse.y + i >= 0 && mouse.y + i < 200) {
                    draw_pixel(mouse.x, mouse.y + i, MOUSE_COLOR_LMB);
                    pixel_colors[mouse.x][mouse.y + i] = MOUSE_COLOR_LMB;
                }
            }
        }
        else if (state.buttons & 2) { // ПКМ
            pixel_colors[mouse.x][mouse.y] = MOUSE_COLOR_RMB;
            draw_pixel(mouse.x, mouse.y, MOUSE_COLOR_RMB);
            // Рисуем круг для ПКМ
            for (int dy = -3; dy <= 3; dy++) {
                for (int dx = -3; dx <= 3; dx++) {
                    if (dx*dx + dy*dy <= 9) {
                        int nx = mouse.x + dx;
                        int ny = mouse.y + dy;
                        if (nx >= 0 && nx < 320 && ny >= 0 && ny < 200) {
                            draw_pixel(nx, ny, MOUSE_COLOR_RMB);
                            pixel_colors[nx][ny] = MOUSE_COLOR_RMB;
                        }
                    }
                }
            }
        }
        else if (state.buttons & 4) { // Средняя кнопка
            pixel_colors[mouse.x][mouse.y] = MOUSE_COLOR_MIDDLE;
            draw_pixel(mouse.x, mouse.y, MOUSE_COLOR_MIDDLE);
            // Рисуем квадрат для средней кнопки
            for (int dy = -2; dy <= 2; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int nx = mouse.x + dx;
                    int ny = mouse.y + dy;
                    if (nx >= 0 && nx < 320 && ny >= 0 && ny < 200) {
                        draw_pixel(nx, ny, MOUSE_COLOR_MIDDLE);
                        pixel_colors[nx][ny] = MOUSE_COLOR_MIDDLE;
                    }
                }
            }
        }
        else if (mouse.dx != 0 || mouse.dy != 0) { // Просто движение
            // Синий след при движении
            if (pixel_colors[mouse.x][mouse.y] == 0) {
                draw_pixel(mouse.x, mouse.y, MOUSE_COLOR_MOVE);
                pixel_colors[mouse.x][mouse.y] = MOUSE_COLOR_MOVE;
            }
        }
        
        // Рисуем курсор мыши (крестик)
        draw_pixel(mouse.x, mouse.y, MOUSE_COLOR_CURSOR);
        for (int i = 1; i <= 3; i++) {
            if (mouse.x - i >= 0) draw_pixel(mouse.x - i, mouse.y, MOUSE_COLOR_CURSOR);
            if (mouse.x + i < 320) draw_pixel(mouse.x + i, mouse.y, MOUSE_COLOR_CURSOR);
            if (mouse.y - i >= 0) draw_pixel(mouse.x, mouse.y - i, MOUSE_COLOR_CURSOR);
            if (mouse.y + i < 200) draw_pixel(mouse.x, mouse.y + i, MOUSE_COLOR_CURSOR);
        }
        
        // Статусная строка внизу
        char status[80];
        sprintf(status, "X:%03d Y:%03d LMB:%c RMB:%c MID:%c DX:%03d DY:%03d", 
                mouse.x, mouse.y,
                (state.buttons & 1) ? 'X' : ' ',
                (state.buttons & 2) ? 'X' : ' ',
                (state.buttons & 4) ? 'X' : ' ',
                mouse.dx, mouse.dy);
        
        // Очищаем строку статуса
        for (int x = 0; x < 320; x++) {
            draw_pixel(x, 199, 0x00);
        }
        
        // Рисуем текст статуса (простые буквы)
        int text_x = 10;
        for (int i = 0; status[i] && text_x < 310; i++) {
            draw_char(text_x, 192, status[i], 0x0F);
            text_x += 9;
        }
        
        // Запоминаем позицию для следующего кадра
        last_x = mouse.x;
        last_y = mouse.y;
        mouse.dx = 0;
        mouse.dy = 0;
        
        // Небольшая задержка
        for (volatile int i = 0; i < 1000; i++);
    }
    
    // Возвращаемся в текстовый режим
    vga_set_mode_text();
    terminal_clear_with_banner();
}