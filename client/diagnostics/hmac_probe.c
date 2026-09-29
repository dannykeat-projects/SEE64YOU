#include <conio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "crypto/sha256.h"

static uint8_t digest[32];
static uint8_t key[32];
static uint8_t block[64];
static S64HmacSha256 hmac;

static char hex_digit(uint8_t value)
{
    value &= 0x0fu;
    return (char)(value < 10u ? '0' + value : 'A' + (value - 10u));
}

static void print_u32(uint32_t value)
{
    char digits[11];
    uint8_t used = 0u;

    if (value == 0u) {
        cputc('0');
        return;
    }
    while (value != 0u && used < sizeof(digits)) {
        digits[used++] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    while (used != 0u) cputc(digits[--used]);
}

int main(void)
{
    static const uint8_t message[] = {'a','b','c'};
    clock_t started;
    clock_t elapsed;
    uint16_t blocks;
    uint8_t i;

    bgcolor(COLOR_BLUE);
    bordercolor(COLOR_BLUE);
    textcolor(COLOR_LIGHTBLUE);
    clrscr();

    for (i = 0u; i < sizeof(key); ++i) key[i] = i;
    s64_hmac_sha256_init(&hmac, key);
    s64_hmac_sha256_update(&hmac, message, sizeof(message));
    s64_hmac_sha256_final(&hmac, digest);

    cputs("SEE64YOU HMAC-SHA256 PROBE\r\n\r\n");
    cputs("HMAC('ABC') PREFIX:\r\n");
    for (i = 0u; i < 16u; ++i) {
        cputc(hex_digit((uint8_t)(digest[i] >> 4)));
        cputc(hex_digit(digest[i]));
    }
    cputs("\r\nEXPECTED:\r\n");
    cputs("F0133729C4163DEDE81E21CD47839256\r\n\r\n");

    memset(block, 'A', sizeof(block));
    s64_hmac_sha256_init(&hmac, key);
    cputs("AUTHENTICATING 8K TEST DATA...\r\n");
    started = clock();
    for (blocks = 0u; blocks < 128u; ++blocks) {
        s64_hmac_sha256_update(&hmac, block, sizeof(block));
    }
    s64_hmac_sha256_final(&hmac, digest);
    elapsed = clock() - started;

    cputs("ELAPSED JIFFIES: ");
    print_u32((uint32_t)elapsed);
    cputs("\r\n");
    cputs("(PAL: 50 JIFFIES APPROX 1 SEC)\r\n");
    cputs("\r\nPRESS A KEY.\r\n");
    cgetc();
    return 0;
}
