#ifndef KERNEL_WPA2_CRYPTO_H
#define KERNEL_WPA2_CRYPTO_H

#include <stdint.h>
#include <stddef.h>

struct wpa2_sha1_ctx {
    uint32_t state[5];
    uint64_t bit_count;
    uint8_t block[64];
    size_t block_length;
};

void wpa2_sha1_init(struct wpa2_sha1_ctx *ctx);
void wpa2_sha1_update(struct wpa2_sha1_ctx *ctx, const uint8_t *data,
                      size_t length);
void wpa2_sha1_final(struct wpa2_sha1_ctx *ctx, uint8_t digest[20]);
void wpa2_hmac_sha1(const uint8_t *key, size_t key_length,
                    const uint8_t *data, size_t data_length,
                    uint8_t digest[20]);
int wpa2_pbkdf2_sha1(const uint8_t *password, size_t password_length,
                     const uint8_t *salt, size_t salt_length,
                     uint32_t iterations, uint8_t *output,
                     size_t output_length);
int wpa2_prf512(const uint8_t *key, size_t key_length,
                const char *label, const uint8_t *data, size_t data_length,
                uint8_t output[64]);
void wpa2_aes128_encrypt(const uint8_t key[16], const uint8_t input[16],
                         uint8_t output[16]);
void wpa2_aes128_decrypt(const uint8_t key[16], const uint8_t input[16],
                         uint8_t output[16]);
int wpa2_aes128_unwrap(const uint8_t key[16], const uint8_t *input,
                       size_t input_length, uint8_t *output,
                       size_t output_capacity, size_t *output_length);

#endif
