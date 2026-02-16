#include "beep.h"
#include "libc.h"
#include "terminal.h"

static void beep_raw(uint32_t freq, uint32_t duration_ms) {
    if (freq < 20 || freq > 20000) return;  // Защита от дурака
    
    // Рассчитываем делитель для частоты
    // 1193180 Гц - базовая частота PC Speaker
    uint32_t div = 1193180 / freq;
    
    // Сохраняем текущее состояние порта 0x61
    uint8_t tmp = inb(0x61);
    
    // Включаем динамик (биты 0 и 1)
    outb(0x61, tmp | 0x03);
    
    // Настраиваем канал 2 таймера (PC Speaker)
    outb(0x43, 0xB6);  // 0xB6 = канал 2, режим 3, двоичный счет
    
    // Отправляем делитель (сначала младший байт, потом старший)
    outb(0x42, div & 0xFF);
    outb(0x42, (div >> 8) & 0xFF);
    
    // Ждем нужное время (приблизительно)
    for (volatile uint32_t i = 0; i < duration_ms * 1000; i++);
    
    // Выключаем динамик (сбрасываем биты 0 и 1)
    outb(0x61, tmp & 0xFC);
}

void beep(void) {
    beep_raw(800, 200);  // 800 Гц на 200 мс
}

void beep_freq(uint32_t freq, uint32_t duration_ms) {
    beep_raw(freq, duration_ms);
}

void beep_ok(void) {
    beep_raw(1000, 100);  // Короткий высокий писк
}

void beep_error(void) {
    beep_raw(400, 150);   // Низкий писк
    for (volatile int i = 0; i < 100000; i++);
    beep_raw(400, 150);   // И еще один
}

void beep_critical(void) {
    for (int i = 0; i < 3; i++) {
        beep_raw(200, 300);  // Три длинных низких писка
        for (volatile int j = 0; j < 200000; j++);
    }
}

void play_startup_tune(void) {
    // Простая мелодия: до-ре-ми-фа-соль
    beep_raw(523, 200);  // до
    for (volatile int i = 0; i < 50000; i++);
    beep_raw(587, 200);  // ре
    for (volatile int i = 0; i < 50000; i++);
    beep_raw(659, 200);  // ми
    for (volatile int i = 0; i < 50000; i++);
    beep_raw(698, 200);  // фа
    for (volatile int i = 0; i < 50000; i++);
    beep_raw(784, 400);  // соль (подлиннее)
}

void play_panic_tune(void) {
    // Печальная мелодия при панике (си-бемоль вниз)
    beep_raw(466, 300);  // си-бемоль
    for (volatile int i = 0; i < 100000; i++);
    beep_raw(415, 300);  // соль-диез
    for (volatile int i = 0; i < 100000; i++);
    beep_raw(370, 300);  // фа-диез
    for (volatile int i = 0; i < 100000; i++);
    beep_raw(311, 600);  // ре-диез (длинный, грустный)
}
