/*
 * ata.c - ATA PIO 驱动实现
 *
 * 使用主通道（0x1F0）主盘（drive 0），28 位 LBA 寻址，轮询模式。
 * 支持 identify 探测、读扇区(0x20)、写扇区(0x30) 与缓存刷新(0xE7)。
 */
#include "ata.h"
#include "ports.h"

#define ATA_DATA      0x1F0
#define ATA_ERROR     0x1F1
#define ATA_SECCOUNT  0x1F2
#define ATA_LBA0      0x1F3
#define ATA_LBA1      0x1F4
#define ATA_LBA2      0x1F5
#define ATA_HDDEVSEL  0x1F6
#define ATA_STATUS    0x1F7
#define ATA_COMMAND   0x1F7
#define ATA_ALTSTATUS 0x3F6
#define ATA_CONTROL   0x3F6

#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF   0x20
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

#define ATA_CMD_READ   0x20
#define ATA_CMD_WRITE  0x30
#define ATA_CMD_FLUSH  0xE7
#define ATA_CMD_IDENT  0xEC

static int detected = 0;

static void ata_io_delay(void)
{
    for (int i = 0; i < 4; i++)
        (void)inb(ATA_ALTSTATUS);
}

/* 等待 BSY 清除；when_drq!=0 时还需等待 DRQ 置位。返回 0 成功。 */
static int ata_wait(int when_drq)
{
    for (int i = 0; i < 1000000; i++) {
        uint8_t st = inb(ATA_STATUS);
        if (st & ATA_SR_BSY)
            continue;
        if (st & (ATA_SR_ERR | ATA_SR_DF))
            return -1;
        if (!when_drq || (st & ATA_SR_DRQ))
            return 0;
    }
    return -1;
}

static void ata_select(uint32_t lba)
{
    outb(ATA_HDDEVSEL, (uint8_t)(0xE0 | ((lba >> 24) & 0x0F)));
    ata_io_delay();
}

static void ata_setup_lba(uint32_t lba, uint8_t count)
{
    outb(ATA_SECCOUNT, count);
    outb(ATA_LBA0, (uint8_t)(lba & 0xFF));
    outb(ATA_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA2, (uint8_t)((lba >> 16) & 0xFF));
}

int ata_init(void)
{
    detected = 0;

    /* 出现即清零中断使能，避免 ATA IRQ 干扰（本驱动用轮询） */
    outb(ATA_CONTROL, 0x02);

    outb(ATA_HDDEVSEL, 0xA0);
    ata_io_delay();

    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA0, 0);
    outb(ATA_LBA1, 0);
    outb(ATA_LBA2, 0);
    outb(ATA_COMMAND, ATA_CMD_IDENT);

    uint8_t st = inb(ATA_STATUS);
    if (st == 0)
        return 0;                 /* 无设备 */

    if (ata_wait(0) != 0)
        return 0;

    /* 非 ATA 设备（如 ATAPI）会在 LBA 中/低寄存器返回非零 */
    if (inb(ATA_LBA1) != 0 || inb(ATA_LBA2) != 0)
        return 0;

    for (int i = 0; i < 100000; i++) {
        st = inb(ATA_STATUS);
        if (st & ATA_SR_BSY) continue;
        if (st & ATA_SR_ERR) return 0;
        if (st & ATA_SR_DRQ) break;
    }
    if (!(inb(ATA_STATUS) & ATA_SR_DRQ))
        return 0;

    for (int i = 0; i < 256; i++)
        (void)inw(ATA_DATA);

    detected = 1;
    return 1;
}

int ata_present(void)
{
    return detected;
}

int ata_read_sector(uint32_t lba, void *buf)
{
    if (!detected) return -1;
    if (ata_wait(0) != 0) return -1;

    ata_select(lba);
    ata_setup_lba(lba, 1);
    outb(ATA_COMMAND, ATA_CMD_READ);

    if (ata_wait(1) != 0) return -1;

    uint16_t *dst = (uint16_t *)buf;
    for (int i = 0; i < 256; i++)
        dst[i] = inw(ATA_DATA);
    return 0;
}

int ata_write_sector(uint32_t lba, const void *buf)
{
    if (!detected) return -1;
    if (ata_wait(0) != 0) return -1;

    ata_select(lba);
    ata_setup_lba(lba, 1);
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    if (ata_wait(1) != 0) return -1;

    const uint16_t *src = (const uint16_t *)buf;
    for (int i = 0; i < 256; i++)
        outw(ATA_DATA, src[i]);

    outb(ATA_COMMAND, ATA_CMD_FLUSH);
    (void)ata_wait(0);
    return 0;
}
