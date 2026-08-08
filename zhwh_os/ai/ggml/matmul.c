/* matmul.c — Matrix operations for Q4_K_M quantized inference */
#include "matmul.h"
#include <stdint.h>

/* Minimal math functions for freestanding environment */
float expf(float x) {
    float r = 1.0f, t = 1.0f;
    for (int i = 1; i < 20; i++) { t *= x / i; r += t; }
    return r;
}
float sqrtf(float x) {
    float r = x;
    for (int i = 0; i < 10; i++) r = (r + x/r) * 0.5f;
    return r;
}
static float cosf(float x) {
    float r = 1.0f, t = 1.0f;
    for (int i = 1; i < 12; i++) { t *= -x*x/((2*i-1)*2*i); r += t; }
    return r;
}
static float sinf(float x) {
    float r = x, t = x;
    for (int i = 1; i < 12; i++) { t *= -x*x/((2*i)*(2*i+1)); r += t; }
    return r;
}
static float logf(float x) {
    float r = 0.0f, t = (x-1)/(x+1), t2 = t*t, p = t;
    for (int i = 1; i < 20; i += 2) { r += p/i; p *= t2; }
    return 2.0f * r;
}
static float powf(float x, float y) { return expf(y * logf(x)); }
#include <stdint.h>
/* math functions self-implemented above */

/* ================================================================
 * Q4_K_M Dequantization
 * Super-block: 256 weights per block
 * Layout: d(16bit) + dmin(16bit) + scales(6bit×16) + qs(4bit×256)
 * ================================================================ */
int dequantize_q4_K_M(const void *qweight, float *output, int n_weights) {
    const uint8_t *qw = (const uint8_t *)qweight;
    int n_blocks = n_weights / 256;
    int out_idx = 0;

    for (int b = 0; b < n_blocks; b++) {
        /* Read super-block header: d (float16) + dmin (float16) */
        uint16_t d_raw = qw[0] | (qw[1] << 8);
        uint16_t m_raw = qw[2] | (qw[3] << 8);

        /* Half-float to float conversion */
        float d, dmin;
        {
            uint32_t sign = (d_raw >> 15) & 1;
            uint32_t exp  = (d_raw >> 10) & 0x1F;
            uint32_t mant = d_raw & 0x3FF;
            if (exp == 0)      d = 0.0f;
            else if (exp < 31) d = (1.0f - 2.0f*sign) * ((float)(1 << (exp-15)) * (1.0f + mant/1024.0f));
            else               d = (sign ? -1.0f/0.0f : 1.0f/0.0f);
        }
        {
            uint32_t sign = (m_raw >> 15) & 1;
            uint32_t exp  = (m_raw >> 10) & 0x1F;
            uint32_t mant = m_raw & 0x3FF;
            if (exp == 0)      dmin = 0.0f;
            else if (exp < 31) dmin = (1.0f - 2.0f*sign) * ((float)(1 << (exp-15)) * (1.0f + mant/1024.0f));
            else               dmin = (sign ? -1.0f/0.0f : 1.0f/0.0f);
        }
        qw += 4;

        /* Read 12 scale bytes → 16 6-bit scales */
        int sc[16];
        for (int i = 0; i < 12; i++) {
            uint8_t bv = qw[i];
            int j = (i / 3) * 4;
            if (i % 3 == 0) {
                sc[j]   = bv & 0x3F;
                sc[j+1] = (bv >> 6) | ((qw[i+1] & 0x0F) << 2);
            } else if (i % 3 == 1) {
                sc[j+2] = (qw[i] >> 4) | ((qw[i+1] & 0x03) << 4);
                sc[j+3] = (qw[i+1] >> 2) & 0x3F;
            }
        }
        qw += 12;

        /* Read 128 quantized bytes → 256 4-bit values */
        for (int j = 0; j < 128; j++) {
            uint8_t bv = qw[j];
            int ql = bv & 0xF;
            int qh = bv >> 4;
            int si_l = sc[j / 16] - 32;
            int si_h = sc[(j / 16) + 8] - 32;
            output[out_idx++] = d * (float)(ql - 8 + si_l);
            output[out_idx++] = d * (float)(qh - 8 + si_h);
        }
        qw += 128;
    }
    return out_idx;
}

/* ================================================================
 * Float GEMM: C[M×N] = A[M×K] × B[K×N]
 * ================================================================ */
void matmul_f32(const float *A, const float *B, float *C, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

/* ================================================================
 * RMS Normalization
 * ================================================================ */
void rms_norm(const float *x, float *out, int n, float eps) {
    float ss = 0.0f;
    for (int i = 0; i < n; i++) ss += x[i] * x[i];
    ss = ss / (float)n + eps;
    ss = 1.0f / sqrtf(ss);
    for (int i = 0; i < n; i++) out[i] = x[i] * ss;
}

/* ================================================================
 * RoPE (Rotary Position Embedding)
 * ================================================================ */
void rope(float *q, float *k, int head_dim, int pos, float theta) {
    for (int i = 0; i < head_dim; i += 2) {
        float freq = 1.0f / powf(theta, (float)i / (float)head_dim);
        float val  = (float)pos * freq;
        float cosv = cosf(val);
        float sinv = sinf(val);

        float q0 = q[i],     q1 = q[i + 1];
        float k0 = k[i],     k1 = k[i + 1];

        q[i]     = q0 * cosv - q1 * sinv;
        q[i + 1] = q1 * cosv + q0 * sinv;
        k[i]     = k0 * cosv - k1 * sinv;
        k[i + 1] = k1 * cosv + k0 * sinv;
    }
}

/* ================================================================
 * Softmax
 * ================================================================ */
void softmax(float *x, int n) {
    float max_val = x[0];
    for (int i = 1; i < n; i++) if (x[i] > max_val) max_val = x[i];

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }
    float inv = 1.0f / sum;
    for (int i = 0; i < n; i++) x[i] *= inv;
}

/* ================================================================
 * SiLU activation (in-place)
 * ================================================================ */
void silu(float *x, int n) {
    for (int i = 0; i < n; i++) {
        float s = 1.0f / (1.0f + expf(-x[i]));
        x[i] = x[i] * s;
    }
}

/* ================================================================
 * Vector ops
 * ================================================================ */
void add_vec(float *a, const float *b, int n) {
    for (int i = 0; i < n; i++) a[i] += b[i];
}

void mul_scalar(float *x, float s, int n) {
    for (int i = 0; i < n; i++) x[i] *= s;
}

/* ================================================================
 * Quantized GEMM: Q4_K_M weight × float input
 *   weight[K][N] in Q4_K_M → dequantize row → dot with input
 * ================================================================ */
void matmul_q4_f32(const void *qw, const float *x, float *y, int n, int d) {
    /* n = output dim, d = input dim */
    /* qw has n rows, each with d/2 bytes (Q4 = 0.5 bytes/weight) */
    const uint8_t *qbytes = (const uint8_t *)qw;
    int block_sz = 4 + 12 + 128; /* sizeof(block_q4_K) */
    int n_blocks = d / 256;

    for (int i = 0; i < n; i++) {
        float sum = 0.0f;
        const uint8_t *row = qbytes + i * n_blocks * block_sz;

        for (int b = 0; b < n_blocks; b++) {
            float tmp[256];
            dequantize_q4_K_M(row + b * block_sz, tmp, 256);
            for (int j = 0; j < 256; j++) {
                sum += tmp[j] * x[b * 256 + j];
            }
        }
        y[i] = sum;
    }
}
