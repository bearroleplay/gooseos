#ifndef BEEP_H
#define BEEP_H

#include <stdint.h>

// Просто писк (частота ~800 Гц, длительность ~200 мс)
void beep(void);

// freq - частота в Гц (50-5000)
// duration_ms - длительность в миллисекундах
void beep_freq(uint32_t freq, uint32_t duration_ms);

// Один короткий писк (успех)
void beep_ok(void);

// Два коротких писка (ошибка)
void beep_error(void);

// Три коротких писка (критическая ошибка)
void beep_critical(void);

// =============== МЕЛОДИИ ===============
// Приветственная мелодия при загрузке
//void play_startup_tune(void);

// Печальная мелодия при панике
void play_panic_tune(void);

#endif
