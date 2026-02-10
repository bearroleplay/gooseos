#ifndef ATA_H
#define ATA_H

#include <stdint.h>

// Порты ATA (Primary Bus)
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_FEATURES    0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_SECTOR_NUM  0x1F3
#define ATA_CYL_LOW     0x1F4
#define ATA_CYL_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7
#define ATA_ALT_STATUS  0x3F6

// Команды ATA
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_WRITE_DMA       0xCA
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_IDENTIFY        0xEC

// Статусные биты
#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_RDY     0x40    // Drive ready
#define ATA_SR_DF      0x20    // Drive write fault
#define ATA_SR_DSC     0x10    // Drive seek complete
#define ATA_SR_DRQ     0x08    // Data request ready
#define ATA_SR_CORR    0x04    // Corrected data
#define ATA_SR_IDX     0x02    // Index
#define ATA_SR_ERR     0x01    // Error

// Биты ошибок
#define ATA_ER_BBK     0x80    // Bad block
#define ATA_ER_UNC     0x40    // Uncorrectable data
#define ATA_ER_MC      0x20    // Media changed
#define ATA_ER_IDNF    0x10    // ID mark not found
#define ATA_ER_MCR     0x08    // Media change request
#define ATA_ER_ABRT    0x04    // Command aborted
#define ATA_ER_TK0NF   0x02    // Track 0 not found
#define ATA_ER_AMNF    0x01    // No address mark

// Функции
int ata_init(void);
uint32_t ata_get_disk_size(void);
int ata_read_block(uint32_t lba, uint8_t* buffer);
int ata_write_block(uint32_t lba, const uint8_t* buffer);
int ata_read_sectors(uint32_t lba, uint8_t* buffer, uint8_t count);
int ata_write_sectors(uint32_t lba, const uint8_t* buffer, uint8_t count);

#endif