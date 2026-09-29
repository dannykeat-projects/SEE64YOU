#include <conio.h>
#include <stdint.h>
#include <string.h>

#include "crypto/sha256.h"

static uint8_t digest[32];
static uint8_t key[32];
static S64HmacSha256 hmac;

static char hex_digit(uint8_t value)
{
    value &= 0x0fu;
    return (char)(value < 10u ? '0' + value : 'A' + (value - 10u));
}

int main(void)
{
    static const uint8_t message[] = {'a','b','c'};
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
    cputs("HMAC('ABC'):\r\n");
    for (i = 0u; i < sizeof(digest); ++i) {
        cputc(hex_digit((uint8_t)(digest[i] >> 4)));
        cputc(hex_digit(digest[i]));
        if (i == 15u) cputs("\r\n");
    }
    cputs("\r\n\r\nEXPECTED PREFIX:\r\nF0133729C4163DEDE81E21CD47839256\r\n");
    cputs("\r\nPRESS A KEY.\r\n");
    cgetc();
    return 0;
}
