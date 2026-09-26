#include <cbm.h>
#include <c64.h>
#include <conio.h>

#include "input.h"
#include "charset.h"

#define S64_SHFLAG (*(volatile unsigned char*)0x028Du)
#define S64_KEYCODE (*(volatile unsigned char*)0x00CBu)
#define S64_KEY_NONE 0x40u
#define S64_KEY_DELETE 0x00u
#define S64_KEY_CURSOR_HORIZONTAL 0x02u
#define S64_KEY_CURSOR_VERTICAL 0x07u
#define S64_MOD_SHIFT 0x01u
#define S64_MOD_COMMODORE 0x02u
#define S64_MOD_CONTROL 0x04u

static unsigned char s64_is_printable(unsigned char key)
{
    uint8_t ascii;

    return s64_charset_petscii_to_ascii(key, &ascii)
        && ascii >= 0x20u
        && ascii <= 0x7Eu;
}

static S64InputType s64_control_command(unsigned char key, unsigned char modifiers)
{
    switch (key) {
        case 0x01u: return S64_INPUT_SELECT_ALL; /* Ctrl+A */
        case 0x03u: return S64_INPUT_COPY;       /* Ctrl+C */
        case 0x06u: return S64_INPUT_FIND;       /* Ctrl+F */
        case 0x07u: return S64_INPUT_GOTO_LINE;  /* Ctrl+G */
        case 0x08u: return S64_INPUT_REPLACE;    /* Ctrl+H */
        case 0x0Eu: return S64_INPUT_NEW;        /* Ctrl+N */
        case 0x0Fu: return S64_INPUT_OPEN;       /* Ctrl+O */
        case 0x13u: return (modifiers & S64_MOD_SHIFT) != 0u
            ? S64_INPUT_SAVE_AS
            : S64_INPUT_SAVE;                   /* Ctrl+Shift+S / Ctrl+S */
        case 0x16u: return S64_INPUT_PASTE;       /* Ctrl+V */
        case 0x18u: return S64_INPUT_CUT;         /* Ctrl+X */
        case 0x1Au: return (modifiers & S64_MOD_SHIFT) != 0u
            ? S64_INPUT_REDO
            : S64_INPUT_UNDO;                    /* Ctrl+Shift+Z / Ctrl+Z */
        default: return S64_INPUT_NONE;
    }
}

void s64_input_read(S64InputEvent* event)
{
    unsigned char key;
    unsigned char raw_key;
    unsigned char modifiers;

    event->type = S64_INPUT_NONE;
    event->character = 0u;

    modifiers = S64_SHFLAG;
    raw_key = S64_KEYCODE;
    cursor(1);
    key = (unsigned char)cgetc();
    cursor(0);
    modifiers |= S64_SHFLAG;
    if (S64_KEYCODE != S64_KEY_NONE) {
        raw_key = S64_KEYCODE;
    }

    if ((modifiers & (S64_MOD_CONTROL | S64_MOD_COMMODORE)) != 0u) {
        if (raw_key == S64_KEY_CURSOR_HORIZONTAL) {
            event->type = (modifiers & S64_MOD_SHIFT) != 0u
                ? S64_INPUT_WORD_LEFT
                : S64_INPUT_WORD_RIGHT;
            return;
        }
        if (raw_key == S64_KEY_DELETE) {
            event->type = S64_INPUT_DELETE;
            return;
        }

        if (raw_key == S64_KEY_NONE) {
            if (key == CH_CURS_LEFT) {
                event->type = S64_INPUT_WORD_LEFT;
                return;
            }
            if (key == CH_CURS_RIGHT) {
                event->type = S64_INPUT_WORD_RIGHT;
                return;
            }
            if (key == CH_DEL) {
                event->type = S64_INPUT_DELETE;
                return;
            }
        }
    }

    if ((modifiers & S64_MOD_CONTROL) != 0u) {
        event->type = s64_control_command(key, modifiers);
        if (event->type != S64_INPUT_NONE) {
            return;
        }
    }

    if ((modifiers & S64_MOD_COMMODORE) != 0u) {
        if (raw_key == S64_KEY_CURSOR_VERTICAL) {
            event->type = (modifiers & S64_MOD_SHIFT) != 0u
                ? S64_INPUT_PAGE_UP
                : S64_INPUT_PAGE_DOWN;
            return;
        }

        switch (key) {
            case CH_F1:
                event->type = S64_INPUT_NEW;
                return;
            case CH_F3:
                event->type = S64_INPUT_OPEN;
                return;
            case CH_F5:
                event->type = S64_INPUT_SAVE;
                return;
            case CH_F7:
                event->type = S64_INPUT_SAVE_AS;
                return;
            case CH_CURS_UP:
                event->type = S64_INPUT_PAGE_UP;
                return;
            case CH_CURS_DOWN:
                event->type = S64_INPUT_PAGE_DOWN;
                return;
            default:
                break;
        }
    }

    switch (key) {
        case CH_CURS_LEFT:
            event->type = S64_INPUT_LEFT;
            return;
        case CH_CURS_RIGHT:
            event->type = S64_INPUT_RIGHT;
            return;
        case CH_CURS_UP:
            event->type = S64_INPUT_UP;
            return;
        case CH_CURS_DOWN:
            event->type = S64_INPUT_DOWN;
            return;
        case CH_HOME:
            event->type = S64_INPUT_HOME;
            return;
        case CH_DEL:
            event->type = S64_INPUT_BACKSPACE;
            return;
        case CH_INS:
            event->type = S64_INPUT_TOGGLE_INSERT;
            return;
        case CH_ENTER:
            event->type = S64_INPUT_NEWLINE;
            return;
        case CH_F1:
            event->type = S64_INPUT_TOGGLE_MARK;
            return;
        case CH_F2:
            event->type = S64_INPUT_FIND_NEXT;
            return;
        case CH_F3:
            event->type = S64_INPUT_COPY;
            return;
        case CH_F4:
            event->type = S64_INPUT_FIND_PREVIOUS;
            return;
        case CH_F5:
            event->type = S64_INPUT_CUT;
            return;
        case CH_F6:
            event->type = S64_INPUT_REPLACE;
            return;
        case CH_F7:
            event->type = S64_INPUT_PASTE;
            return;
        case CH_F8:
            event->type = S64_INPUT_TOGGLE_WRAP;
            return;
        case CH_STOP:
            event->type = S64_INPUT_EXIT;
            return;
        default:
            break;
    }

    if (s64_is_printable(key)) {
        event->type = S64_INPUT_CHARACTER;
        event->character = key;
    }
}
