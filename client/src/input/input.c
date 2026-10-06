#include <cbm.h>
#include <c64.h>
#include <conio.h>
#ifdef S64_REU_BUFFER
#include <joystick.h>
#include <mouse.h>
#endif

#include "input.h"
#include "charset.h"

#define S64_SHFLAG (*(volatile unsigned char*)0x028Du)
#define S64_KEYCODE (*(volatile unsigned char*)0x00CBu)
#define S64_KEY_COUNT (*(volatile unsigned char*)0x00C6u)
#define S64_JIFFY_LOW (*(volatile unsigned char*)0x00A2u)
#define S64_KEY_NONE 0x40u
#define S64_KEY_DELETE 0x00u
#define S64_KEY_CURSOR_HORIZONTAL 0x02u
#define S64_KEY_CURSOR_VERTICAL 0x07u
#define S64_MOD_SHIFT 0x01u
#define S64_MOD_COMMODORE 0x02u
#define S64_MOD_CONTROL 0x04u
#define S64_REPEAT_DELAY 18u
#define S64_REPEAT_RATE 4u

static unsigned char modifier_repeat_state = 0xffu;
static unsigned char modifier_repeat_tick;
#ifdef S64_REU_BUFFER
static unsigned char joystick_ready;
static unsigned char mouse_ready;
static unsigned char joystick_previous;
static unsigned char joystick_repeat_tick;
static unsigned char joystick_repeating;
static unsigned char mouse_previous_buttons;
static int mouse_previous_x;
static int mouse_previous_y;
static unsigned char mouse_visible;
#endif

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
        case 0x02u: return S64_INPUT_FORMAT_BOLD;/* Ctrl+B */
        case 0x03u: return S64_INPUT_COPY;       /* Ctrl+C */
        case 0x06u: return S64_INPUT_FIND;       /* Ctrl+F */
        case 0x07u: return S64_INPUT_GOTO_LINE;  /* Ctrl+G */
        case 0x08u: return S64_INPUT_REPLACE;    /* Ctrl+H */
        case 0x09u: return S64_INPUT_FORMAT_ITALIC;/* Ctrl+I */
        case 0x0Cu: return S64_INPUT_FORMAT_LIST; /* Ctrl+L */
        case 0x0Du: return S64_INPUT_MENU;        /* Ctrl+M where distinct */
        case 0x0Eu: return S64_INPUT_NEW;        /* Ctrl+N */
        case 0x0Fu: return S64_INPUT_OPEN;       /* Ctrl+O */
#ifdef S64_REU_BUFFER
        case 0x10u: return S64_INPUT_PUBLISH;    /* Ctrl+P */
#endif
        case 0x13u: return (modifiers & S64_MOD_SHIFT) != 0u
            ? S64_INPUT_SAVE_AS
            : S64_INPUT_SAVE;                   /* Ctrl+Shift+S / Ctrl+S */
        case 0x16u: return S64_INPUT_PASTE;       /* Ctrl+V */
        case 0x17u: return S64_INPUT_TOGGLE_WRAP; /* Ctrl+W */
        case 0x18u: return S64_INPUT_CUT;         /* Ctrl+X */
        case 0x1Au: return (modifiers & S64_MOD_SHIFT) != 0u
            ? S64_INPUT_REDO
            : S64_INPUT_UNDO;                    /* Ctrl+Shift+Z / Ctrl+Z */
        default: return S64_INPUT_NONE;
    }
}

static S64InputType s64_modifier_command(
    unsigned char raw_key,
    unsigned char modifiers
)
{
    if ((modifiers & S64_MOD_CONTROL) != 0u) {
        if (raw_key == S64_KEY_CURSOR_HORIZONTAL) {
            return (modifiers & S64_MOD_SHIFT) != 0u
                ? S64_INPUT_WORD_LEFT
                : S64_INPUT_WORD_RIGHT;
        }
        if (raw_key == S64_KEY_CURSOR_VERTICAL) {
            return (modifiers & S64_MOD_SHIFT) != 0u
                ? S64_INPUT_PAGE_UP
                : S64_INPUT_PAGE_DOWN;
        }
    }

    if ((modifiers & (S64_MOD_CONTROL | S64_MOD_COMMODORE)) != 0u
        && raw_key == S64_KEY_DELETE) {
        return S64_INPUT_DELETE;
    }

    return S64_INPUT_NONE;
}

static unsigned char s64_modifier_ready(
    unsigned char raw_key,
    unsigned char modifiers,
    S64InputType* command
)
{
    unsigned char signature;
    unsigned char now;
    unsigned char interval;

    *command = s64_modifier_command(raw_key, modifiers);
    if (*command == S64_INPUT_NONE) {
        modifier_repeat_state = 0xffu;
        return 0u;
    }

    signature = (unsigned char)((raw_key << 3) | (modifiers & 0x07u));
    now = S64_JIFFY_LOW;

    if ((modifier_repeat_state & 0x7fu) != signature) {
        modifier_repeat_state = signature;
        modifier_repeat_tick = now;
        return;
    }

    interval = (modifier_repeat_state & 0x80u) != 0u
        ? S64_REPEAT_RATE
        : S64_REPEAT_DELAY;

    if ((unsigned char)(now - modifier_repeat_tick) < interval) {
        return 0u;
    }

    modifier_repeat_state = (unsigned char)(signature | 0x80u);
    modifier_repeat_tick = now;
    return;
}

#ifdef S64_REU_BUFFER
static unsigned char s64_joystick_event(S64InputEvent* event)
{
    unsigned char state;
    unsigned char now;
    unsigned char interval;

    if (!joystick_ready) {
        return 0u;
    }

    state = joy_read(JOY_2);
    if (state == 0u) {
        joystick_previous = 0u;
        joystick_repeating = 0u;
        return 0u;
    }

    now = S64_JIFFY_LOW;
    if (state != joystick_previous) {
        joystick_previous = state;
        joystick_repeat_tick = now;
        joystick_repeating = 0u;
    } else {
        interval = joystick_repeating ? S64_REPEAT_RATE : S64_REPEAT_DELAY;
        if ((unsigned char)(now - joystick_repeat_tick) < interval) {
            return;
        }
        joystick_repeat_tick = now;
        joystick_repeating = 1u;
    }

    if (JOY_BTN_1(state)) event->type = S64_INPUT_ACTIVATE;
    else if (JOY_UP(state)) event->type = S64_INPUT_UP;
    else if (JOY_DOWN(state)) event->type = S64_INPUT_DOWN;
    else if (JOY_LEFT(state)) event->type = S64_INPUT_LEFT;
    else if (JOY_RIGHT(state)) event->type = S64_INPUT_RIGHT;
    else return 0u;
    return;
}

static unsigned char s64_mouse_event(S64InputEvent* event)
{
    struct mouse_info info;
    unsigned char pressed;

    if (!mouse_ready) {
        return 0u;
    }

    mouse_info(&info);
    pressed = (unsigned char)(info.buttons & MOUSE_BTN_LEFT);
    if (!mouse_visible
        && (info.pos.x != mouse_previous_x || info.pos.y != mouse_previous_y || pressed)) {
        mouse_show();
        mouse_visible = 1u;
    }
    if (pressed && !mouse_previous_buttons) {
        event->type = S64_INPUT_POINTER_CLICK;
        event->column = (unsigned char)(info.pos.x / 8);
        event->row = (unsigned char)(info.pos.y / 8);
        if (event->column > 39u) event->column = 39u;
        if (event->row > 24u) event->row = 24u;
        mouse_previous_buttons = pressed;
        mouse_previous_x = info.pos.x;
        mouse_previous_y = info.pos.y;
        return;
    }

    if (pressed && mouse_previous_buttons
        && (info.pos.x != mouse_previous_x || info.pos.y != mouse_previous_y)) {
        event->type = S64_INPUT_POINTER_DRAG;
        event->column = (unsigned char)(info.pos.x / 8);
        event->row = (unsigned char)(info.pos.y / 8);
        if (event->column > 39u) event->column = 39u;
        if (event->row > 24u) event->row = 24u;
        mouse_previous_x = info.pos.x;
        mouse_previous_y = info.pos.y;
        return;
    }

    mouse_previous_buttons = pressed;
    mouse_previous_x = info.pos.x;
    mouse_previous_y = info.pos.y;
    return 0u;
}
#endif

void s64_input_init(void)
{
#ifdef S64_REU_BUFFER
    struct mouse_box box;

    joystick_ready = joy_install((void*)joy_static_stddrv) == JOY_ERR_OK;
    mouse_ready = mouse_install(&mouse_def_callbacks, (void*)mouse_static_stddrv)
        == MOUSE_ERR_OK;
    joystick_previous = 0u;
    joystick_repeating = 0u;
    mouse_previous_buttons = 0u;
    mouse_previous_x = 160;
    mouse_previous_y = 100;
    mouse_visible = 0u;

    if (mouse_ready) {
        box.minx = 0;
        box.miny = 0;
        box.maxx = 319;
        box.maxy = 199;
        mouse_setbox(&box);
        mouse_move(160, 100);
    }
#endif
}

void s64_input_shutdown(void)
{
#ifdef S64_REU_BUFFER
    if (mouse_ready) {
        if (mouse_visible) mouse_hide();
        (void)mouse_uninstall();
        mouse_ready = 0u;
    }
    if (joystick_ready) {
        (void)joy_uninstall();
        joystick_ready = 0u;
    }
#endif
}

void s64_input_read(S64InputEvent* event, unsigned char idle_jiffies)
{
    unsigned char key;
    unsigned char raw_key;
    unsigned char modifiers;
    unsigned char idle_start;

    event->type = S64_INPUT_NONE;
    event->character = 0u;
    event->column = 0u;
    event->row = 0u;

    idle_start = S64_JIFFY_LOW;
    for (;;) {
        s64_input_sync_repeat();
        modifiers = S64_SHFLAG;
        raw_key = S64_KEYCODE;

#ifdef S64_REU_BUFFER
        if (s64_mouse_event(event) || s64_joystick_event(event)) {
            return;
        }
#endif

        if (s64_modifier_ready(raw_key, modifiers, &event->type)) {
            if (S64_KEY_COUNT != 0u) {
                (void)cbm_k_getin();
            }
            return;
        }

        if (kbhit()) {
            break;
        }
        if (idle_jiffies
            && (unsigned char)(S64_JIFFY_LOW - idle_start) >= idle_jiffies) {
            return 0u;
        }
    }

    key = (unsigned char)cgetc();
    modifiers |= S64_SHFLAG;

    if ((modifiers & S64_MOD_CONTROL) != 0u) {
        event->type = s64_control_command(key, modifiers);
        if (event->type != S64_INPUT_NONE) {
            return;
        }
    }

    if ((modifiers & S64_MOD_COMMODORE) != 0u) {
        if (key == 'm' || key == 'M') {
            event->type = S64_INPUT_MENU;
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
            event->type = S64_INPUT_HELP;
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
