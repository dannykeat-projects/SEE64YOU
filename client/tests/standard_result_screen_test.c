#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "publisher/standard_display.h"

/*
 * Host screen model for the shared Publisher status renderer in
 * src/publisher/standard_display.c.  The model grid is the shipped 40 columns
 * by 25 rows, so a write outside it is a defect, and every row is checked
 * character by character.  The result strings mirror what
 * standard_main.c:show_result() hands to the renderer.
 */

static char screen[25][40];
static int pen_column;
static int pen_row;

void clrscr(void)
{
    memset(screen, ' ', sizeof screen);
    pen_column = 0;
    pen_row = 0;
}

void gotoxy(unsigned char x, unsigned char y)
{
    pen_column = (int)x;
    pen_row = (int)y;
}

void cputc(char value)
{
    if (pen_row < 0 || pen_row >= 25 || pen_column < 0 || pen_column >= 40) {
        fprintf(stderr, "FAIL: write outside the 40x25 grid\n");
        exit(1);
    }
    screen[pen_row][pen_column++] = value;
}

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        exit(1);
    }
}

/* Compare one grid row against a string padded with spaces. */
static void expect_row(unsigned char row, const char* expected, const char* why)
{
    unsigned int column;
    unsigned int expected_length = (unsigned int)strlen(expected);

    for (column = 0u; column < 40u; ++column) {
        require(screen[row][column]
                == (column < expected_length ? expected[column] : ' '),
            why);
    }
}

/* Snapshot and re-check a row that the status window must not disturb. */
static void snapshot(unsigned char row, char* before)
{
    memcpy(before, screen[row], 40u);
}

static void expect_unchanged(unsigned char row, const char* before, const char* why)
{
    unsigned int column;

    for (column = 0u; column < 40u; ++column) {
        require(screen[row][column] == before[column], why);
    }
}

/* Render and check that the reply is split into three exact 39-column rows,
 * truncated to the stored reply, with every other grid row left clear. */
static void render_and_expect(const char* message, unsigned char first_row,
    int others_clear)
{
    char rows[3][40];
    unsigned int stored;
    unsigned int row;
    unsigned int column;

    s64_publish_message(message);
    s64_publish_draw(first_row);

    stored = (unsigned int)strlen(message);
    if (stored > 80u) {
        stored = 80u;
    }

    for (row = 0u; row < 3u; ++row) {
        unsigned int offset = row * 39u;
        unsigned int written = 0u;

        for (column = 0u;
             column < 39u && offset + column < stored;
             ++column) {
            rows[row][written++] = message[offset + column];
        }
        rows[row][written] = 0;
    }

    for (row = 0u; row < 3u; ++row) {
        expect_row((unsigned char)(first_row + row), rows[row],
            "each status row shows exactly its slice of the reply");
    }

    if (!others_clear) {
        return;
    }
    for (row = 0u; row < 25u; ++row) {
        if (row >= first_row && row < first_row + 3u) {
            continue;
        }
        for (column = 0u; column < 40u; ++column) {
            require(screen[row][column] == ' ',
                "grid rows outside the status window stay clear");
        }
    }
}

int main(void)
{
    static const char* length_texts[] = {
        "",
        "X",
        "12345678901234567890123456789012345678",
        "123456789012345678901234567890123456789",
        "1234567890123456789012345678901234567890",
        "12345678901234567890123456789012345678901234567890123456789012345678",
        "123456789012345678901234567890123456789012345678901234567890123456789",
        "12345678901234567890123456789012345678901234567890123456789012345678901",
        "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"
    };
    static const char* result_texts[] = {
        "Outcome unknown. Inspect remote first.",
        "Source exceeds server limit",
        "Publishing failed; check endpoint/key",
        "HTTPS unavailable; no downgrade",
        "Allow HTTP explicitly in Settings"
    };
    char before[40];
    char row39[40];
    char long_text[81];
    unsigned int i;

    /* Length sweep: 0, 1, 38, 39, 40, 78, 79, 80 and one over the limit. */
    for (i = 0u; i < sizeof length_texts / sizeof length_texts[0]; ++i) {
        clrscr();
        render_and_expect(length_texts[i], 22u, 1);
    }

    /* A split that lands on a space and one that breaks a word. */
    clrscr();
    render_and_expect(
        "12345678901234567890123456789012345678 word two three four five six",
        22u, 1);
    clrscr();
    render_and_expect(
        "123456789012345678901234567890123456789word223456789012345678901234567890",
        22u, 1);

    /* Every shipped result message renders inside the three-row window. */
    for (i = 0u; i < sizeof result_texts / sizeof result_texts[0]; ++i) {
        require(strlen(result_texts[i]) <= 80u, "a result message fits the stored reply");
        clrscr();
        render_and_expect(result_texts[i], 22u, 1);
    }

    /* Bottom-of-grid fit: rows 22, 23 and 24 are the last three grid rows. */
    clrscr();
    render_and_expect(
        "1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",
        22u, 1);
    require(screen[24][2] == ' ', "the last status row stays inside its 39 columns");

    /* A long reply followed by a short one leaves no stale letters. */
    clrscr();
    s64_publish_message("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012");
    s64_publish_draw(22u);
    clrscr();
    render_and_expect("OK", 22u, 1);

    /* An over-long reply is truncated at the stored limit, never read past it. */
    memset(long_text, 'a', sizeof long_text - 1u);
    long_text[sizeof long_text - 1u] = 0;
    clrscr();
    render_and_expect(long_text, 22u, 1);

    /* Browse recovery: status at rows 20..22 must not touch the menu rows. */
    /* Browse recovery, in the shipped order: clear, draw the reply from row
     * 20, then paint the menu rows below the three-row window. */
    clrscr();
    s64_publish_message("Outcome unknown. Inspect remote first.");
    s64_publish_draw(20u);
    s64_publish_put(23u, "7 SET NAME  0 BACK", 39u);
    s64_publish_put(24u, "HELP", 39u);
    expect_row(20u, "Outcome unknown. Inspect remote first.",
        "the recovery screen shows the reply from row 20");
    expect_row(21u, "", "the shorter reply leaves the second status row clear");
    expect_row(22u, "", "the third status row stays clear");
    expect_row(23u, "7 SET NAME  0 BACK", "the menu row sits below the status window");
    expect_row(24u, "HELP", "the menu row at the last grid row stays inside the grid");

    /* A three-row reply must still stop above the menu rows: the window is
     * drawn with the menu already painted, so a write into row 23 would
     * overwrite the menu text. */
    clrscr();
    s64_publish_put(23u, "7 SET NAME  0 BACK", 39u);
    snapshot(23u, before);
    s64_publish_message(long_text);
    s64_publish_draw(20u);
    memset(row39, 'a', 39u);
    row39[39] = 0;
    expect_row(20u, row39, "the first status row takes exactly 39 characters");
    expect_row(21u, row39, "the second status row takes exactly 39 characters");
    expect_row(22u, "aa", "the third status row takes only the leftover pair");
    expect_unchanged(23u, before, "a three-row reply does not disturb the menu row");

    puts("standard publisher result screen renders within the 40x25 grid");
    return 0;
}
