/* gguf.c — GGUF parser implementation */
#include "gguf.h"
#include <stdint.h>
#include <string.h>

/* Minimal libc for HillsonOS */
extern void *sbrk(int increment);
extern int printf(const char *fmt, ...);
extern int open(const char *path, int flags);
extern int read(int fd, char *buf, int len);
extern int close(int fd);
extern int lseek(int fd, int offset, int whence);

static char *hp = NULL;
static char *hp_end = NULL;
static void *hmalloc(int sz) {
    if (sz <= 0) return NULL;
    int aligned = (sz + 15) & ~15;
    if (!hp || hp + aligned > hp_end) {
        int grow = aligned > 4*1024*1024 ? aligned : 4*1024*1024;
        grow = (grow + 4095) & ~4095;
        char *block = (char *)sbrk(grow);
        if (!block) return NULL;
        hp = block;
        hp_end = block + grow;
    }
    void *p = hp;
    hp += aligned;
    return p;
}

/* File I/O through libuser so syscall numbers stay in one place. */
static int gguf_open_file(const char *path) {
    return open(path, 0);
}
static int gguf_read(int fd, void *buf, int sz) {
    return read(fd, (char *)buf, sz);
}
static void gguf_close_file(int fd) {
    close(fd);
}

static uint32_t read_u32(uint8_t *p) { return p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24); }
static uint64_t read_u64(uint8_t *p) {
    return (uint64_t)read_u32(p) | ((uint64_t)read_u32(p+4) << 32);
}
static int read_str(int fd, char *out, int max) {
    uint64_t len;
    if(gguf_read(fd, &len, 8) != 8) return -1;
    len = read_u64((uint8_t*)&len);
    if(len > (uint64_t)(max-1)) len = max-1;
    if(gguf_read(fd, out, (int)len) != (int)len) return -1;
    out[len] = 0;
    return (int)len;
}

int gguf_open(const char *path, gguf_ctx_t *ctx) {
    int fd = gguf_open_file(path);
    if(fd == -1) { printf("[GGUF] cannot open %s\n", path); return -1; }
    printf("[GGUF] opened %s fd=%x\n", path, fd);
    ctx->fd = fd;

    /* Read header */
    uint8_t hdr[24];
    if(gguf_read(fd, hdr, 24) != 24) { gguf_close_file(fd); return -1; }
    uint32_t magic = read_u32(hdr);
    if(magic != GGUF_MAGIC) { printf("[GGUF] bad magic %x\n", magic); gguf_close_file(fd); return -1; }
    ctx->version   = read_u32(hdr+4);
    ctx->n_tensors = read_u64(hdr+8);
    ctx->n_kv      = read_u64(hdr+16);
    printf("[GGUF] v%d, %lld tensors, %lld kv\n", ctx->version, ctx->n_tensors, ctx->n_kv);

    /* Read metadata KV pairs */
    uint64_t i;
    for(i = 0; i < ctx->n_kv; i++) {
        char key[256], val_str[256];
        if(read_str(fd, key, sizeof(key)) < 0) break;
        uint32_t vtype;
        if(gguf_read(fd, &vtype, 4) != 4) break;
        vtype = read_u32((uint8_t*)&vtype);

        /* Parse known keys */
        if(vtype == GGUF_TYPE_STRING) {
            read_str(fd, val_str, sizeof(val_str));
        } else if(vtype == GGUF_TYPE_UINT32) {
            uint32_t v; gguf_read(fd, &v, 4);
        } else if(vtype == GGUF_TYPE_FLOAT32) {
            float f; gguf_read(fd, &f, 4);
        } else {
            /* Skip unknown types */
            uint8_t skip[256];
            if(vtype == GGUF_TYPE_BOOL) { gguf_read(fd, skip, 1); }
            else if(vtype == GGUF_TYPE_ARRAY) {
                uint32_t atype, alen;
                gguf_read(fd, &atype, 4); atype=read_u32((uint8_t*)&atype);
                gguf_read(fd, &alen, 4);  alen=read_u32((uint8_t*)&alen);
                for(uint32_t j=0; j<alen; j++) { gguf_read(fd, skip, 4); }
            }
        }

        /* Extract metadata */
        int eq=0; while(key[eq] && key[eq]!='=') eq++;
        char *val = key+eq+1; if(eq>0) key[eq]=0;

        if(vtype==GGUF_TYPE_STRING) {
            if(eq>0) {
                if(!strcmp(key,"general.name")) { int j=0; while(val_str[j]&&j<63){ctx->name[j]=val_str[j];j++;} ctx->name[j]=0; }
                if(!strcmp(key,"general.architecture")) { int j=0; while(val_str[j]&&j<31){ctx->arch[j]=val_str[j];j++;} ctx->arch[j]=0; }
            }
        }
    }

    /* Read tensor infos */
    ctx->tensors = (gguf_tensor_t*)hmalloc(sizeof(gguf_tensor_t) * ctx->n_tensors);
    for(i = 0; i < ctx->n_tensors; i++) {
        gguf_tensor_t *t = &ctx->tensors[i];
        if(read_str(fd, t->name, sizeof(t->name)) < 0) break;
        uint8_t tbuf[48];
        if(gguf_read(fd, tbuf, 48) != 48) break;
        t->n_dims   = read_u32(tbuf);
        for(int j=0; j<4; j++) t->shape[j] = read_u64(tbuf+4+j*8);
        t->ggml_type = read_u32(tbuf+36);
        t->offset    = read_u64(tbuf+40);
    }

    /* Tensor offsets are relative to the aligned GGUF data section. */
    int tensor_info_end = lseek(fd, 0, 1);
    if (tensor_info_end < 0) return -1;
    ctx->data_offset = ((uint64_t)(uint32_t)tensor_info_end + 31) & ~31ULL;
    ctx->data = NULL; /* Use seek+read instead of mmap */
    printf("[GGUF] loaded %s (%s), %lld tensors, data=%x\n",
           ctx->name, ctx->arch, ctx->n_tensors, (uint32_t)ctx->data_offset);
    return 0;
}

void gguf_close(gguf_ctx_t *ctx) {
    if(ctx->fd != -1) gguf_close_file(ctx->fd);
    ctx->fd = -1;
}

int gguf_get_tensor(gguf_ctx_t *ctx, const char *name, void **data, uint64_t *nbytes) {
    for(uint64_t i = 0; i < ctx->n_tensors; i++) {
        gguf_tensor_t *t = &ctx->tensors[i];
        /* Simple name match */
        int m=1, j=0;
        while(name[j] && t->name[j] && j<127) { if(name[j]!=t->name[j]){m=0;break;} j++; }
        if(m && !name[j] && !t->name[j]) {
            uint64_t sz = 1;
            for(int d=0; d<(int)t->n_dims; d++) sz *= t->shape[d];
            /* element size depends on type */
            if(t->ggml_type == 0) sz *= 4;      /* F32 */
            else if(t->ggml_type == 10) sz /= 2; /* Q4_K_M: ~0.5 bytes/elem */
            else sz *= 2; /* F16 default */
            *nbytes = sz;
            *data = NULL; /* caller must read from file at t->offset */
            return 0;
        }
    }
    return -1;
}

/* strcmp from libc */
