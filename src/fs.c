/*
 * fs.c - RAM 磁盘块设备 + 极简文件系统实现
 */
#include "fs.h"
#include "string.h"
#include "vga.h"

#define FS_MAGIC       0x31465342u   /* "BSF1" */
#define FS_ENTRY_BYTES 52
#define FS_ENTRY_BLOCKS (((FS_MAX_FILES * FS_ENTRY_BYTES) + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE)
#define FS_DATA_START  (1 + FS_ENTRY_BLOCKS)

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t total_blocks;
    uint32_t file_count;
    uint32_t data_start;
    uint32_t reserved;
} __attribute__((packed)) fs_super_t;

static uint8_t  ramdisk[FS_TOTAL_BLOCKS * FS_BLOCK_SIZE];
static uint8_t  block_used[FS_TOTAL_BLOCKS];
static fs_entry_t entries[FS_MAX_FILES];
static fs_super_t super;
static int      fs_ready = 0;

/* ---------------- 底层块读写 ---------------- */

static void rd_write(uint32_t off, const void *src, uint32_t len)
{
    if (off + len > sizeof(ramdisk)) return;
    memcpy(ramdisk + off, src, len);
}

static void rd_read(uint32_t off, void *dst, uint32_t len)
{
    if (off + len > sizeof(ramdisk)) return;
    memcpy(dst, ramdisk + off, len);
}

static void write_super(void)
{
    rd_write(0, &super, sizeof(super));
}

static void write_entries(void)
{
    rd_write(FS_BLOCK_SIZE, entries, sizeof(entries));
}

static void read_entries(void)
{
    rd_read(FS_BLOCK_SIZE, entries, sizeof(entries));
}

/* ---------------- 块分配 ---------------- */

static void mark_region(uint32_t start, uint32_t count, uint8_t used)
{
    for (uint32_t i = 0; i < count; i++) {
        if (start + i < FS_TOTAL_BLOCKS)
            block_used[start + i] = used;
    }
}

static void rebuild_bitmap(void)
{
    memset(block_used, 0, sizeof(block_used));
    /* 元数据区已被占用 */
    mark_region(0, FS_DATA_START, 1);
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (entries[i].used && entries[i].size > 0) {
            uint32_t nblk = (entries[i].size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
            mark_region(entries[i].start_block, nblk, 1);
        }
    }
}

static int alloc_blocks(uint32_t count, uint32_t *out_start)
{
    if (count == 0) return -1;
    for (uint32_t start = FS_DATA_START; start + count <= FS_TOTAL_BLOCKS; start++) {
        int ok = 1;
        for (uint32_t i = 0; i < count; i++) {
            if (block_used[start + i]) { ok = 0; break; }
        }
        if (ok) {
            mark_region(start, count, 1);
            *out_start = start;
            return 0;
        }
    }
    return -1;
}

static void free_blocks(uint32_t start, uint32_t count)
{
    mark_region(start, count, 0);
}

/* ---------------- 公共 API ---------------- */

void fs_init(void)
{
    fs_super_t tmp;
    rd_read(0, &tmp, sizeof(tmp));

    if (tmp.magic != FS_MAGIC || tmp.total_blocks != FS_TOTAL_BLOCKS) {
        fs_format();
        return;
    }

    super = tmp;
    read_entries();
    rebuild_bitmap();
    fs_ready = 1;
}

int fs_format(void)
{
    memset(ramdisk, 0, sizeof(ramdisk));
    memset(entries, 0, sizeof(entries));

    super.magic        = FS_MAGIC;
    super.version      = 1;
    super.total_blocks = FS_TOTAL_BLOCKS;
    super.file_count   = 0;
    super.data_start   = FS_DATA_START;
    super.reserved     = 0;

    rebuild_bitmap();
    write_super();
    write_entries();
    fs_ready = 1;
    return 0;
}

fs_entry_t *fs_get(int index)
{
    if (index < 0 || index >= FS_MAX_FILES)
        return (fs_entry_t *)0;
    return entries[index].used ? &entries[index] : (fs_entry_t *)0;
}

int fs_count(void)
{
    int n = 0;
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (entries[i].used) n++;
    return n;
}

fs_entry_t *fs_find(const char *name)
{
    if (!name) return (fs_entry_t *)0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (entries[i].used && strcmp(entries[i].name, name) == 0)
            return &entries[i];
    }
    return (fs_entry_t *)0;
}

int fs_exists(const char *name)
{
    return fs_find(name) != (fs_entry_t *)0;
}

static int find_free_entry(void)
{
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (!entries[i].used) return i;
    return -1;
}

int fs_delete(const char *name)
{
    fs_entry_t *e = fs_find(name);
    if (!e) return -1;

    if (e->size > 0) {
        uint32_t nblk = (e->size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        free_blocks(e->start_block, nblk);
    }
    memset(e, 0, sizeof(*e));
    if (super.file_count > 0) super.file_count--;

    write_super();
    write_entries();
    return 0;
}

int fs_create(const char *name, uint8_t type, const void *data, uint32_t size)
{
    if (!fs_ready || !name) return -1;
    if (strlen(name) > FS_NAME_MAX) return -1;

    uint32_t max_bytes = (FS_TOTAL_BLOCKS - FS_DATA_START) * FS_BLOCK_SIZE;
    if (size > max_bytes) return -1;

    if (fs_exists(name))
        fs_delete(name);

    int idx = find_free_entry();
    if (idx < 0) return -1;

    uint32_t nblk = (size + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
    uint32_t start = FS_DATA_START;

    if (nblk > 0) {
        if (alloc_blocks(nblk, &start) != 0)
            return -1;
        rd_write(start * FS_BLOCK_SIZE, data, size);
    }

    fs_entry_t *e = &entries[idx];
    memset(e, 0, sizeof(*e));
    e->used        = 1;
    e->type        = type;
    e->flags       = 0;
    for (uint32_t i = 0; i <= strlen(name) && i < sizeof(e->name); i++)
        e->name[i] = name[i];
    e->start_block = start;
    e->size        = size;
    e->version     = 1;

    super.file_count++;

    write_super();
    write_entries();
    return 0;
}

int fs_write(const char *name, const void *data, uint32_t size)
{
    uint8_t type = FS_TYPE_FILE;
    fs_entry_t *e = fs_find(name);
    if (e) type = e->type;
    return fs_create(name, type, data, size);
}

int fs_read(const char *name, void *buf, uint32_t max, uint32_t *out_size)
{
    fs_entry_t *e = fs_find(name);
    if (!e) return -1;
    if (e->size > max) return -1;

    if (e->size > 0)
        rd_read(e->start_block * FS_BLOCK_SIZE, buf, e->size);
    if (out_size) *out_size = e->size;
    return 0;
}

int fs_set_version(const char *name, uint32_t version)
{
    fs_entry_t *e = fs_find(name);
    if (!e) return -1;
    e->version = version;
    write_entries();
    return 0;
}

uint32_t fs_get_version(const char *name)
{
    fs_entry_t *e = fs_find(name);
    return e ? e->version : 0;
}

const char *fs_type_str(uint8_t type)
{
    switch (type) {
    case FS_TYPE_FILE: return "FILE";
    case FS_TYPE_APP:  return "APP ";
    case FS_TYPE_PKG:  return "PKG ";
    case FS_TYPE_SYS:  return "SYS ";
    default:           return "----";
    }
}

void fs_list_print(void)
{
    int n = fs_count();
    terminal_print("total ");
    terminal_print_dec(n);
    terminal_print(" file(s):\n");

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (!entries[i].used) continue;
        terminal_print("  ");
        terminal_print(fs_type_str(entries[i].type));
        terminal_print(" ");
        terminal_print(entries[i].name);
        terminal_print("  ");
        terminal_print_dec(entries[i].size);
        terminal_print(" B  v");
        terminal_print_dec(entries[i].version);
        terminal_print("\n");
    }
}

uint64_t fs_used_bytes(void)
{
    uint64_t used = 0;
    for (int i = 0; i < FS_MAX_FILES; i++)
        if (entries[i].used)
            used += entries[i].size;
    return used;
}

uint64_t fs_total_bytes(void)
{
    return (uint64_t)(FS_TOTAL_BLOCKS - FS_DATA_START) * FS_BLOCK_SIZE;
}
