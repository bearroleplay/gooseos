#include "keyboard.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static int shift_pressed = 0;
static int caps_lock = 0;
static int ctrl_pressed = 0;    // Добавляем
static int alt_pressed = 0;     // Добавляем

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static const char scancode_table[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0
};

static const char scancode_table_shift[128] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',0
};

void keyboard_init(void) {
    outb(0x64, 0xFF);
    while (inb(0x64) & 0x02);
}

char keyboard_getch(void) {
    if (!(inb(KEYBOARD_STATUS_PORT) & 0x01)) return 0;
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    if (scancode & 0x80) {
        // Отпускание клавиши
        uint8_t key = scancode & 0x7F;
        if (key == 0x2A || key == 0x36) shift_pressed = 0;
        else if (key == 0x1D) ctrl_pressed = 0;      // Добавляем
        else if (key == 0x38) alt_pressed = 0;       // Добавляем
        return 0;
    }
    
    // Нажатие клавиши
    switch (scancode) {
        case 0x2A:
        case 0x36: shift_pressed = 1; return 0;
        case 0x1D: ctrl_pressed = 1; return 0;      // Добавляем
        case 0x38: alt_pressed = 1; return 0;       // Добавляем
        case 0x3A: caps_lock = !caps_lock; return 0;
        case 0x0E: return '\b';
        case 0x1C: return '\n';
        case 0x39: return ' ';
        case 0x0F: return '\t';
    }
    
    if (scancode < 128) {
        if (shift_pressed ^ caps_lock) {
            return scancode_table_shift[scancode];
        } else {
            return scancode_table[scancode];
        }
    }
    
    return 0;
}

int keyboard_get_shift(void) { return shift_pressed; }
int keyboard_get_caps(void) { return caps_lock; }
int keyboard_get_ctrl(void) { return ctrl_pressed; }    // Добавляем
int keyboard_get_alt(void) { return alt_pressed; }      // Добавляем