/*
 * settings.h - 系统设置（键值配置，持久化到文件系统）
 */
#ifndef SETTINGS_H
#define SETTINGS_H

#include "types.h"

void settings_init(void);
int  settings_get(const char *key, char *out, uint32_t max);
int  settings_set(const char *key, const char *value);
void settings_list_print(void);
void settings_reset(void);

#endif /* SETTINGS_H */
