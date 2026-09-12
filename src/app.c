/*
 * app.c - 应用（.BAK）注册表、加载与运行
 */
#include "app.h"
#include "fs.h"
#include "heap.h"
#include "settings.h"
#include "timer.h"
#include "version.h"
#include "string.h"
#include "vga.h"

/* ---------------- 内置应用 ---------------- */

static void app_hello(void)
{
    terminal_print("Hello from a .BAK application!\n");
    terminal_print("This code is running as HU basic OS application code.\n");
}

static void app_sysinfo(void)
{
    terminal_print(BASIC_OS_FULL_STRING "\n");
    terminal_print("kernel : " KERNEL_VERSION_STR "\n");
    terminal_print("build  : " BASIC_OS_BUILD_STR "\n");
    terminal_print("heap   : ");
    terminal_print_dec(heap_used());
    terminal_print(" / ");
    terminal_print_dec(heap_total());
    terminal_print(" bytes\n");
    terminal_print("disk   : ");
    terminal_print_dec(fs_used_bytes());
    terminal_print(" / ");
    terminal_print_dec(fs_total_bytes());
    terminal_print(" bytes, ");
    terminal_print_dec(fs_count());
    terminal_print(" file(s)\n");
    terminal_print("ticks  : ");
    terminal_print_dec(timer_get_ticks());
    terminal_print("\n");
}

static void app_files(void)
{
    fs_list_print();
}

static void app_settings(void)
{
    settings_list_print();
}

static void app_about(void)
{
    terminal_print(BASIC_OS_FULL_STRING "\n");
    terminal_print(BASIC_OS_COPYRIGHT "\n");
    terminal_print("Fully self-developed x86_64 kernel.\n");
}

static const builtin_app_t builtin_apps[] = {
    { "hello",    "Say hello from an application", app_hello },
    { "sysinfo",  "Show system information",       app_sysinfo },
    { "files",    "Browse the file system",        app_files },
    { "settings", "Open system settings",          app_settings },
    { "about",    "About HU basic OS",             app_about },
};

int app_builtin_count(void)
{
    return (int)(sizeof(builtin_apps) / sizeof(builtin_apps[0]));
}

const builtin_app_t *app_builtin_get(int index)
{
    if (index < 0 || index >= app_builtin_count())
        return (const builtin_app_t *)0;
    return &builtin_apps[index];
}

/* ---------------- .BAK 序列化 ---------------- */

static void put_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put_name(uint8_t *dst, const char *src, uint32_t cap)
{
    uint32_t i = 0;
    if (src) {
        for (; src[i] && i < cap - 1; i++)
            dst[i] = (uint8_t)src[i];
    }
    for (; i < cap; i++)
        dst[i] = 0;
}

int app_make_bak(int index, uint8_t *buf, uint32_t max)
{
    const builtin_app_t *a = app_builtin_get(index);
    if (!a || max < BAK_HEADER_SIZE) return -1;

    memset(buf, 0, BAK_HEADER_SIZE);
    put_u32(buf + 0,  BAK_MAGIC);
    put_u32(buf + 4,  1);
    put_u32(buf + 8,  BAK_KIND_BUILTIN);
    put_u32(buf + 12, (uint32_t)index);
    put_u32(buf + 16, 0);
    put_name(buf + 20, a->name, 32);
    put_name(buf + 52, a->desc, 64);
    return BAK_HEADER_SIZE;
}

/* ---------------- 文件名解析 ---------------- */

static void resolve_bak_name(const char *name, char *out, uint32_t max)
{
    uint32_t i = 0;
    for (; name[i] && i < max - 1; i++) out[i] = name[i];
    out[i] = '\0';
    if (i < 4 || out[i - 4] != '.' ||
        (out[i - 3] != 'b' && out[i - 3] != 'B') ||
        (out[i - 2] != 'a' && out[i - 2] != 'A') ||
        (out[i - 1] != 'k' && out[i - 1] != 'K')) {
        if (i + 4 < max) {
            out[i++] = '.';
            out[i++] = 'b';
            out[i++] = 'a';
            out[i++] = 'k';
            out[i] = '\0';
        }
    }
}

/* ---------------- 安装与运行 ---------------- */

int app_install_bak(const uint8_t *bak, uint32_t size)
{
    if (!bak || size < BAK_HEADER_SIZE) return -1;
    if (get_u32(bak + 0) != BAK_MAGIC) return -1;

    char fname[40];
    const char *bname = (const char *)(bak + 20);
    uint32_t i = 0;
    for (; bname[i] && i < 31; i++) fname[i] = bname[i];
    fname[i] = '\0';
    if (i + 5 >= sizeof(fname)) return -1;
    fname[i++] = '.';
    fname[i++] = 'b';
    fname[i++] = 'a';
    fname[i++] = 'k';
    fname[i] = '\0';

    return fs_create(fname, FS_TYPE_APP, bak, size);
}

void app_list(void)
{
    int n = fs_count();
    int shown = 0;
    terminal_print("Installed applications:\n");
    for (int i = 0; i < n; i++) {
        fs_entry_t *e = fs_get(i);
        if (!e || e->type != FS_TYPE_APP) continue;
        terminal_print("  ");
        terminal_print(e->name);
        if (e->size >= BAK_HEADER_SIZE) {
            char tmp[8192];
            uint32_t sz = 0;
            if (e->size <= sizeof(tmp) &&
                fs_read(e->name, tmp, sizeof(tmp), &sz) == 0) {
                terminal_print("  - ");
                terminal_print((const char *)((uint8_t *)tmp + 52));
            }
        }
        terminal_print("\n");
        shown++;
    }
    if (shown == 0)
        terminal_print("  (none)\n");
}

int app_run(const char *name)
{
    char fname[40];
    resolve_bak_name(name, fname, sizeof(fname));

    fs_entry_t *e = fs_find(fname);
    if (!e || e->type != FS_TYPE_APP) {
        terminal_print("app: not found: ");
        terminal_print(fname);
        terminal_print("\n");
        return -1;
    }

    uint8_t *img = (uint8_t *)kmalloc(e->size);
    if (!img) {
        terminal_print("app: out of memory\n");
        return -1;
    }

    uint32_t sz = 0;
    if (fs_read(fname, img, e->size, &sz) != 0 || sz < BAK_HEADER_SIZE) {
        kfree(img);
        terminal_print("app: invalid .BAK\n");
        return -1;
    }

    if (get_u32(img + 0) != BAK_MAGIC) {
        kfree(img);
        terminal_print("app: bad magic (not .BAK)\n");
        return -1;
    }

    uint32_t kind = get_u32(img + 8);

    if (kind == BAK_KIND_BUILTIN) {
        uint32_t id = get_u32(img + 12);
        const builtin_app_t *a = app_builtin_get((int)id);
        if (!a) {
            kfree(img);
            terminal_print("app: unknown builtin id\n");
            return -1;
        }
        terminal_print("[running ");
        terminal_print(a->name);
        terminal_print("]\n");
        a->fn();
        kfree(img);
        return 0;
    }

    if (kind == BAK_KIND_NATIVE) {
        uint32_t code_size = get_u32(img + 16);
        if (code_size == 0 || BAK_HEADER_SIZE + code_size > sz) {
            kfree(img);
            terminal_print("app: bad payload\n");
            return -1;
        }
        uint8_t *code = (uint8_t *)kmalloc(code_size);
        if (!code) {
            kfree(img);
            terminal_print("app: out of memory\n");
            return -1;
        }
        memcpy(code, img + BAK_HEADER_SIZE, code_size);
        terminal_print("[running native ");
        terminal_print(fname);
        terminal_print("]\n");
        void (*entry)(void) = (void (*)(void))code;
        entry();
        kfree(code);
        kfree(img);
        return 0;
    }

    kfree(img);
    terminal_print("app: unsupported kind\n");
    return -1;
}
