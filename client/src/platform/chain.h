#ifndef SEE64YOU_PLATFORM_CHAIN_H
#define SEE64YOU_PLATFORM_CHAIN_H

/*
 * Replace the current C64 program with the packaged C64U publisher.
 * This deliberately uses BASIC program chaining so the next cc65 program
 * starts with a valid BASIC/KERNAL return context.
 */
void s64_chain_publisher(void);
void s64_chain_editor(void);

#endif
