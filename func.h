#ifndef _FUNC_H_

#define _FUNC_H_

#include <stdio.h>

extern unsigned short opcode;
extern unsigned char memory[4096];
extern unsigned short stack[16];
extern unsigned short I;
extern unsigned char V[16];
extern unsigned char display[64 * 32];
extern unsigned short pc;
extern unsigned short sp;
extern unsigned char delay_timer, sound_timer;

extern void load_rom(char *name);
extern void init();
extern void test();
extern int random_num_with_NN_mask(unsigned char NN);//func for opcode CXNN
extern void execute(FILE *log); // emaculate cycle

#endif