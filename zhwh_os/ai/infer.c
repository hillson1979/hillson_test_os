/*
 * ai/infer.c — IBM Granite 3.0 Nano Inference Engine
 */
#include "transformer.h"
#include "gguf.h"
#include "matmul.h"
#include <stdint.h>

extern void *sbrk(int increment);
extern int lsdisk(const char *path, char *buf, int max);
extern int read(int fd, char *buf, int len);
extern int lseek(int fd, int offset, int whence);

static char *hp = NULL;
static char *hp_end = NULL;

void *malloc(int sz) {
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
void free(void *p) { (void)p; }

extern int printf(const char *fmt, ...);

/* Select the first GGUF-like file reported by the mounted USB filesystem. */
static int find_model_path(char *out, int out_sz) {
    char listing[2048];
    int n = lsdisk("/usb", listing, sizeof(listing));
    if (n <= 0) return -1;
    printf("[AI] /usb:\n%s", listing);
    for (int i = 0; i < n;) {
        char name[128];
        int p = 0;
        while (i < n && listing[i] == '\n') i++;
        while (i < n && listing[i] != '\n' && listing[i] != ' ' && p < (int)sizeof(name)-1)
            name[p++] = listing[i++];
        name[p] = 0;
        while (i < n && listing[i] != '\n') i++;
        if (p >= 4) {
            int a = p - 4;
            int short_ggu = name[a] == '.' &&
                (name[p-1] == 'U' || name[p-1] == 'u') &&
                (name[p-2] == 'G' || name[p-2] == 'g') &&
                (name[p-3] == 'G' || name[p-3] == 'g');
            int long_gguf = p >= 5 && name[p-5] == '.' &&
                (name[p-4] == 'G' || name[p-4] == 'g') &&
                (name[p-3] == 'G' || name[p-3] == 'g') &&
                (name[p-2] == 'U' || name[p-2] == 'u') &&
                (name[p-1] == 'F' || name[p-1] == 'f');
            if (short_ggu || long_gguf) {
                int q = 0;
                const char *prefix = "/usb/";
                while (prefix[q] && q < out_sz-1) { out[q] = prefix[q]; q++; }
                for (int j = 0; j < p && q < out_sz-1; j++) out[q++] = name[j];
                out[q] = 0;
                return 0;
            }
        }
        if (i < n) i++;
    }
    return -1;
}

/* Minimal snprintf for tensor name formatting */
static void fmt_name(char *buf, int sz, const char *pfx, int num, const char *sfx) {
    int p = 0;
    while (*pfx && p < sz-1) buf[p++] = *pfx++;
    if (num >= 100)  { buf[p++] = '0' + num/100; num %= 100; }
    if (num >= 10)   { buf[p++] = '0' + num/10;  num %= 10; }
    buf[p++] = '0' + num;
    while (*sfx && p < sz-1) buf[p++] = *sfx++;
    buf[p] = 0;
}

/* Load weight tensor from GGUF file by name */
static int load_q4(gguf_ctx_t *ctx, const char *name, void **ptr) {
    for (uint64_t i = 0; i < ctx->n_tensors; i++) {
        gguf_tensor_t *t = &ctx->tensors[i];
        int m = 1, j = 0;
        while (name[j] && t->name[j] && j < 127) { if (name[j] != t->name[j]) { m = 0; break; } j++; }
        if (!m || name[j] || t->name[j]) continue;

        int rows = (int)t->shape[0], cols = (t->n_dims > 1) ? (int)t->shape[1] : 1;
        int n_elem = rows * cols;
        int sz;
        if (t->ggml_type == 10) /* Q4_K_M */
            sz = (n_elem + 1) / 2 + (n_elem / 256) * 144;
        else if (t->ggml_type == 0) /* F32 */
            sz = n_elem * 4;
        else /* F16 */
            sz = n_elem * 2;

        *ptr = malloc(sz);
        if (!*ptr) return -1;
        if (ctx->fd != -1) {
            int fd = ctx->fd;
            printf("[AI] loading %s (%d bytes)\n", name, sz);
            uint64_t absolute_offset = ctx->data_offset + t->offset;
            if (absolute_offset > 0x7fffffffULL) return -1;
            lseek(fd, (int)absolute_offset, 0);
            int done = 0;
            while (done < sz) {
                int got = read(fd, (char *)*ptr + done, sz - done);
                if (got <= 0) return -1;
                done += got;
                if ((done & ((16 * 1024 * 1024) - 1)) == 0)
                    printf("[AI] loaded %d/%d bytes\n", done, sz);
            }
        }
        return 0;
    }
    return -1;
}

int main(int argc, char **argv) {
    printf("[AI] IBM Granite 3.0 Nano Inference\n");
    const char *prompt = "Hello";
    if (argc > 1 && argv[1] && argv[1][0])
        prompt = argv[1];
    else if (argc > 0 && argv[0] && argv[0][0])
        prompt = argv[0];
    printf("[AI] prompt: %s\n", prompt);

    char model_path[160];
    if (find_model_path(model_path, sizeof(model_path)) < 0) {
        printf("[AI] no GGUF model found under /usb\n");
        return -1;
    }
    printf("[AI] model: %s\n", model_path);
    gguf_ctx_t ctx;
    if (gguf_open(model_path, &ctx) < 0) {
        printf("[AI] model open failed: %s\n", model_path);
        return -1;
    }

    model_t m = {0};
    m.n_vocab   = 49152;
    m.n_embd    = 2048;
    m.n_head    = 32;
    m.n_head_kv = 8;
    m.n_layer   = 40;
    m.n_ff      = 8192;
    m.n_ctx     = 256;
    m.norm_eps  = 1e-5f;
    m.rope_theta = 500000.0f;

    printf("[AI] %d layers, d=%d, vocab=%d\n", m.n_layer, m.n_embd, m.n_vocab);

    /* Allocate per-layer weight arrays */
    m.attn_norm = malloc(m.n_layer * sizeof(void*));
    m.attn_q    = malloc(m.n_layer * sizeof(void*));
    m.attn_k    = malloc(m.n_layer * sizeof(void*));
    m.attn_v    = malloc(m.n_layer * sizeof(void*));
    m.attn_o    = malloc(m.n_layer * sizeof(void*));
    m.ffn_norm  = malloc(m.n_layer * sizeof(void*));
    m.ffn_gate  = malloc(m.n_layer * sizeof(void*));
    m.ffn_up    = malloc(m.n_layer * sizeof(void*));
    m.ffn_down  = malloc(m.n_layer * sizeof(void*));

    /* Load weights */
    load_q4(&ctx, "token_embd.weight", &m.tok_embeddings);
    load_q4(&ctx, "output_norm.weight", &m.output_norm);
    load_q4(&ctx, "output.weight", &m.output_weight);

    char name[128];
    for (int l = 0; l < m.n_layer; l++) {
        fmt_name(name, sizeof(name), "blk.", l, ".attn_norm.weight");    load_q4(&ctx, name, &m.attn_norm[l]);
        fmt_name(name, sizeof(name), "blk.", l, ".attn_q.weight");       load_q4(&ctx, name, &m.attn_q[l]);
        fmt_name(name, sizeof(name), "blk.", l, ".attn_k.weight");       load_q4(&ctx, name, &m.attn_k[l]);
        fmt_name(name, sizeof(name), "blk.", l, ".attn_v.weight");       load_q4(&ctx, name, &m.attn_v[l]);
        fmt_name(name, sizeof(name), "blk.", l, ".attn_output.weight");  load_q4(&ctx, name, &m.attn_o[l]);
        fmt_name(name, sizeof(name), "blk.", l, ".ffn_norm.weight");     load_q4(&ctx, name, &m.ffn_norm[l]);
        fmt_name(name, sizeof(name), "blk.", l, ".ffn_gate.weight");     load_q4(&ctx, name, &m.ffn_gate[l]);
        fmt_name(name, sizeof(name), "blk.", l, ".ffn_up.weight");       load_q4(&ctx, name, &m.ffn_up[l]);
        fmt_name(name, sizeof(name), "blk.", l, ".ffn_down.weight");     load_q4(&ctx, name, &m.ffn_down[l]);
    }

    printf("[AI] weights loaded, init model...\n");
    if (model_init(&m) < 0) {
        printf("[AI] KV cache allocation failed\n");
        return -1;
    }

    int tokens[1024], n = 0;
    for (const char *p = prompt; *p && n < 1023; p++)
        tokens[n++] = (int)(uint8_t)*p;
    printf("[AI] generating...\n");
    model_generate(&m, tokens, n, 128, 0.8f);

    model_free(&m);
    gguf_close(&ctx);
    return 0;
}
