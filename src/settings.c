/*
 * settings.c - 系统设置实现
 *
 * 配置以 "key=value\n" 文本形式保存在文件系统 settings.cfg 中，
 * 修改后立即写回，重启（RAM 盘范围内）后仍可读取。
 */
#include "settings.h"
#include "fs.h"
#include "string.h"
#include "vga.h"

#define SETTINGS_FILE "settings.cfg"
#define SETTINGS_MAX  2048

static char     cfg[SETTINGS_MAX];
static uint32_t cfg_len = 0;

static const char *defaults =
    "hostname=basic64\n"
    "theme=dark\n"
    "timer_hz=100\n"
    "boot_animation=1\n"
    "language=zh-CN\n"
    "developer_mode=1\n";

static void persist(void)
{
    fs_create(SETTINGS_FILE, FS_TYPE_SYS, cfg, cfg_len);
}

void settings_init(void)
{
    uint32_t sz = 0;
    if (fs_exists(SETTINGS_FILE) &&
        fs_read(SETTINGS_FILE, cfg, SETTINGS_MAX - 1, &sz) == 0) {
        cfg[sz] = '\0';
        cfg_len = sz;
        return;
    }

    cfg_len = strlen(defaults);
    memcpy(cfg, defaults, cfg_len + 1);
    persist();
}

/* 在配置中查找 key，返回 value 起始下标；未找到返回 -1 */
static int find_key(const char *key)
{
    uint64_t klen = strlen(key);
    uint32_t i = 0;
    while (i < cfg_len) {
        uint32_t line_start = i;
        while (i < cfg_len && cfg[i] != '\n') i++;
        uint32_t line_len = i - line_start;
        if (line_len > klen && cfg[line_start + klen] == '=' &&
            memcmp(cfg + line_start, key, klen) == 0)
            return line_start + klen + 1;
        if (i < cfg_len) i++;
    }
    return -1;
}

int settings_get(const char *key, char *out, uint32_t max)
{
    int pos = find_key(key);
    if (pos < 0) return -1;

    uint32_t i = 0;
    while ((uint32_t)pos + i < cfg_len && cfg[pos + i] != '\n' && i < max - 1) {
        out[i] = cfg[pos + i];
        i++;
    }
    out[i] = '\0';
    return 0;
}

int settings_set(const char *key, const char *value)
{
    if (!key || !value) return -1;

    char line[256];
    uint64_t klen = strlen(key), vlen = strlen(value);
    if (klen + vlen + 2 >= sizeof(line)) return -1;

    uint32_t p = 0;
    for (uint64_t i = 0; i < klen; i++) line[p++] = key[i];
    line[p++] = '=';
    for (uint64_t i = 0; i < vlen; i++) line[p++] = value[i];
    line[p++] = '\n';

    int pos = find_key(key);
    if (pos < 0) {
        /* 追加到末尾 */
        if (cfg_len + p >= SETTINGS_MAX) return -1;
        memcpy(cfg + cfg_len, line, p);
        cfg_len += p;
        cfg[cfg_len] = '\0';
        persist();
        return 0;
    }

    /* 定位整行：value 起点 - key长度 - '=' 即行首 */
    uint32_t key_start = (uint32_t)pos - (uint32_t)klen - 1;

    uint32_t line_end = (uint32_t)pos;
    while (line_end < cfg_len && cfg[line_end] != '\n') line_end++;
    if (line_end < cfg_len) line_end++;   /* 含换行 */

    char tmp[SETTINGS_MAX];
    uint32_t n = 0;
    memcpy(tmp + n, cfg, key_start);
    n += key_start;
    memcpy(tmp + n, line, p);
    n += p;
    memcpy(tmp + n, cfg + line_end, cfg_len - line_end);
    n += cfg_len - line_end;
    memcpy(cfg, tmp, n);
    cfg_len = n;
    cfg[cfg_len] = '\0';
    persist();
    return 0;
}

void settings_list_print(void)
{
    terminal_print("Settings (" SETTINGS_FILE "):\n");
    for (uint32_t i = 0; i < cfg_len; i++) {
        terminal_putchar(cfg[i]);
    }
    terminal_print("\n");
}

void settings_reset(void)
{
    cfg_len = strlen(defaults);
    memcpy(cfg, defaults, cfg_len + 1);
    persist();
    terminal_print("Settings reset to defaults.\n");
}
