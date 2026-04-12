#include "internal.h"

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define RR(x, n)    (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z)(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)      (RR(x,2) ^ RR(x,13) ^ RR(x,22))
#define EP1(x)      (RR(x,6) ^ RR(x,11) ^ RR(x,25))
#define SIG0(x)     (RR(x,7) ^ RR(x,18) ^ ((x) >> 3))
#define SIG1(x)     (RR(x,17) ^ RR(x,19) ^ ((x) >> 10))

typedef struct {
    uint32_t state[8];
    uint8_t  buf[64];
    uint64_t total;
    size_t   buflen;
} sha256_ctx;

static void sha256_transform(sha256_ctx *ctx) {
    uint32_t w[64], v[8], t1, t2;
    int i;

    for (i = 0; i < 16; i++)
        w[i] = ((uint32_t)ctx->buf[i*4] << 24) | ((uint32_t)ctx->buf[i*4+1] << 16) |
               ((uint32_t)ctx->buf[i*4+2] << 8)  |  (uint32_t)ctx->buf[i*4+3];
    for (i = 16; i < 64; i++)
        w[i] = SIG1(w[i-2]) + w[i-7] + SIG0(w[i-15]) + w[i-16];

    for (i = 0; i < 8; i++) v[i] = ctx->state[i];

    for (i = 0; i < 64; i++) {
        t1 = v[7] + EP1(v[4]) + CH(v[4], v[5], v[6]) + K[i] + w[i];
        t2 = EP0(v[0]) + MAJ(v[0], v[1], v[2]);
        v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
        v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
    }

    for (i = 0; i < 8; i++) ctx->state[i] += v[i];
}

static void sha256_init(sha256_ctx *ctx) {
    ctx->state[0] = 0x6a09e667; ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372; ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f; ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab; ctx->state[7] = 0x5be0cd19;
    ctx->total = 0;
    ctx->buflen = 0;
}

static void sha256_update(sha256_ctx *ctx, const void *data, size_t len) {
    const uint8_t *p = data;
    ctx->total += len;
    while (len) {
        size_t room = 64 - ctx->buflen;
        size_t take = len < room ? len : room;
        memcpy(ctx->buf + ctx->buflen, p, take);
        ctx->buflen += take;
        p += take;
        len -= take;
        if (ctx->buflen == 64) {
            sha256_transform(ctx);
            ctx->buflen = 0;
        }
    }
}

static void sha256_final(sha256_ctx *ctx, uint8_t out[32]) {
    uint64_t bits = ctx->total * 8;
    uint8_t pad = 0x80;
    sha256_update(ctx, &pad, 1);
    pad = 0;
    while (ctx->buflen != 56)
        sha256_update(ctx, &pad, 1);
    for (int i = 7; i >= 0; i--) {
        pad = (bits >> (i * 8)) & 0xff;
        sha256_update(ctx, &pad, 1);
    }
    for (int i = 0; i < 8; i++) {
        out[i*4]   = (ctx->state[i] >> 24) & 0xff;
        out[i*4+1] = (ctx->state[i] >> 16) & 0xff;
        out[i*4+2] = (ctx->state[i] >> 8)  & 0xff;
        out[i*4+3] =  ctx->state[i]        & 0xff;
    }
}

void loom_sha256(const void *data, size_t len, loom_hash_t out) {
    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, out);
}

int loom_sha256_file(const char *path, loom_hash_t out) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    sha256_ctx ctx;
    sha256_init(&ctx);
    uint8_t buf[8192];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        sha256_update(&ctx, buf, n);
    fclose(f);
    sha256_final(&ctx, out);
    return 0;
}

void loom_hash_hex(const loom_hash_t h, char *out) {
    for (int i = 0; i < 32; i++)
        sprintf(out + i * 2, "%02x", h[i]);
    out[64] = '\0';
}
