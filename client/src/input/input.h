#ifndef SEE64YOU_INPUT_H
#define SEE64YOU_INPUT_H

typedef enum {
    S64_INPUT_NONE = 0,
    S64_INPUT_CHARACTER,
    S64_INPUT_LEFT,
    S64_INPUT_RIGHT,
    S64_INPUT_UP,
    S64_INPUT_DOWN,
    S64_INPUT_HOME,
    S64_INPUT_END,
    S64_INPUT_PAGE_UP,
    S64_INPUT_PAGE_DOWN,
    S64_INPUT_WORD_LEFT,
    S64_INPUT_WORD_RIGHT,
    S64_INPUT_BACKSPACE,
    S64_INPUT_DELETE,
    S64_INPUT_NEWLINE,
    S64_INPUT_TOGGLE_INSERT,
    S64_INPUT_TOGGLE_WRAP,
    S64_INPUT_HELP,
    S64_INPUT_MENU,
    S64_INPUT_ACTIVATE,
    S64_INPUT_POINTER_CLICK,
    S64_INPUT_POINTER_DRAG,
    S64_INPUT_TOGGLE_MARK,
    S64_INPUT_COPY,
    S64_INPUT_CUT,
    S64_INPUT_PASTE,
    S64_INPUT_SELECT_ALL,
    S64_INPUT_NEW,
    S64_INPUT_OPEN,
    S64_INPUT_SAVE,
    S64_INPUT_SAVE_AS,
    S64_INPUT_FIND,
    S64_INPUT_FIND_NEXT,
    S64_INPUT_FIND_PREVIOUS,
    S64_INPUT_GOTO_LINE,
    S64_INPUT_REPLACE,
    S64_INPUT_UNDO,
    S64_INPUT_REDO,
    S64_INPUT_FORMAT_BOLD,
    S64_INPUT_FORMAT_ITALIC,
    S64_INPUT_FORMAT_LIST,
    S64_INPUT_PUBLISH,
    S64_INPUT_EXIT
} S64InputType;

typedef struct {
    S64InputType type;
    unsigned char character;
    unsigned char column;
    unsigned char row;
} S64InputEvent;

void s64_input_init(void);
void s64_input_shutdown(void);
void s64_input_read(S64InputEvent* event, unsigned char idle_jiffies);
void s64_input_sync_repeat(void);
void s64_input_restore_repeat(void);

#endif
