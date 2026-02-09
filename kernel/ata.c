#include "ata.h"
#include "libc.h"
#include "terminal.h"

// Более простой ATA драйвер для QEMU
static void io_wait(void) {
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}

static void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int ata_init(void) {
    // Проверяем наличие диска (Primary Master)
    outb(0x1F6, 0xA0); // Select Primary Master
    
    // Ждем
    for(int i = 0; i < 4; i++) inb(0x1F7);
    
    // Проверяем статус
    uint8_t status = inb(0x1F7);
    if(status == 0xFF) {
        terminal_print("ATA: No drive detected (status=0xFF)\n", VGA_COLOR_YELLOW);
        return -1;
    }
    
    // Ждем пока диск не будет готов
    int timeout = 100000;
    while(timeout-- > 0) {
        status = inb(0x1F7);
        if(!(status & 0x80)) break; // BSY cleared
        io_wait();
    }
    
    if(timeout <= 0) {
        terminal_print("ATA: Drive busy timeout\n", VGA_COLOR_YELLOW);
        return -1;
    }
    
    terminal_print("ATA: Drive detected\n", VGA_COLOR_GREEN);
    return 0;
}

int ata_read_sectors(uint32_t lba, uint8_t* buffer, uint8_t count) {
    // Проверяем что диск не busy
    if(inb(0x1F7) & 0x80) return -1;
    
    // Устанавливаем параметры
    outb(0x1F2, count);          // Sector count
    outb(0x1F3, lba & 0xFF);     // LBA low
    outb(0x1F4, (lba >> 8) & 0xFF); // LBA mid
    outb(0x1F5, (lba >> 16) & 0xFF); // LBA high
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F)); // LBA最高шие биты + 0xE0
    
    // Команда чтения
    outb(0x1F7, 0x20); // READ SECTORS command
    
    // Читаем секторы
    for(int s = 0; s < count; s++) {
        // Ждем пока данные готовы
        int timeout = 100000;
        while(timeout-- > 0) {
            uint8_t status = inb(0x1F7);
            if(status & 0x08) break; // DRQ set
            if(status & 0x01) { // Error
                terminal_print("ATA: Read error\n", VGA_COLOR_RED);
                return -1;
            }
            io_wait();
        }
        
        if(timeout <= 0) {
            terminal_print("ATA: Read timeout\n", VGA_COLOR_RED);
            return -1;
        }
        
        // Читаем 256 слов (512 байт)
        uint16_t* buf = (uint16_t*)(buffer + s * 512);
        for(int i = 0; i < 256; i++) {
            buf[i] = inb(0x1F0);
        }
    }
    
    return 0;
}

int ata_write_sectors(uint32_t lba, const uint8_t* buffer, uint8_t count) {
    // Пока не реализуем запись
    terminal_print("ATA: Write not implemented\n", VGA_COLOR_YELLOW);
    return -1;
}
