/*
 * fs.h - RAM 磁盘块设备 + 极简文件系统
 *
 * 磁盘布局（每块 512 字节）：
 *   block 0            : 超级块
 *   block 1..7         : 文件表（64 项）
 *   block 8..          : 数据区（连续块分配）
 *
 * 完全自主实现，提供创建/读/写/删除/枚举能力，
 * 供应用(.BAK)、安装包(.BAS)、设置、系统文件使用。
 */
#ifndef FS_H
#define FS_H

#include "types.h"

#define FS_BLOCK_SIZE    512
#define FS_TOTAL_BLOCKS  1024              /* 512 KB 内存盘 */
#define FS_MAX_FILES     64
#define FS_NAME_MAX      31

/* 条目类型 */
#define FS_TYPE_NONE 0
#define FS_TYPE_FILE 1
#define FS_TYPE_APP  2   /* .BAK 可打开应用 */
#define FS_TYPE_PKG  3   /* .BAS 安装包记录 */
#define FS_TYPE_SYS  4   /* 系统/设置文件 */

typedef struct {
    uint8_t  used;
    uint8_t  type;
    uint16_t flags;
    char     name[32];
    uint32_t start_block;
    uint32_t size;
    uint32_t version;
    uint32_t reserved;
} __attribute__((packed)) fs_entry_t;

void        fs_init(void);
int         fs_format(void);

int         fs_create(const char *name, uint8_t type, const void *data, uint32_t size);
int         fs_write(const char *name, const void *data, uint32_t size);
int         fs_read(const char *name, void *buf, uint32_t max, uint32_t *out_size);
int         fs_delete(const char *name);
int         fs_exists(const char *name);
int         fs_set_version(const char *name, uint32_t version);
uint32_t    fs_get_version(const char *name);

fs_entry_t *fs_find(const char *name);
fs_entry_t *fs_get(int index);
int         fs_count(void);

void        fs_list_print(void);
const char *fs_type_str(uint8_t type);

uint64_t    fs_used_bytes(void);
uint64_t    fs_total_bytes(void);

#endif /* FS_H */
