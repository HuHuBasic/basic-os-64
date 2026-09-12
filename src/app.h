/*
 * app.h - 应用模型（.BAK）
 *
 * .BAK 是 HU basic OS 的可打开应用格式：
 *   偏移 0   : magic 'BAK1'
 *   4        : 格式版本
 *   8        : kind (0=内置应用, 1=本地机器码)
 *   12       : builtin_id（kind=0 时有效）
 *   16       : payload_size
 *   20       : name[32]
 *   52       : desc[64]
 *   116      : payload（kind=1 时为 x86-64 机器码）
 */
#ifndef APP_H
#define APP_H

#include "types.h"

#define BAK_MAGIC        0x314B4142u   /* "BAK1" */
#define BAK_KIND_BUILTIN 0
#define BAK_KIND_NATIVE  1
#define BAK_HEADER_SIZE  116

typedef struct {
    const char *name;
    const char *desc;
    void      (*fn)(void);
} builtin_app_t;

int                  app_builtin_count(void);
const builtin_app_t *app_builtin_get(int index);

/* 序列化内置应用为 .BAK 映像，返回长度，<0 失败 */
int  app_make_bak(int index, uint8_t *buf, uint32_t max);

void app_list(void);                 /* 枚举文件系统中所有 .BAK */
int  app_run(const char *name);      /* 打开/运行应用 */
int  app_install_bak(const uint8_t *bak, uint32_t size);   /* 写入单个 .BAK */

#endif /* APP_H */
