/*
 * pkg.h - .BAS 安装包格式（类似 Android APK 的安装包）
 *
 * 布局：
 *   偏移 0 : magic 'BAS1'
 *   4      : 格式版本
 *   8      : 包版本
 *   12     : name[32]
 *   44     : desc[64]
 *   108    : file_count
 *   112    : 文件表，每项 48 字节：
 *              filename[32] + fstype(4) + offset(4) + size(4) + version(4)
 *   ...    : 各文件数据（offset 相对文件起始）
 */
#ifndef PKG_H
#define PKG_H

#include "types.h"

#define PKG_MAGIC        0x31534142u   /* "BAS1" */
#define PKG_HEADER_SIZE  112
#define PKG_FILE_SIZE    48

typedef int (*pkg_file_cb)(const char *fname, uint32_t fstype,
                           const uint8_t *data, uint32_t size,
                           uint32_t version, void *ctx);

int  pkg_parse(const uint8_t *buf, uint32_t size, pkg_file_cb cb, void *ctx);
int  pkg_install(const uint8_t *buf, uint32_t size);
void pkg_list(void);
int  pkg_uninstall(const char *name);
void pkg_ensure_defaults(void);
int  pkg_build_default(uint8_t *buf, uint32_t max);

#endif /* PKG_H */
