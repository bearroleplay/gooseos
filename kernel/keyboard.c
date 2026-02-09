#include "keyboard.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static int shift_pressed = 0;
static int caps_lock = 0;
static int ctrl_pressed = 0;
static int alt_pressed = 0;
static int layout = 0;  // 0 = English, 1 = Russian
static int shift_was_pressed = 0;
static int alt_was_pressed = 0;

// Английская раскладка (основная)
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

// РУССКАЯ РАСКЛАДКА ЙЦУКЕН (коды ASCII)
static const unsigned char scancode_table_ru[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t',0xE9,0xF6,0xF3,0xEA,0xE5,0xED,0xE3,0xE8,0xF9,0xE7,0xF5,0xFA,'\n', // й ц у к е н г ш щ з х ъ
    0,0xF4,0xFB,0xE2,0xE0,0xEF,0xF0,0xEE,0xEB,0xE4,0xE6,0xFD,0xB8,0,'\\', // ф ы в а п р о л д ж э ё
    0xFF,0xF7,0xF1,0xEC,0xE8,0xF2,0xFC,0xE1,0xFE,'/',0,'*',0,' ',0         // я ч с м и т ь б ю
};

static const unsigned char scancode_table_ru_shift[128] = {
    0,0,'!','"','#','$','%',':','&','*','(',')','_','+','\b',
    '\t',0xC9,0xD6,0xD3,0xCA,0xC5,0xCD,0xC3,0xC8,0xD9,0xC7,0xD5,0xDA,'\n', // Й Ц У К Е Н Г Ш Щ З Х Ъ
    0,0xD4,0xDB,0xC2,0xC0,0xCF,0xD0,0xCE,0xCB,0xC4,0xC6,0xDD,0xA8,0,'|', // Ф Ы В А П Р О Л Д Ж Э Ё
    0xDF,0xD7,0xD1,0xCC,0xC8,0xD2,0xDC,0xC1,0xDE,'?',0,'*',0,' ',0         // Я Ч С М И Т Ь Б Ю
};

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

void keyboard_init(void) {
    outb(0x64, 0xFF);
    while (inb(0x64) & 0x02);
}

// Проверка Alt+Shift
static void check_layout_switch(uint8_t scancode) {
    if (scancode & 0x80) {
        // Отпускание
        uint8_t key = scancode & 0x7F;
        if (key == 0x2A || key == 0x36) shift_was_pressed = 0;
        if (key == 0x38) alt_was_pressed = 0;
    } else {
        // Нажатие
        if (scancode == 0x2A || scancode == 0x36) shift_was_pressed = 1;
        if (scancode == 0x38) alt_was_pressed = 1;
        
        // Если обе нажаты - переключаем
        if (shift_was_pressed && alt_was_pressed) {
            layout = !layout;
            shift_was_pressed = 0;
            alt_was_pressed = 0;
            
            // Пищим
            __asm__ volatile("outb %0, $0x61" : : "a"((uint8_t)2));
            for (volatile int i = 0; i < 5000; i++);
            __asm__ volatile("outb %0, $0x61" : : "a"((uint8_t)0));
        }
    }
}

char keyboard_getch(void) {
    if (!(inb(KEYBOARD_STATUS_PORT) & 0x01)) return 0;
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Проверяем переключение раскладки
    check_layout_switch(scancode);
    
    if (scancode & 0x80) {
        uint8_t key = scancode & 0x7F;
        if (key == 0x2A || key == 0x36) shift_pressed = 0;
        else if (key == 0x1D) ctrl_pressed = 0;
        else if (key == 0x38) alt_pressed = 0;
        return 0;
    }
    
    switch (scancode) {
        case 0x2A: case 0x36: shift_pressed = 1; return 0;
        case 0x1D: ctrl_pressed = 1; return 0;
        case 0x38: alt_pressed = 1; return 0;
        case 0x3A: caps_lock = !caps_lock; return 0;
        case 0x0E: return '\b';
        case 0x1C: return '\n';
        case 0x39: return ' ';
        case 0x0F: return '\t';
    }
    
    if (scancode < 128) {
        const char* table;
        if (layout == 0) {
            table = (shift_pressed ^ caps_lock) ? scancode_table_shift : scancode_table;
        } else {
            table = (const char*)((shift_pressed ^ caps_lock) ? scancode_table_ru_shift : scancode_table_ru);  // Исправлено приведение типа
        }
        return table[scancode];
    }
    
    return 0;
}

int keyboard_get_shift(void) { return shift_pressed; }
int keyboard_get_caps(void) { return caps_lock; }
int keyboard_get_ctrl(void) { return ctrl_pressed; }
int keyboard_get_alt(void) { return alt_pressed; }
int keyboard_get_layout(void) { return layout; }
void keyboard_switch_layout(void) { layout = !layout; }


