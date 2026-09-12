/*
 * upgrade.c - 系统升级实现
 *
 * 升级包就是一个 .BAS 包，里面的文件（type=SYS）被真正覆盖写入文件系统。
 * 覆盖前会把旧文件备份为 rb.<name>，并把文件名记录进 rollback.list；
 * 调用回滚即可恢复旧文件。升级后 system.version 随之改变并持久化。
 */
#include "upgrade.h"
#include "pkg.h"
#include "fs.h"
#include "heap.h"
#include "string.h"
#include "vga.h"
#include "version.h"

#define VER_FILE  "system.version"
#define RB_LIST   "rollback.list"
#define RB_PREFIX "rb."

static char cur_ver[16];

static void wr32_pub(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void wrname_pub(uint8_t *dst, const char *src, uint32_t cap)
{
    uint32_t i = 0;
    if (src) {
        for (; src[i] && i < cap - 1; i++)
            dst[i] = (uint8_t)src[i];
    }
    for (; i < cap; i++)
        dst[i] = 0;
}

static void copy_str(char *dst, const char *src, uint32_t max)
{
    uint32_t i = 0;
    for (; src[i] && i < max - 1; i++) dst[i] = src[i];
    dst[i] = '\0';
}

static void backup_name(const char *fname, char *out, uint32_t max)
{
    uint32_t i = 0;
    const char *pre = RB_PREFIX;
    for (uint32_t j = 0; pre[j] && i < max - 1; j++) out[i++] = pre[j];
    for (uint32_t j = 0; fname[j] && i < max - 1; j++) out[i++] = fname[j];
    out[i] = '\0';
}

/* ---------------- 版本文件 ---------------- */

static void load_version(void)
{
    char tmp[32];
    uint32_t sz = 0;
    if (fs_read(VER_FILE, tmp, sizeof(tmp) - 1, &sz) == 0) {
        tmp[sz] = '\0';
        copy_str(cur_ver, tmp, sizeof(cur_ver));
    }
}

void upgrade_init(void)
{
    if (!fs_exists(VER_FILE)) {
        copy_str(cur_ver, BASIC_OS_VERSION_STR, sizeof(cur_ver));
        fs_create(VER_FILE, FS_TYPE_SYS, cur_ver, strlen(cur_ver));
        return;
    }
    load_version();
}

const char *upgrade_current(void)
{
    return cur_ver;
}

/* ---------------- 回滚记录 ---------------- */

static int list_contains(const char *list, uint32_t len, const char *name)
{
    uint32_t i = 0, n = strlen(name);
    while (i < len) {
        uint32_t s = i;
        while (i < len && list[i] != '\n') i++;
        if (i - s == n && memcmp(list + s, name, n) == 0) return 1;
        i++;
    }
    return 0;
}

static void append_rollback(const char *fname)
{
    static char list[4096];
    uint32_t len = 0;

    if (fs_exists(RB_LIST)) {
        uint32_t sz = 0;
        if (fs_read(RB_LIST, list, sizeof(list) - 1, &sz) == 0)
            len = sz;
    }
    if (list_contains(list, len, fname)) return;

    uint32_t n = strlen(fname);
    if (len + n + 1 < sizeof(list)) {
        memcpy(list + len, fname, n);
        len += n;
        list[len++] = '\n';
        fs_create(RB_LIST, FS_TYPE_SYS, list, len);
    }
}

/* 清除上一轮的回滚点（删除旧备份文件与清单） */
static void clear_rollback(void)
{
    if (!fs_exists(RB_LIST)) return;

    char list[4096];
    uint32_t sz = 0;
    if (fs_read(RB_LIST, list, sizeof(list) - 1, &sz) != 0) {
        fs_delete(RB_LIST);
        return;
    }

    uint32_t i = 0;
    while (i < sz) {
        uint32_t s = i;
        while (i < sz && list[i] != '\n') i++;
        list[i] = '\0';
        if (i > s) {
            char rb[48];
            backup_name(list + s, rb, sizeof(rb));
            fs_delete(rb);
        }
        i++;
    }
    fs_delete(RB_LIST);
}

/* ---------------- 升级应用 ---------------- */

typedef struct {
    int failed;
} apply_ctx_t;

static int apply_cb(const char *fname, uint32_t fstype,
                    const uint8_t *data, uint32_t size,
                    uint32_t version, void *ctx)
{
    apply_ctx_t *ac = (apply_ctx_t *)ctx;

    fs_entry_t *old = fs_find(fname);
    if (old) {
        uint8_t *oldbuf = (uint8_t *)kmalloc(old->size + 1);
        if (oldbuf) {
            uint32_t osz = 0;
            if (fs_read(fname, oldbuf + 1, old->size, &osz) == 0) {
                oldbuf[0] = old->type;
                char rb[48];
                backup_name(fname, rb, sizeof(rb));
                fs_create(rb, FS_TYPE_SYS, oldbuf, osz + 1);
            }
            kfree(oldbuf);
        }
    }
    /* 无论文件原先是否存在都记入回滚清单：
     * 原先存在 → 回滚恢复；原先不存在 → 回滚删除。 */
    append_rollback(fname);

    if (fs_create(fname, (uint8_t)fstype, data, size) != 0) {
        ac->failed = 1;
        return -1;
    }
    fs_set_version(fname, version ? version : 1);
    return 0;
}

int upgrade_apply(const uint8_t *buf, uint32_t size)
{
    if (pkg_parse(buf, size, (pkg_file_cb)0, (void *)0) != 0) {
        terminal_print("upgrade: invalid package\n");
        return -1;
    }

    clear_rollback();

    apply_ctx_t ac;
    ac.failed = 0;
    if (pkg_parse(buf, size, apply_cb, &ac) != 0 || ac.failed) {
        terminal_print("upgrade: apply failed\n");
        return -1;
    }

    load_version();
    terminal_print("upgrade: system upgraded to v");
    terminal_print(cur_ver);
    terminal_print("\n");
    return 0;
}

int upgrade_rollback(void)
{
    if (!fs_exists(RB_LIST)) {
        terminal_print("upgrade: no rollback point available\n");
        return -1;
    }

    char list[4096];
    uint32_t sz = 0;
    if (fs_read(RB_LIST, list, sizeof(list) - 1, &sz) != 0) {
        terminal_print("upgrade: cannot read rollback list\n");
        return -1;
    }

    static uint8_t buf[8192];
    uint32_t i = 0;
    int restored = 0;

    while (i < sz) {
        uint32_t s = i;
        while (i < sz && list[i] != '\n') i++;
        list[i] = '\0';

        if (i > s) {
            const char *fname = list + s;
            char rb[48];
            backup_name(fname, rb, sizeof(rb));

            uint32_t bsz = 0;
            if (fs_exists(rb) &&
                fs_read(rb, buf, sizeof(buf), &bsz) == 0 && bsz > 0) {
                fs_create(fname, buf[0], buf + 1, bsz - 1);
                fs_delete(rb);
            } else {
                /* 升级时新加入的文件：回滚即删除 */
                fs_delete(fname);
            }
            restored++;
        }
        i++;
    }

    fs_delete(RB_LIST);
    load_version();

    terminal_print("upgrade: rolled back ");
    terminal_print_dec(restored);
    terminal_print(" file(s), now v");
    terminal_print(cur_ver);
    terminal_print("\n");
    return 0;
}

/* ---------------- 演示升级包 ---------------- */

static uint32_t ver_num(const char *s)
{
    uint32_t parts[3] = { 0, 0, 0 };
    int pi = 0;
    for (uint32_t i = 0; s[i] && pi < 3; i++) {
        if (s[i] == '.') { pi++; continue; }
        if (s[i] >= '0' && s[i] <= '9')
            parts[pi] = parts[pi] * 10 + (uint32_t)(s[i] - '0');
    }
    return parts[0] * 10000 + parts[1] * 100 + parts[2];
}

void upgrade_next_version(char *out, uint32_t max)
{
    uint32_t maj = 0, min = 0, pat = 0;
    uint32_t p[3] = { 0, 0, 0 };
    int pi = 0;
    for (uint32_t i = 0; cur_ver[i] && pi < 3; i++) {
        if (cur_ver[i] == '.') { pi++; continue; }
        if (cur_ver[i] >= '0' && cur_ver[i] <= '9')
            p[pi] = p[pi] * 10 + (uint32_t)(cur_ver[i] - '0');
    }
    maj = p[0]; min = p[1]; pat = p[2] + 1;

    char tmp[16];
    uint32_t n = 0;
    int64_t vals[3] = { (int64_t)maj, (int64_t)min, (int64_t)pat };
    for (int k = 0; k < 3; k++) {
        char digits[12];
        itoa(vals[k], digits, 10);
        for (uint32_t j = 0; digits[j] && n < sizeof(tmp) - 1; j++)
            tmp[n++] = digits[j];
        if (k < 2 && n < sizeof(tmp) - 1) tmp[n++] = '.';
    }
    tmp[n] = '\0';
    copy_str(out, tmp, max);
}

int upgrade_build_demo(uint8_t *buf, uint32_t max, const char *new_version)
{
    typedef struct { const char *name; const char *data; } item_t;
    char motd[96];
    uint32_t mn = 0;
    const char *mp = "Welcome to HU basic OS v";
    for (uint32_t i = 0; mp[i] && mn < sizeof(motd) - 32; i++) motd[mn++] = mp[i];
    for (uint32_t i = 0; new_version[i] && mn < sizeof(motd) - 2; i++) motd[mn++] = new_version[i];
    motd[mn++] = '\n';
    motd[mn] = '\0';

    item_t items[3];
    items[0].name = VER_FILE;
    items[0].data = new_version;
    items[1].name = "motd.txt";
    items[1].data = motd;
    items[2].name = "update.marker";
    items[2].data = "applied\n";

    uint32_t cnt = 3;
    uint32_t data_off = PKG_HEADER_SIZE + cnt * PKG_FILE_SIZE;
    if (data_off > max) return -1;

    memset(buf, 0, PKG_HEADER_SIZE);
    wr32_pub(buf + 0, PKG_MAGIC);
    wr32_pub(buf + 4, 1);
    wr32_pub(buf + 8, ver_num(new_version));
    wrname_pub(buf + 12, "system-update", 32);
    wrname_pub(buf + 44, "HU basic OS system update", 64);
    wr32_pub(buf + 108, cnt);

    uint32_t off = data_off;
    for (uint32_t i = 0; i < cnt; i++) {
        uint32_t len = strlen(items[i].data);
        if (off + len > max) return -1;
        memcpy(buf + off, items[i].data, len);

        uint8_t *fe = buf + PKG_HEADER_SIZE + i * PKG_FILE_SIZE;
        wrname_pub(fe, items[i].name, 32);
        wr32_pub(fe + 32, FS_TYPE_SYS);
        wr32_pub(fe + 36, off);
        wr32_pub(fe + 40, len);
        wr32_pub(fe + 44, ver_num(new_version));

        off += len;
    }
    return (int)off;
}

void upgrade_status(void)
{
    terminal_print("System version : v");
    terminal_print(cur_ver);
    terminal_print("\n");
    terminal_print("Kernel build   : " BASIC_OS_BUILD_STR "\n");
    terminal_print("Rollback point : ");
    terminal_print(fs_exists(RB_LIST) ? "available" : "none");
    terminal_print("\n");
}
