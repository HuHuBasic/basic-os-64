/*
 * pkg.c - .BAS 安装包的解析、安装、卸载
 */
#include "pkg.h"
#include "app.h"
#include "fs.h"
#include "string.h"
#include "vga.h"

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void wrname(uint8_t *dst, const char *src, uint32_t cap)
{
    uint32_t i = 0;
    if (src) {
        for (; src[i] && i < cap - 1; i++)
            dst[i] = (uint8_t)src[i];
    }
    for (; i < cap; i++)
        dst[i] = 0;
}

int pkg_parse(const uint8_t *buf, uint32_t size, pkg_file_cb cb, void *ctx)
{
    if (!buf || size < PKG_HEADER_SIZE) return -1;
    if (rd32(buf + 0) != PKG_MAGIC) return -1;

    uint32_t file_count = rd32(buf + 108);
    if (file_count > 1024) return -1;
    if ((uint64_t)PKG_HEADER_SIZE + (uint64_t)file_count * PKG_FILE_SIZE > size)
        return -1;

    for (uint32_t i = 0; i < file_count; i++) {
        const uint8_t *fe = buf + PKG_HEADER_SIZE + i * PKG_FILE_SIZE;
        const char *fname = (const char *)fe;
        uint32_t fstype = rd32(fe + 32);
        uint32_t off    = rd32(fe + 36);
        uint32_t fsz    = rd32(fe + 40);
        uint32_t ver    = rd32(fe + 44);

        if ((uint64_t)off + fsz > size) return -1;

        if (cb && cb(fname, fstype, buf + off, fsz, ver, ctx) != 0)
            return -1;
    }
    return 0;
}

/* ---------------- 安装 ---------------- */

typedef struct {
    char     list[4096];
    uint32_t len;
} install_ctx_t;

static int install_cb(const char *fname, uint32_t fstype,
                      const uint8_t *data, uint32_t size,
                      uint32_t version, void *ctx)
{
    install_ctx_t *ic = (install_ctx_t *)ctx;

    if (fs_create(fname, (uint8_t)fstype, data, size) != 0)
        return -1;
    fs_set_version(fname, version ? version : 1);

    uint32_t n = strlen(fname);
    if (ic->len + n + 1 < sizeof(ic->list)) {
        memcpy(ic->list + ic->len, fname, n);
        ic->len += n;
        ic->list[ic->len++] = '\n';
    }
    return 0;
}

static void record_name(const char *pkg, char *out, uint32_t max)
{
    const char *pre = "pkg.";
    uint32_t i = 0;
    for (; pre[i] && i < max - 1; i++) out[i] = pre[i];
    for (uint32_t j = 0; pkg[j] && i < max - 1; j++) out[i++] = pkg[j];
    out[i] = '\0';
}

int pkg_install(const uint8_t *buf, uint32_t size)
{
    if (pkg_parse(buf, size, (pkg_file_cb)0, (void *)0) != 0) {
        terminal_print("pkg: invalid .BAS package\n");
        return -1;
    }

    install_ctx_t *ic = (install_ctx_t *)0;
    static install_ctx_t ctx;
    ic = &ctx;
    memset(ic, 0, sizeof(*ic));

    if (pkg_parse(buf, size, install_cb, ic) != 0) {
        terminal_print("pkg: installation failed\n");
        return -1;
    }

    char pname[40], rec[48];
    const char *src = (const char *)(buf + 12);
    uint32_t i = 0;
    for (; src[i] && i < 31; i++) pname[i] = src[i];
    pname[i] = '\0';
    record_name(pname, rec, sizeof(rec));

    fs_create(rec, FS_TYPE_PKG, ic->list, ic->len);
    terminal_print("pkg: installed '");
    terminal_print(pname);
    terminal_print("'.\n");
    return 0;
}

void pkg_list(void)
{
    terminal_print("Installed packages:\n");
    int shown = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        fs_entry_t *e = fs_get(i);
        if (!e || e->type != FS_TYPE_PKG) continue;

        char content[4096];
        uint32_t sz = 0;
        int files = 0;
        if (e->size <= sizeof(content) &&
            fs_read(e->name, content, sizeof(content), &sz) == 0) {
            for (uint32_t j = 0; j < sz; j++)
                if (content[j] == '\n') files++;
        }

        const char *disp = e->name;
        if (disp[0] == 'p' && disp[1] == 'k' && disp[2] == 'g' && disp[3] == '.')
            disp += 4;

        terminal_print("  ");
        terminal_print(disp);
        terminal_print("  (");
        terminal_print_dec(files);
        terminal_print(" file(s))\n");
        shown++;
    }
    if (shown == 0)
        terminal_print("  (none)\n");
}

int pkg_uninstall(const char *name)
{
    char rec[48];
    record_name(name, rec, sizeof(rec));

    fs_entry_t *e = fs_find(rec);
    if (!e) {
        terminal_print("pkg: not installed: ");
        terminal_print(name);
        terminal_print("\n");
        return -1;
    }

    char content[4096];
    uint32_t sz = 0;
    if (e->size > sizeof(content) ||
        fs_read(rec, content, sizeof(content), &sz) != 0) {
        terminal_print("pkg: cannot read package record\n");
        return -1;
    }

    uint32_t i = 0;
    while (i < sz) {
        uint32_t start = i;
        while (i < sz && content[i] != '\n') i++;
        content[i] = '\0';
        if (i > start)
            fs_delete(content + start);
        i++;
    }

    fs_delete(rec);
    terminal_print("pkg: uninstalled '");
    terminal_print(name);
    terminal_print("'.\n");
    return 0;
}

/* ---------------- 构建内置系统包 ---------------- */

int pkg_build_default(uint8_t *buf, uint32_t max)
{
    int count = app_builtin_count();
    uint32_t data_off = PKG_HEADER_SIZE + (uint32_t)count * PKG_FILE_SIZE;
    if (data_off > max) return -1;

    memset(buf, 0, PKG_HEADER_SIZE);
    wr32(buf + 0, PKG_MAGIC);
    wr32(buf + 4, 1);
    wr32(buf + 8, 1);
    wrname(buf + 12, "system", 32);
    wrname(buf + 44, "HU basic OS built-in applications", 64);
    wr32(buf + 108, (uint32_t)count);

    uint32_t off = data_off;
    for (int i = 0; i < count; i++) {
        const builtin_app_t *a = app_builtin_get(i);
        uint8_t *fe = buf + PKG_HEADER_SIZE + (uint32_t)i * PKG_FILE_SIZE;

        char fname[40];
        uint32_t p = 0;
        for (uint32_t j = 0; a->name[j] && p < 31; j++) fname[p++] = a->name[j];
        fname[p++] = '.';
        fname[p++] = 'b';
        fname[p++] = 'a';
        fname[p++] = 'k';
        fname[p] = '\0';

        if (off + BAK_HEADER_SIZE > max) return -1;
        int bak_len = app_make_bak(i, buf + off, max - off);
        if (bak_len < 0) return -1;

        wrname(fe, fname, 32);
        wr32(fe + 32, FS_TYPE_APP);
        wr32(fe + 36, off);
        wr32(fe + 40, (uint32_t)bak_len);
        wr32(fe + 44, 1);

        off += (uint32_t)bak_len;
    }
    return (int)off;
}

void pkg_ensure_defaults(void)
{
    if (fs_exists("pkg.system")) return;

    static uint8_t buf[8192];
    int n = pkg_build_default(buf, sizeof(buf));
    if (n <= 0) return;
    pkg_install(buf, (uint32_t)n);
}
