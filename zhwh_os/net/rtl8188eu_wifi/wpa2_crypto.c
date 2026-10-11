#include <stdint.h>
#include <stddef.h>
#include <kernel/errno.h>
#include <kernel/wpa2_crypto.h>

static uint32_t wpa2_rol32(uint32_t value, unsigned int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

static uint32_t wpa2_get_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

static void wpa2_put_be32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static void wpa2_sha1_transform(struct wpa2_sha1_ctx *ctx,
                                const uint8_t block[64])
{
    uint32_t words[80];
    uint32_t a, b, c, d, e, f, k, temp;
    unsigned int i;

    for (i = 0; i < 16; i++)
        words[i] = wpa2_get_be32(block + i * 4);
    for (i = 16; i < 80; i++)
        words[i] = wpa2_rol32(words[i - 3] ^ words[i - 8] ^
                              words[i - 14] ^ words[i - 16], 1);

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    for (i = 0; i < 80; i++) {
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5a827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdc;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6;
        }
        temp = wpa2_rol32(a, 5) + f + e + k + words[i];
        e = d;
        d = c;
        c = wpa2_rol32(b, 30);
        b = a;
        a = temp;
    }
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

void wpa2_sha1_init(struct wpa2_sha1_ctx *ctx)
{
    if (!ctx)
        return;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xc3d2e1f0;
    ctx->bit_count = 0;
    ctx->block_length = 0;
}

void wpa2_sha1_update(struct wpa2_sha1_ctx *ctx, const uint8_t *data,
                      size_t length)
{
    size_t take;

    if (!ctx || (!data && length))
        return;
    ctx->bit_count += (uint64_t)length * 8;
    while (length) {
        take = sizeof(ctx->block) - ctx->block_length;
        if (take > length)
            take = length;
        for (size_t i = 0; i < take; i++)
            ctx->block[ctx->block_length + i] = data[i];
        ctx->block_length += take;
        data += take;
        length -= take;
        if (ctx->block_length == sizeof(ctx->block)) {
            wpa2_sha1_transform(ctx, ctx->block);
            ctx->block_length = 0;
        }
    }
}

void wpa2_sha1_final(struct wpa2_sha1_ctx *ctx, uint8_t digest[20])
{
    uint64_t bits;
    size_t i;

    if (!ctx || !digest)
        return;
    bits = ctx->bit_count;
    ctx->block[ctx->block_length++] = 0x80;
    while (ctx->block_length != 56) {
        if (ctx->block_length == 64) {
            wpa2_sha1_transform(ctx, ctx->block);
            ctx->block_length = 0;
        }
        ctx->block[ctx->block_length++] = 0;
    }
    for (i = 0; i < 8; i++)
        ctx->block[56 + i] = (uint8_t)(bits >> (56 - i * 8));
    wpa2_sha1_transform(ctx, ctx->block);
    for (i = 0; i < 5; i++)
        wpa2_put_be32(digest + i * 4, ctx->state[i]);
}

void wpa2_hmac_sha1(const uint8_t *key, size_t key_length,
                    const uint8_t *data, size_t data_length,
                    uint8_t digest[20])
{
    struct wpa2_sha1_ctx ctx;
    uint8_t key_block[64];
    uint8_t inner[20];
    size_t i;

    if (!digest || (!key && key_length) || (!data && data_length))
        return;
    for (i = 0; i < sizeof(key_block); i++)
        key_block[i] = 0;
    if (key_length > sizeof(key_block)) {
        wpa2_sha1_init(&ctx);
        wpa2_sha1_update(&ctx, key, key_length);
        wpa2_sha1_final(&ctx, key_block);
        key_length = 20;
    } else {
        for (i = 0; i < key_length; i++)
            key_block[i] = key[i];
    }
    for (i = 0; i < sizeof(key_block); i++)
        key_block[i] ^= 0x36;
    wpa2_sha1_init(&ctx);
    wpa2_sha1_update(&ctx, key_block, sizeof(key_block));
    wpa2_sha1_update(&ctx, data, data_length);
    wpa2_sha1_final(&ctx, inner);
    for (i = 0; i < sizeof(key_block); i++)
        key_block[i] ^= 0x36 ^ 0x5c;
    wpa2_sha1_init(&ctx);
    wpa2_sha1_update(&ctx, key_block, sizeof(key_block));
    wpa2_sha1_update(&ctx, inner, sizeof(inner));
    wpa2_sha1_final(&ctx, digest);
}

int wpa2_pbkdf2_sha1(const uint8_t *password, size_t password_length,
                     const uint8_t *salt, size_t salt_length,
                     uint32_t iterations, uint8_t *output,
                     size_t output_length)
{
    uint8_t u[20];
    uint8_t t[20];
    uint8_t *message;
    size_t message_length;
    size_t copied = 0;
    uint32_t block = 1;
    unsigned int i;

    if ((!password && password_length) || (!salt && salt_length) ||
        !iterations || !output)
        return -EINVAL;
    message_length = salt_length + 4;
    message = (uint8_t *)__builtin_alloca(message_length);
    for (i = 0; i < salt_length; i++)
        message[i] = salt[i];
    while (copied < output_length) {
        message[salt_length] = (uint8_t)(block >> 24);
        message[salt_length + 1] = (uint8_t)(block >> 16);
        message[salt_length + 2] = (uint8_t)(block >> 8);
        message[salt_length + 3] = (uint8_t)block;
        wpa2_hmac_sha1(password, password_length, message, message_length, u);
        for (i = 0; i < sizeof(t); i++)
            t[i] = u[i];
        for (i = 1; i < iterations; i++) {
            wpa2_hmac_sha1(password, password_length, u, sizeof(u), u);
            for (unsigned int j = 0; j < sizeof(t); j++)
                t[j] ^= u[j];
        }
        for (i = 0; i < sizeof(t) && copied < output_length; i++)
            output[copied++] = t[i];
        block++;
    }
    return 0;
}

int wpa2_prf512(const uint8_t *key, size_t key_length,
                const char *label, const uint8_t *data, size_t data_length,
                uint8_t output[64])
{
    uint8_t input[256];
    uint8_t digest[20];
    size_t label_length;
    size_t input_length;
    size_t copied = 0;
    uint8_t counter = 0;

    if (!key || !label || (!data && data_length) || !output)
        return -EINVAL;
    label_length = 0;
    while (label[label_length])
        label_length++;
    input_length = label_length + 1 + data_length + 1;
    if (input_length > sizeof(input))
        return -EINVAL;
    for (size_t i = 0; i < label_length; i++)
        input[i] = (uint8_t)label[i];
    input[label_length] = 0;
    for (size_t i = 0; i < data_length; i++)
        input[label_length + 1 + i] = data[i];
    while (copied < 64) {
        counter++;
        input[label_length + 1 + data_length] = counter;
        wpa2_hmac_sha1(key, key_length, input, input_length, digest);
        for (size_t i = 0; i < sizeof(digest) && copied < 64; i++)
            output[copied++] = digest[i];
    }
    return 0;
}

static const uint8_t wpa2_aes_sbox[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16
};

static const uint8_t wpa2_aes_inv_sbox[256] = {
    0x52,0x09,0x6a,0xd5,0x30,0x36,0xa5,0x38,0xbf,0x40,0xa3,0x9e,0x81,0xf3,0xd7,0xfb,
    0x7c,0xe3,0x39,0x82,0x9b,0x2f,0xff,0x87,0x34,0x8e,0x43,0x44,0xc4,0xde,0xe9,0xcb,
    0x54,0x7b,0x94,0x32,0xa6,0xc2,0x23,0x3d,0xee,0x4c,0x95,0x0b,0x42,0xfa,0xc3,0x4e,
    0x08,0x2e,0xa1,0x66,0x28,0xd9,0x24,0xb2,0x76,0x5b,0xa2,0x49,0x6d,0x8b,0xd1,0x25,
    0x72,0xf8,0xf6,0x64,0x86,0x68,0x98,0x16,0xd4,0xa4,0x5c,0xcc,0x5d,0x65,0xb6,0x92,
    0x6c,0x70,0x48,0x50,0xfd,0xed,0xb9,0xda,0x5e,0x15,0x46,0x57,0xa7,0x8d,0x9d,0x84,
    0x90,0xd8,0xab,0x00,0x8c,0xbc,0xd3,0x0a,0xf7,0xe4,0x58,0x05,0xb8,0xb3,0x45,0x06,
    0xd0,0x2c,0x1e,0x8f,0xca,0x3f,0x0f,0x02,0xc1,0xaf,0xbd,0x03,0x01,0x13,0x8a,0x6b,
    0x3a,0x91,0x11,0x41,0x4f,0x67,0xdc,0xea,0x97,0xf2,0xcf,0xce,0xf0,0xb4,0xe6,0x73,
    0x96,0xac,0x74,0x22,0xe7,0xad,0x35,0x85,0xe2,0xf9,0x37,0xe8,0x1c,0x75,0xdf,0x6e,
    0x47,0xf1,0x1a,0x71,0x1d,0x29,0xc5,0x89,0x6f,0xb7,0x62,0x0e,0xaa,0x18,0xbe,0x1b,
    0xfc,0x56,0x3e,0x4b,0xc6,0xd2,0x79,0x20,0x9a,0xdb,0xc0,0xfe,0x78,0xcd,0x5a,0xf4,
    0x1f,0xdd,0xa8,0x33,0x88,0x07,0xc7,0x31,0xb1,0x12,0x10,0x59,0x27,0x80,0xec,0x5f,
    0x60,0x51,0x7f,0xa9,0x19,0xb5,0x4a,0x0d,0x2d,0xe5,0x7a,0x9f,0x93,0xc9,0x9c,0xef,
    0xa0,0xe0,0x3b,0x4d,0xae,0x2a,0xf5,0xb0,0xc8,0xeb,0xbb,0x3c,0x83,0x53,0x99,0x61,
    0x17,0x2b,0x04,0x7e,0xba,0x77,0xd6,0x26,0xe1,0x69,0x14,0x63,0x55,0x21,0x0c,0x7d
};

static uint8_t wpa2_aes_xtime(uint8_t value)
{
    return (uint8_t)((value << 1) ^ ((value >> 7) * 0x1b));
}

static uint8_t wpa2_aes_mul(uint8_t a, uint8_t b)
{
    uint8_t result = 0;
    while (b) {
        if (b & 1)
            result ^= a;
        a = wpa2_aes_xtime(a);
        b >>= 1;
    }
    return result;
}

static void wpa2_aes_key_expand(const uint8_t key[16], uint8_t round_key[176])
{
    static const uint8_t rcon[10] =
        { 1, 2, 4, 8, 16, 32, 64, 128, 27, 54 };
    unsigned int bytes = 16;
    unsigned int round = 0;
    uint8_t temp[4];

    for (unsigned int i = 0; i < 16; i++)
        round_key[i] = key[i];
    while (bytes < 176) {
        for (unsigned int i = 0; i < 4; i++)
            temp[i] = round_key[bytes - 4 + i];
        if ((bytes & 15u) == 0) {
            uint8_t first = temp[0];
            temp[0] = wpa2_aes_sbox[temp[1]] ^ rcon[round++];
            temp[1] = wpa2_aes_sbox[temp[2]];
            temp[2] = wpa2_aes_sbox[temp[3]];
            temp[3] = wpa2_aes_sbox[first];
        }
        for (unsigned int i = 0; i < 4; i++) {
            round_key[bytes] = round_key[bytes - 16] ^ temp[i];
            bytes++;
        }
    }
}

static void wpa2_aes_add_round_key(uint8_t state[16],
                                   const uint8_t *round_key)
{
    for (unsigned int i = 0; i < 16; i++)
        state[i] ^= round_key[i];
}

static void wpa2_aes_sub_bytes(uint8_t state[16])
{
    for (unsigned int i = 0; i < 16; i++)
        state[i] = wpa2_aes_sbox[state[i]];
}

static void wpa2_aes_inv_sub_bytes(uint8_t state[16])
{
    for (unsigned int i = 0; i < 16; i++)
        state[i] = wpa2_aes_inv_sbox[state[i]];
}

static void wpa2_aes_shift_rows(uint8_t state[16])
{
    uint8_t copy[16];
    for (unsigned int i = 0; i < 16; i++)
        copy[i] = state[i];
    for (unsigned int row = 0; row < 4; row++)
        for (unsigned int col = 0; col < 4; col++)
            state[4 * col + row] = copy[4 * ((col + row) & 3u) + row];
}

static void wpa2_aes_inv_shift_rows(uint8_t state[16])
{
    uint8_t copy[16];
    for (unsigned int i = 0; i < 16; i++)
        copy[i] = state[i];
    for (unsigned int row = 0; row < 4; row++)
        for (unsigned int col = 0; col < 4; col++)
            state[4 * col + row] = copy[4 * ((col - row) & 3u) + row];
}

static void wpa2_aes_mix_columns(uint8_t state[16])
{
    for (unsigned int col = 0; col < 4; col++) {
        uint8_t *s = state + col * 4;
        uint8_t a = s[0], b = s[1], c = s[2], d = s[3];
        s[0] = wpa2_aes_mul(a, 2) ^ wpa2_aes_mul(b, 3) ^ c ^ d;
        s[1] = a ^ wpa2_aes_mul(b, 2) ^ wpa2_aes_mul(c, 3) ^ d;
        s[2] = a ^ b ^ wpa2_aes_mul(c, 2) ^ wpa2_aes_mul(d, 3);
        s[3] = wpa2_aes_mul(a, 3) ^ b ^ c ^ wpa2_aes_mul(d, 2);
    }
}

static void wpa2_aes_inv_mix_columns(uint8_t state[16])
{
    for (unsigned int col = 0; col < 4; col++) {
        uint8_t *s = state + col * 4;
        uint8_t a = s[0], b = s[1], c = s[2], d = s[3];
        s[0] = wpa2_aes_mul(a, 14) ^ wpa2_aes_mul(b, 11) ^
               wpa2_aes_mul(c, 13) ^ wpa2_aes_mul(d, 9);
        s[1] = wpa2_aes_mul(a, 9) ^ wpa2_aes_mul(b, 14) ^
               wpa2_aes_mul(c, 11) ^ wpa2_aes_mul(d, 13);
        s[2] = wpa2_aes_mul(a, 13) ^ wpa2_aes_mul(b, 9) ^
               wpa2_aes_mul(c, 14) ^ wpa2_aes_mul(d, 11);
        s[3] = wpa2_aes_mul(a, 11) ^ wpa2_aes_mul(b, 13) ^
               wpa2_aes_mul(c, 9) ^ wpa2_aes_mul(d, 14);
    }
}

void wpa2_aes128_encrypt(const uint8_t key[16], const uint8_t input[16],
                         uint8_t output[16])
{
    uint8_t round_key[176];
    uint8_t state[16];

    wpa2_aes_key_expand(key, round_key);
    for (unsigned int i = 0; i < 16; i++)
        state[i] = input[i];
    wpa2_aes_add_round_key(state, round_key);
    for (unsigned int round = 1; round < 10; round++) {
        wpa2_aes_sub_bytes(state);
        wpa2_aes_shift_rows(state);
        wpa2_aes_mix_columns(state);
        wpa2_aes_add_round_key(state, round_key + round * 16);
    }
    wpa2_aes_sub_bytes(state);
    wpa2_aes_shift_rows(state);
    wpa2_aes_add_round_key(state, round_key + 160);
    for (unsigned int i = 0; i < 16; i++)
        output[i] = state[i];
}

void wpa2_aes128_decrypt(const uint8_t key[16], const uint8_t input[16],
                         uint8_t output[16])
{
    uint8_t round_key[176];
    uint8_t state[16];

    wpa2_aes_key_expand(key, round_key);
    for (unsigned int i = 0; i < 16; i++)
        state[i] = input[i];
    wpa2_aes_add_round_key(state, round_key + 160);
    for (unsigned int round = 9; round > 0; round--) {
        wpa2_aes_inv_shift_rows(state);
        wpa2_aes_inv_sub_bytes(state);
        wpa2_aes_add_round_key(state, round_key + round * 16);
        wpa2_aes_inv_mix_columns(state);
    }
    wpa2_aes_inv_shift_rows(state);
    wpa2_aes_inv_sub_bytes(state);
    wpa2_aes_add_round_key(state, round_key);
    for (unsigned int i = 0; i < 16; i++)
        output[i] = state[i];
}

int wpa2_aes128_unwrap(const uint8_t key[16], const uint8_t *input,
                       size_t input_length, uint8_t *output,
                       size_t output_capacity, size_t *output_length)
{
    uint8_t a[8];
    uint8_t block[16];
    uint8_t result[16];
    size_t n;

    if (!key || !input || !output || input_length < 16 ||
        (input_length & 7u) || output_capacity < input_length - 8)
        return -EINVAL;
    n = input_length / 8 - 1;
    for (unsigned int i = 0; i < 8; i++)
        a[i] = input[i];
    for (size_t i = 0; i < n * 8; i++)
        output[i] = input[8 + i];
    for (int j = 5; j >= 0; j--) {
        for (size_t i = n; i > 0; i--) {
            uint32_t t = (uint32_t)(n * (unsigned int)j + i);
            for (unsigned int k = 0; k < 8; k++)
                block[k] = a[k] ^ (uint8_t)(t >> (56 - k * 8));
            for (unsigned int k = 0; k < 8; k++)
                block[8 + k] = output[(i - 1) * 8 + k];
            wpa2_aes128_decrypt(key, block, result);
            for (unsigned int k = 0; k < 8; k++)
                a[k] = result[k];
            for (unsigned int k = 0; k < 8; k++)
                output[(i - 1) * 8 + k] = result[8 + k];
        }
    }
    for (unsigned int i = 0; i < 8; i++)
        if (a[i] != 0xa6)
            return -EIO;
    if (output_length)
        *output_length = n * 8;
    return 0;
}
