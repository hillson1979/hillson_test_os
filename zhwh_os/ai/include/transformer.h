/* transformer.h — Granite Transformer model runner */
#ifndef TRANSFORMER_H
#define TRANSFORMER_H
#include <stdint.h>

typedef struct {
    int n_vocab, n_embd, n_head, n_head_kv, n_layer, n_ff, n_ctx;
    float norm_eps, rope_theta;

    /* Model weights (pointers into GGUF memory) */
    void *tok_embeddings;   /* [n_vocab, n_embd] */
    void *output_norm;      /* [n_embd] */
    void *output_weight;    /* [n_vocab, n_embd] */

    /* Per-layer weights */
    void **attn_norm;      /* [n_layer][n_embd] */
    void **attn_q;         /* [n_layer][n_embd, n_head * head_dim] */
    void **attn_k;
    void **attn_v;
    void **attn_o;         /* [n_layer][n_head * head_dim, n_embd] */
    void **ffn_norm;       /* [n_layer][n_embd] */
    void **ffn_gate;       /* [n_layer][n_ff, n_embd] */
    void **ffn_up;         /* [n_layer][n_ff, n_embd] */
    void **ffn_down;       /* [n_layer][n_embd, n_ff] */

    /* KV cache */
    uint16_t **k_cache;    /* F16 [n_layer][n_ctx, n_head_kv * head_dim] */
    uint16_t **v_cache;    /* F16 [n_layer][n_ctx, n_head_kv * head_dim] */
    int     cache_pos;
} model_t;

int  model_init(model_t *m);
void model_free(model_t *m);
int  model_forward(model_t *m, int token, float *logits);
int  model_generate(model_t *m, int *tokens, int n_prompt, int max_new, float temp);

#endif
