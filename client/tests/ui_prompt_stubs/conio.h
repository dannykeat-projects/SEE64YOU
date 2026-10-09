#ifndef S64_UI_PROMPT_TEST_CONIO_H
#define S64_UI_PROMPT_TEST_CONIO_H
#define CH_STOP 3
#define CH_ENTER 13
#define CH_DEL 20
#define COLOR_BLACK 0
#define COLOR_GREEN 5
void clrscr(void);
void cputs(const char* text);
void cputc(char value);
void gotoxy(unsigned char x, unsigned char y);
unsigned char revers(unsigned char enabled);
unsigned char cursor(unsigned char enabled);
unsigned char textcolor(unsigned char color);
unsigned char bgcolor(unsigned char color);
unsigned char bordercolor(unsigned char color);
char cgetc(void);
#endif
