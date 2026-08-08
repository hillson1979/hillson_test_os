/* matmul.h — Matrix operations for inference */
#ifndef MATMUL_H
#define MATMUL_H
#include <stdint.h>

/* Q4_K_M block dequantization */
int dequantize_q4_K_M(const void *qweight, float *output, int n_elements);

/* Core ops */
void matmul_f32(const float *A, const float *B, float *C, int M, int N, int K);
void rms_norm(const float *x, float *out, int n, float eps);
void rope(float *q, float *k, int head_dim, int pos, float theta);
void softmax(float *x, int n);
void silu(float *x, int n);
void add_vec(float *a, const float *b, int n);
void mul_scalar(float *x, float s, int n);

/* Quantized matmul: Q4_K_M weight × float input → float output */
void matmul_q4_f32(const void *qw, const float *x, float *y, int n, int d);

#endif
