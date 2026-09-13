/*
 * ata.h - ATA PIO 块设备驱动（主通道主盘，28 位 LBA）
 *
 * 用于把文件系统从纯内存盘升级为真实磁盘持久化。
 * 完全自主实现，不依赖第三方库。
 */
#ifndef ATA_H
#define ATA_H

#include "types.h"

#define ATA_SECTOR_SIZE 512

/* 初始化并探测主通道主盘，返回 1 表示磁盘可用，0 表示不存在 */
int  ata_init(void);
int  ata_present(void);

/* 读写一个 512 字节扇区（lba 为 28 位逻辑块地址），返回 0 成功 */
int  ata_read_sector(uint32_t lba, void *buf);
int  ata_write_sector(uint32_t lba, const void *buf);

#endif /* ATA_H */
