#include "ata.h"
#include "libc.h"
#include "terminal.h"
#include "panic.h"

// Ждем готовности контроллера
static void ata_io_wait(void) {
    __asm__ volatile("outb %%al, $0x80" : : "a"(0));
}

// Ожидание BSY=0
static int ata_wait_not_busy(void) {
    uint32_t timeout = 1000000; // 10 секунд
    
    while(timeout--) {
        uint8_t status = inb(ATA_STATUS);
        if((status & ATA_SR_BSY) == 0) {
            return 1;
        }
        ata_io_wait();
    }
    
    terminal_print("ATA: Timeout waiting for not busy\n", VGA_COLOR_YELLOW);
    return 0;
}

// Ожидание DRQ=1 или ERR=1
static int ata_wait_drq(void) {
    uint32_t timeout = 1000000;
    
    while(timeout--) {
        uint8_t status = inb(ATA_STATUS);
        if(status & ATA_SR_DRQ) return 1;
        if(status & ATA_SR_ERR) {
            uint8_t error = inb(ATA_ERROR);
            terminal_print("ATA: Error 0x", VGA_COLOR_RED);
            char buf[4];
            itoa(error, buf, 16);
            terminal_print(buf, VGA_COLOR_RED);
            terminal_print("\n", VGA_COLOR_RED);
            return 0;
        }
        ata_io_wait();
    }
    
    terminal_print("ATA: Timeout waiting for DRQ\n", VGA_COLOR_YELLOW);
    return 0;
}

// Простая инициализация ATA
int ata_init(void) {
    terminal_print("ATA: Initializing... ", VGA_COLOR_CYAN);
    
    // Ждем пока диск не будет готов
    if(!ata_wait_not_busy()) {
        terminal_print("NO DISK\n", VGA_COLOR_YELLOW);
        return 0;
    }
    
    // Выбираем Master drive (LBA)
    outb(ATA_DRIVE_HEAD, 0xE0);
    
    // Читаем статус
    uint8_t status = inb(ATA_STATUS);
    
    // Проверяем что это действительно диск (не 0xFF)
    if(status == 0xFF) {
        terminal_print("NO DRIVE (status=FF)\n", VGA_COLOR_YELLOW);
        return 0;
    }
    
    // Проверяем готовность
    if(!(status & ATA_SR_RDY)) {
        terminal_print("NOT READY\n", VGA_COLOR_YELLOW);
        return 0;
    }
    
    terminal_print("OK (", VGA_COLOR_GREEN);
    
    // Пробуем IDENTIFY
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
    
    // Ждем ответа
    if(!ata_wait_drq()) {
        // Может быть старый диск без IDENTIFY
        terminal_print("NO IDENTIFY)\n", VGA_COLOR_YELLOW);
        // Но все равно считаем что диск есть
        return 1;
    }
    
    // Читаем ответ (первое слово - тип устройства)
    uint16_t data = inw(ATA_DATA);
    
    if(data == 0 || data == 0xFFFF) {
        terminal_print("INVALID RESPONSE)\n", VGA_COLOR_YELLOW);
        return 1; // Но диск есть
    }
    
    // Пропускаем остальные данные
    for(int i = 1; i < 256; i++) {
        inw(ATA_DATA);
    }
    
    terminal_print("IDENTIFY OK)\n", VGA_COLOR_GREEN);
    return 1;
}

// Получить размер диска (в секторах)
uint32_t ata_get_disk_size(void) {
    if(!ata_wait_not_busy()) return 0;
    
    // Пробуем IDENTIFY
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);
    
    if(!ata_wait_drq()) {
        return 0;
    }
    
    uint16_t data[256];
    for(int i = 0; i < 256; i++) {
        data[i] = inw(ATA_DATA);
    }
    
    // LBA28: слова 60-61
    uint32_t lba_sectors = (data[61] << 16) | data[60];
    
    if(lba_sectors == 0 || lba_sectors == 0xFFFFFFFF) {
        // Пробуем угадать размер (старые диски)
        return 1000000; // ~500MB
    }
    
    return lba_sectors;
}

// Чтение одного сектора (512 байт)
int ata_read_block(uint32_t lba, uint8_t* buffer) {
    if(!ata_wait_not_busy()) return 0;
    
    // Устанавливаем параметры LBA28
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_SECTOR_NUM, lba & 0xFF);
    outb(ATA_CYL_LOW, (lba >> 8) & 0xFF);
    outb(ATA_CYL_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    // Команда чтения
    outb(ATA_COMMAND, ATA_CMD_READ_PIO);
    
    // Ждем готовности данных
    if(!ata_wait_drq()) return 0;
    
    // Читаем 256 слов (512 байт)
    uint16_t* buf = (uint16_t*)buffer;
    for(int i = 0; i < 256; i++) {
        buf[i] = inw(ATA_DATA);
    }
    
    return 1;
}

// Запись одного сектора
int ata_write_block(uint32_t lba, const uint8_t* buffer) {
    if(!ata_wait_not_busy()) return 0;
    
    // Устанавливаем параметры LBA28
    outb(ATA_SECTOR_COUNT, 1);
    outb(ATA_SECTOR_NUM, lba & 0xFF);
    outb(ATA_CYL_LOW, (lba >> 8) & 0xFF);
    outb(ATA_CYL_HIGH, (lba >> 16) & 0xFF);
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    
    // Команда записи
    outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);
    
    // Ждем готовности принять данные
    if(!ata_wait_drq()) return 0;
    
    // Записываем 256 слов (512 байт)
    const uint16_t* buf = (const uint16_t*)buffer;
    for(int i = 0; i < 256; i++) {
        outw(ATA_DATA, buf[i]);
    }
    
    // Flush кэш
    outb(ATA_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_not_busy();
    
    return 1;
}

// Чтение нескольких секторов (для совместимости)
int ata_read_sectors(uint32_t lba, uint8_t* buffer, uint8_t count) {
    for(int i = 0; i < count; i++) {
        if(!ata_read_block(lba + i, buffer + i * 512)) {
            return i; // Возвращаем сколько прочитали
        }
    }
    return count;
}

// Запись нескольких секторов
int ata_write_sectors(uint32_t lba, const uint8_t* buffer, uint8_t count) {
    for(int i = 0; i < count; i++) {
        if(!ata_write_block(lba + i, buffer + i * 512)) {
            return i;
        }
    }
    return count;
}