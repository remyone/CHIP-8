#include "func.h"
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 4096
#define CF 0xF
#define dt "\x1b[0m"
#define red "\x1b[31m"

unsigned short opcode;
unsigned char memory[MEMORY_SIZE];
unsigned short stack[16];
unsigned short I;
unsigned char V[16];
unsigned char display[64 * 32];
unsigned short pc;
unsigned short sp;
unsigned char delay_timer, sound_timer;

void load_rom(char *name) {
    FILE *ROM = fopen(name, "rb");
    fseek(ROM, 0, SEEK_END);
    int size = ftell(ROM);
    fseek(ROM, 0, SEEK_SET);
    fread(memory + 0x200, 2, size, ROM);
    fclose(ROM);
}

void init() {
    opcode = 0;
    sp = 0;
    I = 0;
    delay_timer = 0;
    sound_timer = 0;
    pc = 0x200;
    memset(memory, 0, MEMORY_SIZE);
    memset(display, 0, 64 * 32);
    memset(stack, 0, 16);
    memset(V, 0, 16);
}

void test() {
    unsigned char program[] = {0xF2, 0x07,
			                   0xC3, 0xFF,
			                   0xF3, 0x15,
			                   0xF3, 0x18,
			                   0xC0, 0xFF,
			                   0xC1, 0xFF,
			                   0xF3, 0x55};
    memcpy(memory + 0x200, program, sizeof(program));
}

int random_num_with_NN_mask(unsigned char NN) {
    int rnd_num = rand() % NN + 1;
    return rnd_num;
}

//emaculate cycle
void execute(FILE *log) {
    opcode = memory[pc] << 8 | memory[pc + 1];
    fprintf(log, "PC : [0x%04X] | OPCODE : [0x%04x]\n", pc, opcode);

    unsigned char X, NN, Y;
    //printf("NN : %02X, Register : V%01X\n", NN, X);

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                memset(display, 0, sizeof(display));
                fprintf(log, "[Clear screen]\n\n");
            } else if (opcode == 0x0000) {
                unsigned short address = opcode & 0x0FFF;
                fprintf(log, "Warning: Machine code call at 0x%03X (unsupported in emulators)\n\n", address);
            }
            pc += 2;
            break;
        case 0x1000:
            unsigned short tmp_pc = opcode & 0x0FFF;
            if (pc == tmp_pc) {
                fprintf(log, "BUG : [NNN(0x%03X) in opcode 1NNN equals pc]\n\n", tmp_pc);
                pc += 2;
            } else {
                pc = tmp_pc;
                fprintf(log, "[Jump to 0x%03X]\n\n", pc);
            }
            break;
        case 0x2000:
            stack[sp] = pc + 2;
            ++sp;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000:
            X = (opcode & 0x0F00) >> 8;
            NN = opcode & 0x00FF;
            if (V[X] == NN) {
                pc += 4;
                fprintf(log, "[Jump to 0x%03X]\n\n", pc);
            } else 
                pc += 2;
            break;
        case 0x4000:
            X = (opcode & 0x0F00) >> 8;
            NN = opcode & 0x00FF;
            if (V[X] != NN) {
                pc += 4;
                fprintf(log, "[Jump to 0x%03X]\n\n", pc);
            } else
                pc += 2;
            break;
        case 0x5000:
            X = (opcode & 0x0F00) >> 8;
            Y = (opcode & 0x00F0) >> 4;
            if (V[X] == V[Y]) {
                pc += 4;
                fprintf(log, "[Jump to 0x%03X]\n\n", pc);
            } else
                pc += 2;
            break;
        case 0x6000:
            X = (opcode & 0x0F00) >> 8;
            NN = opcode & 0x00FF;
            V[X] = NN;
            fprintf(log, "[Store number %02X in register V%01X]\n\n", NN, X);
            pc += 2;
            break;
        case 0x7000:
            X = (opcode & 0x0F00) >> 8;
            NN = opcode & 0x00FF;
            V[X] += NN;
            fprintf(log, "[Add %02X in register V%01X]\n\n", NN, X);
            pc += 2;
            break;
        case 0x8000:
            X = (opcode & 0x0F00) >> 8;
            Y = (opcode & 0x00F0) >> 4;
            switch (opcode & 0x800F) {
                case 0x8000:
                    V[X] = V[Y];
                    fprintf(log, "[Store the value of register V%01X in register V%01X]\n\n", Y, X);
                    break;
                case 0x8001:
                    V[X] |= V[Y];
                    fprintf(log, "[Set V%01X to V%01X OR V%01X]\n\n", X, X, Y);
                    break;
                case 0x8002:
                    V[X] &= V[Y];
                    fprintf(log, "[Set V%01X to V%01X AND V%01X]\n\n", X, X, Y);
                    break;
                case 0x8003:
                    V[X] ^= V[Y];
                    fprintf(log, "[Set V%01X to V%01X XOR V%01X]\n\n", X, X, Y);
                    break;
                case 0x8004:
                    V[CF] = ((V[X] + V[Y]) > 255) ? 1 : 0;
                    V[CF] == 1 ? fprintf(log, "[Carry flag equals 1]\n") : fprintf(log, "[Carry flag equals 0]\n");
                    V[X] += V[Y];
                    fprintf(log, "[Add the value of register V%01X to register V%01X]\n\n", Y, X);
                    break;
                case 0x8005:
                    V[CF] = ((V[X] + V[Y]) > 255) ? 1 : 0;
                    V[CF] == 1 ? fprintf(log, "[Carry flag equals 1]\n") : fprintf(log, "[Carry flag equals 0]\n");
                    V[Y] -= V[X];
                    fprintf(log, "[Subtract the value of register V%01X from register V%01X]\n\n", Y, X);
                    break;
                case 0x8007:
                    V[CF] = (V[X] + V[Y] > 255) ? 1 : 0;
                    V[CF] == 1 ? fprintf(log, "[Carry flag equals 1]\n") : fprintf(log, "[Carry flag equals 0]\n");
                    V[X] = V[Y] - V[X];
                    fprintf(log, "[Set register V%01X to the value of V%01X minus V%01X]\n\n", X, Y, X);
                    break;
            }
            pc += 2;
            break;
        case 0x9000:
            if (V[X] != V[Y]) {
                pc += 4;
                fprintf(log, "[Jump to 0x%03X]\n\n", pc);
            } else
                pc += 2;
            break;
        case 0xA000:
            I = opcode & 0x0FFF;
            fprintf(log, "[Store %03X in register I]\n\n", opcode & 0x0FFF);
            pc += 2;
            break;
        case 0xB000:
            pc = (opcode & 0x0FFF) + V[0];
            fprintf(log, "[Jump to 0x%03X]\n\n", pc);
            break;
        case 0xC000:
            X = (opcode & 0x0F00) >> 8;
            NN = opcode & 0x00FF;
            int random_num = random_num_with_NN_mask(NN);
            V[X] = random_num & NN;
            pc += 2;
            break;
        case 0xF000:   
            X = (opcode & 0x0F00) >> 8;
            switch (opcode & 0xF0FF) {
                case 0xF01E:
                    I += V[X];
                    fprintf(log, "[Add value stored in V[%d] to I]\n\n", V[X]);
                    break;
                case 0xF055:
                    for (int i = 0; i < X; ++i)
                        memory[I + i] = V[i];
                    I += (X + 1);
                    fprintf(log, "[Store the values of registers V0 to V%d inclusive in memory starting at address I]\n\n", X);
                    printf("I = %d\n", I);
                    break;
                case 0xF007:
                    V[X] = delay_timer;
                    fprintf(log, "[Store the current value of the delay timer(%d) in V%d]\n\n", delay_timer, X);
                    break;
                case 0xF015:
                    fprintf(log, "[Set the delay timer to the value of register V%d(value is %d)]\n\n", X, V[X]);
                    delay_timer = V[X];
                    break;
                case 0xF018:
                    fprintf(log, "[Set the sound timer to the value of register V%d(value is %d)]\n\n", X, V[X]);
                    sound_timer = V[X];
                    break;
            }
            pc += 2;
            break;
        default:
            fprintf(log, "[unknown opcode : [0x%X]]\n\n", opcode);
            pc += 2;
    }
}