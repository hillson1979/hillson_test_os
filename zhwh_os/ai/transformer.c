/* transformer.c — Granite model forward pass */
#include "transformer.h"
#include "matmul.h"
#include <stdint.h>

extern void *malloc(int sz);
extern void free(void *p);

static uint16_t f32_to_f16(float value) {
    union { float f; uint32_t u; } v = { value };
    uint32_t sign = (v.u >> 16) & 0x8000;
    int exp = (int)((v.u >> 23) & 0xff) - 112;
    uint32_t mant = v.u & 0x7fffff;
    if (exp <= 0) return (uint16_t)sign;
    if (exp >= 31) return (uint16_t)(sign | 0x7c00);
    return (uint16_t)(sign | ((uint32_t)exp << 10) | (mant >> 13));
}

static float f16_to_f32(uint16_t value) {
    uint32_t sign = ((uint32_t)value & 0x8000) << 16;
    uint32_t exp = (value >> 10) & 0x1f;
    uint32_t mant = value & 0x3ff;
    union { uint32_t u; float f; } v;
    if (exp == 0) v.u = sign;
    else if (exp == 31) v.u = sign | 0x7f800000 | (mant << 13);
    else v.u = sign | ((exp + 112) << 23) | (mant << 13);
    return v.f;
}
/* printf, expf, sqrtf etc — linked from matmul.c and OS libc */

int model_init(model_t *m) {
    int head_dim = m->n_embd / m->n_head;

    /* Allocate KV cache: [n_layer][n_ctx, n_head_kv * head_dim] */
    m->k_cache = malloc(m->n_layer * sizeof(uint16_t*));
    m->v_cache = malloc(m->n_layer * sizeof(uint16_t*));
    if (!m->k_cache || !m->v_cache) return -1;
    for (int i = 0; i < m->n_layer; i++) {
        m->k_cache[i] = malloc(m->n_ctx * m->n_head_kv * head_dim * sizeof(uint16_t));
        m->v_cache[i] = malloc(m->n_ctx * m->n_head_kv * head_dim * sizeof(uint16_t));
        if (!m->k_cache[i] || !m->v_cache[i]) return -1;
    }
    m->cache_pos = 0;
    return 0;
}

void model_free(model_t *m) {
    for (int i = 0; i < m->n_layer; i++) { free(m->k_cache[i]); free(m->v_cache[i]); }
    free(m->k_cache); free(m->v_cache);
}

/* ================================================================
 * Single-head attention
 * ================================================================ */
static void attention_layer(model_t *m, int layer, const float *x, float *out,
                             int n_tokens, int head_dim, int n_head, int n_kv_head) {
    int n_groups = n_head / n_kv_head;  /* GQA: group query heads */
    int dim = n_head * head_dim;

    /* Allocate scratch space for Q, K, V, attn scores */
    float *q = malloc(n_tokens * dim * sizeof(float));
    float *k = malloc(n_tokens * n_kv_head * head_dim * sizeof(float));
    float *v = malloc(n_tokens * n_kv_head * head_dim * sizeof(float));
    float *scores = malloc(n_tokens * n_head * n_tokens * sizeof(float));

    /* Q = xWq, K = xWk, V = xWv */
    matmul_q4_f32(m->attn_q[layer], x, q, n_tokens * dim, m->n_embd);
    matmul_q4_f32(m->attn_k[layer], x, k, n_tokens * n_kv_head * head_dim, m->n_embd);
    matmul_q4_f32(m->attn_v[layer], x, v, n_tokens * n_kv_head * head_dim, m->n_embd);

    /* RoPE on Q, K */
    for (int t = 0; t < n_tokens; t++) {
        for (int h = 0; h < n_head; h++) {
            rope(q + t * dim + h * head_dim, k + t * n_kv_head * head_dim + (h/n_groups) * head_dim,
                 head_dim, m->cache_pos + t, m->rope_theta);
        }
    }

    /* Store K, V in cache */
    uint16_t *kc = m->k_cache[layer];
    uint16_t *vc = m->v_cache[layer];
    int cache_start = m->cache_pos;
    for (int t = 0; t < n_tokens; t++) {
        for (int i = 0; i < n_kv_head * head_dim; i++) {
            kc[(cache_start + t) * n_kv_head * head_dim + i] = f32_to_f16(k[t * n_kv_head * head_dim + i]);
            vc[(cache_start + t) * n_kv_head * head_dim + i] = f32_to_f16(v[t * n_kv_head * head_dim + i]);
        }
    }
    int total_kv = cache_start + n_tokens;

    /* Scaled dot-product attention */
    float scale = 1.0f / sqrtf((float)head_dim);
    for (int t = 0; t < n_tokens; t++) {
        for (int h = 0; h < n_head; h++) {
            int kh = h / n_groups;
            float *s = scores + t * n_head * total_kv + h * total_kv;
            for (int j = 0; j < total_kv; j++) {
                float dot = 0.0f;
                for (int d = 0; d < head_dim; d++) {
                    dot += q[t * dim + h * head_dim + d] *
                           f16_to_f32(kc[j * n_kv_head * head_dim + kh * head_dim + d]);
                }
                s[j] = dot * scale;
            }
            softmax(s, total_kv);

            /* Weighted sum of values */
            for (int d = 0; d < head_dim; d++) {
                float sum = 0.0f;
                for (int j = 0; j < total_kv; j++) {
                    sum += s[j] *
                           f16_to_f32(vc[j * n_kv_head * head_dim + kh * head_dim + d]);
                }
                out[t * dim + h * head_dim + d] = sum;
            }
        }
    }

    /* Output projection */
    float *attn_out = malloc(n_tokens * dim * sizeof(float));
    matmul_q4_f32(m->attn_o[layer], out, attn_out, n_tokens * dim, dim);

    /* Residual: out = attn_out + x (pre-norm architecture) */
    for (int i = 0; i < n_tokens * dim; i++) out[i] = attn_out[i];

    free(attn_out); free(scores); free(v); free(k); free(q);
}

/* ================================================================
 * Feed-Forward Network (SwiGLU)
 * ================================================================ */
static void ffn_layer(model_t *m, int layer, float *x, int n_tokens, int dim) {
    float *gate = malloc(n_tokens * m->n_ff * sizeof(float));
    float *up   = malloc(n_tokens * m->n_ff * sizeof(float));
    float *down = malloc(n_tokens * dim * sizeof(float));

    matmul_q4_f32(m->ffn_gate[layer], x, gate, n_tokens * m->n_ff, dim);
    matmul_q4_f32(m->ffn_up[layer],   x, up,   n_tokens * m->n_ff, dim);

    /* SwiGLU: gate * SiLU(gate) * up */
    silu(gate, n_tokens * m->n_ff);
    for (int i = 0; i < n_tokens * m->n_ff; i++) gate[i] = gate[i] * up[i];

    matmul_q4_f32(m->ffn_down[layer], gate, down, n_tokens * dim, m->n_ff);

    /* Residual */
    for (int i = 0; i < n_tokens * dim; i++) x[i] = x[i] + down[i];

    free(down); free(up); free(gate);
}

/* ================================================================
 * Full forward pass: input token → logits[vocab]
 * ================================================================ */
int model_forward(model_t *m, int token, float *logits) {
    int dim = m->n_embd;
    int head_dim = dim / m->n_head;

    /* Embedding lookup */
    float *x = malloc(dim * sizeof(float));
    {
        /* token_embeddings[token] → x */
        float tmp_buf[4096];
        int embd_rounded = ((dim + 255) / 256) * 256;
        dequantize_q4_K_M((char*)m->tok_embeddings + token * dim / 2, tmp_buf, embd_rounded);
        for (int i = 0; i < dim; i++) x[i] = tmp_buf[i];
    }

    float *norm_buf = malloc(dim * sizeof(float));

    /* Process layers */
    for (int l = 0; l < m->n_layer; l++) {
        /* Pre-norm */
        rms_norm(x, norm_buf, dim, m->norm_eps);

        /* Attention */
        float *attn_x = malloc(dim * sizeof(float));
        attention_layer(m, l, norm_buf, attn_x, 1, head_dim, m->n_head, m->n_head_kv);
        for (int i = 0; i < dim; i++) x[i] += attn_x[i]; /* residual */
        free(attn_x);

        /* FFN */
        rms_norm(x, norm_buf, dim, m->norm_eps);
        ffn_layer(m, l, norm_buf, 1, dim);
    }

    /* Final norm + output projection */
    rms_norm(x, norm_buf, dim, m->norm_eps);
    matmul_q4_f32(m->output_weight, norm_buf, logits, m->n_vocab, dim);

    free(norm_buf); free(x);
    m->cache_pos += 1;
    return 0;
}

/* ================================================================
 * Autoregressive generation
 * ================================================================ */
static int sample_token(float *logits, int n, float temp) {
    if (temp <= 0.0f) {
        /* Greedy */
        int best = 0;
        float best_val = logits[0];
        for (int i = 1; i < n; i++) if (logits[i] > best_val) { best_val = logits[i]; best = i; }
        return best;
    }
    /* Temperature sampling */
    float sum = 0.0f;
    for (int i = 0; i < n; i++) { logits[i] = expf(logits[i] / temp); sum += logits[i]; }
    /* Simple argmax after temperature (proper sampling needs random) */
    int best = 0;
    float best_val = logits[0];
    for (int i = 1; i < n; i++) if (logits[i] > best_val) { best_val = logits[i]; best = i; }
    return best;
}

int model_generate(model_t *m, int *tokens, int n_prompt, int max_new, float temp) {
    float *logits = malloc(m->n_vocab * sizeof(float));
    if (!logits) return -1;

    /* Process prompt */
    for (int i = 0; i < n_prompt; i++) {
        model_forward(m, tokens[i], logits);
    }

    /* Generate */
    int n_gen = 0;
    for (; n_gen < max_new; n_gen++) {
        int next = sample_token(logits, m->n_vocab, temp);
        if (next == 2) break; /* EOS */
        tokens[n_prompt + n_gen] = next;
        model_forward(m, next, logits);
    }

    free(logits);
    return n_gen;
}
