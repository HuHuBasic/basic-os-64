/*
 * upgrade.h - 系统升级（版本包应用 + 回滚）
 */
#ifndef UPGRADE_H
#define UPGRADE_H

#include "types.h"

void        upgrade_init(void);
const char *upgrade_current(void);

/* 应用一个包含系统文件的 .BAS 升级包（写入前自动备份，可回滚） */
int  upgrade_apply(const uint8_t *buf, uint32_t size);

/* 回滚到上一次升级前的状态 */
int  upgrade_rollback(void);

/* 构建一个演示用升级包，把系统版本提升到 new_version */
int  upgrade_build_demo(uint8_t *buf, uint32_t max, const char *new_version);

/* 生成下一个版本号字符串（patch+1），写入 out */
void upgrade_next_version(char *out, uint32_t max);

void upgrade_status(void);

#endif /* UPGRADE_H */
