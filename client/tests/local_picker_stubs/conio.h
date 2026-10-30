#ifndef SEE64YOU_LOCAL_PICKER_CONIO_TEST_H
#define SEE64YOU_LOCAL_PICKER_CONIO_TEST_H
#define CH_STOP 3u
unsigned char cgetc(void);
void clrscr(void);
void cputs(const char* text);
void cputc(char c);
void gotoxy(unsigned char x, unsigned char y);
void revers(unsigned char enabled);
#endif
