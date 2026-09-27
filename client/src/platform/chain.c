#include "platform/chain.h"

#include <device.h>
#include <stdlib.h>

#pragma data-name(push, "LOWCODE")
static struct S64ChainLine {
    const char end_of_line;
    const struct S64ChainLine* const next;
    const unsigned line_number;
    const char clr_token;
    const char colon;
    const char load_token;
    const char empty_quotes[2];
    const char add_token;
    const char opening_quote;
    char name[21];
    const char comma;
    char unit[3];
} chain_line = {
    '\0',
    &chain_line + 1,
    0,
    (char)0x9c,
    ':',
    (char)0x93,
    "\"\"",
    (char)0xaa,
    '\"',
    "S64PUB.PRG\"          ",
    ',',
    "08"
};
#pragma data-name(pop)

extern const void* vartab;
#pragma zpsym("vartab")
extern const void* memsize;
#pragma zpsym("memsize")
extern const struct S64ChainLine* txtptr;
#pragma zpsym("txtptr")
extern char basbuf[];

static void set_device(unsigned char device)
{
    unsigned char tens = 0u;

    if (device < 8u || device > 30u) {
        device = 8u;
    }

    while (device >= 10u) {
        device = (unsigned char)(device - 10u);
        ++tens;
    }

    chain_line.unit[0] = (char)('0' + tens);
    chain_line.unit[1] = (char)('0' + device);
    chain_line.unit[2] = '\0';
}

void s64_chain_publisher(void)
{
    set_device(getcurrentdevice());

    /*
     * Match cc65's Commodore exec() setup, minus its generic filename,
     * command-line and pre-open machinery. The CLR fixes the remaining BASIC
     * variable pointers after cc65 restores BASIC on exit.
     */
    vartab = (char*)memsize - 256;
    basbuf[0] = (char)0x8f;
    basbuf[1] = '\0';
    txtptr = &chain_line;
    exit(0);
}
