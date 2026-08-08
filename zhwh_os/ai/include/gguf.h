/* gguf.h — GGUF format parser for IBM Granite */
#ifndef GGUF_H
#define GGUF_H
#include <stdint.h>

#define GGUF_MAGIC 0x46554747  /* "GGUF" little-endian */

/* Value types for metadata KV pairs */
enum gguf_type {
    GGUF_TYPE_UINT8   = 0,
    GGUF_TYPE_INT8    = 1,
    GGUF_TYPE_UINT16  = 2,
    GGUF_TYPE_INT16   = 3,
    GGUF_TYPE_UINT32  = 4,
    GGUF_TYPE_INT32   = 5,
    GGUF_TYPE_FLOAT32 = 6,
    GGUF_TYPE_BOOL    = 7,
    GGUF_TYPE_STRING  = 8,
    GGUF_TYPE_ARRAY   = 9,
    GGUF_TYPE_UINT64  = 10,
    GGUF_TYPE_INT64   = 11,
    GGUF_TYPE_FLOAT64 = 12,
};

/* Tensor info from GGUF file */
typedef struct {
    char     name[128];
    uint32_t n_dims;
    uint64_t shape[4];
    uint32_t ggml_type;   /* 0=F32, 1=F16, 2=Q4_0, ..., 10=Q4_K_M */
    uint64_t offset;       /* byte offset to tensor data in file */
} gguf_tensor_t;

/* GGUF context — holds all parsed model info */
typedef struct {
    int fd;                 /* file descriptor (or -1 if using mmap) */

    uint32_t version;
    uint64_t n_tensors;
    uint64_t n_kv;

    /* Model metadata */
    char     name[64];      /* "Granite 3.0 Nano" */
    char     arch[32];      /* "granite" */
    uint32_t n_vocab;
    uint32_t n_embd;
    uint32_t n_head;
    uint32_t n_head_kv;
    uint32_t n_layer;
    uint32_t n_ff;
    uint32_t n_ctx;          /* max context length */
    float    f_norm_eps;
    uint32_t bos_token_id;
    uint32_t eos_token_id;

    /* Tensor info array */
    gguf_tensor_t *tensors;
    uint8_t *data;           /* mmap'd data or NULL if using file reads */
    uint64_t data_offset;    /* aligned absolute file offset of tensor data */
} gguf_ctx_t;

/* API */
int  gguf_open(const char *path, gguf_ctx_t *ctx);
void gguf_close(gguf_ctx_t *ctx);
int  gguf_get_tensor(gguf_ctx_t *ctx, const char *name, void **data, uint64_t *nbytes);

#endif
