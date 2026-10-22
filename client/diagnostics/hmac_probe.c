#include <conio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "crypto/sha256.h"

#define S64_TURBO_CONTROL (*(volatile uint8_t*)0xD031u)
#define S64_TURBO_SPEED_MASK 0x0fu
#define S64_TURBO_MAX_SPEED  0x0fu

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

static clock_t benchmark_8k(void)
{
    clock_t started;
    uint16_t blocks;

    s64_hmac_sha256_init(&hmac, key);
    started = clock();
    for (blocks = 0u; blocks < 128u; ++blocks) {
        s64_hmac_sha256_update(&hmac, block, sizeof(block));
    }
    s64_hmac_sha256_final(&hmac, digest);
    return clock() - started;
}

static uint8_t set_turbo_speed(uint8_t original, uint8_t speed)
{
    uint8_t requested = (uint8_t)(
        (original & (uint8_t)~S64_TURBO_SPEED_MASK)
        | (speed & S64_TURBO_SPEED_MASK)
    );

    S64_TURBO_CONTROL = requested;
    return (uint8_t)(S64_TURBO_CONTROL & S64_TURBO_SPEED_MASK)
        == (uint8_t)(speed & S64_TURBO_SPEED_MASK);
}

int main(void)
{
    static const uint8_t message[] = {0x61u, 0x62u, 0x63u};
    clock_t normal_elapsed;
    clock_t turbo_elapsed;
    uint8_t turbo_original;
    uint8_t turbo_available;
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

    memset(block, 0x41, sizeof(block));

    turbo_original = S64_TURBO_CONTROL;
    turbo_available = turbo_original != 0xffu;

    if (turbo_available && set_turbo_speed(turbo_original, 0u)) {
        cputs("8K HMAC AT 1MHZ...\r\n");
        normal_elapsed = benchmark_8k();
        cputs("JIFFIES: ");
        print_u32((uint32_t)normal_elapsed);
        cputs("\r\n");

        if (set_turbo_speed(turbo_original, S64_TURBO_MAX_SPEED)) {
            cputs("8K HMAC AT MAX TURBO...\r\n");
            turbo_elapsed = benchmark_8k();
            cputs("JIFFIES: ");
            print_u32((uint32_t)turbo_elapsed);
            cputs("\r\n");

            if (turbo_elapsed != 0u) {
                cputs("SPEEDUP X100: ");
                print_u32(
                    ((uint32_t)normal_elapsed * 100u)
                    / (uint32_t)turbo_elapsed
                );
                cputs("\r\n");
            }
        } else {
            cputs("MAX TURBO WRITE NOT ACCEPTED.\r\n");
        }

        S64_TURBO_CONTROL = turbo_original;
    } else {
        if (turbo_available) {
            S64_TURBO_CONTROL = turbo_original;
        }
        cputs("TURBO REGISTER UNAVAILABLE.\r\n");
        cputs("ENABLE C64U TURBO REG CONTROL.\r\n");
    }

    cputs("\r\n(PAL: 50 JIFFIES APPROX 1 SEC)\r\n");
    cputs("ORIGINAL TURBO STATE RESTORED.\r\n");
    cputs("\r\nPRESS A KEY.\r\n");
    cgetc();
    return 0;
}
