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
unsigned char font[80] = {0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
                          0x20, 0x60, 0x20, 0x20, 0x70, // 1
                          0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
                          0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
                          0x90, 0x90, 0xF0, 0x10, 0x10, // 4
                          0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
                          0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
                          0xF0, 0x10, 0x20, 0x40, 0x40, // 7
                          0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
                          0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
                          0xF0, 0x90, 0xF0, 0x90, 0x90, // A
                          0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
                          0xF0, 0x80, 0x80, 0x80, 0xF0, // C
                          0xE0, 0x90, 0x90, 0x90, 0xE0, // D
                          0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
                          0xF0, 0x80, 0xF0, 0x80, 0x80}; // F

unsigned short I;
unsigned char V[16];
unsigned char display[32][64];
unsigned char keypad[16];
unsigned char dis_updated = 0;
unsigned short pc;
unsigned short sp;
unsigned char delay_timer, sound_timer;

void execute_DXYN(unsigned char X, unsigned char Y, unsigned char N) {
    unsigned char Vx = V[X] % 64;
    unsigned char Vy = V[Y] % 32;
    V[CF] = 0;

    for (int row = 0; row < N; ++row) {
        unsigned char sprite_byte = memory[I + row];

        for (int col = 0; col < 8; ++col) {
            if (sprite_byte & (0x80 >> col)) {
                int x = (Vx + col) % 64;
                int y = (Vy + row) % 32;

                if (display[y][x] == 1)
                    V[CF] = 1;

                display[y][x] ^= 1;
            }
        }
    }

    dis_updated = 1;
}

void render_screen() {
    //system("clear");
    printf("\033[H");

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 64; ++x) {
            printf("%s", display[y][x] ? "##" : "  ");
        }
        putchar('\n');
    }
}

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
    memset(display, 0, sizeof(display));
    memset(stack, 0, 16);
    memset(V, 0, 16);
    memcpy(&memory[0x50], font, 80);
}

void test() {
    unsigned char program[] = {0x80, 0x1E};
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
                dis_updated = 1;
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
                case 0x800E:
                    V[X] = V[Y] << 1;
                    V[CF] = (V[Y] >> 7) & 1;
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
                case 0xF055:
                    for (int i = 0; i <= X; ++i)
                        memory[I + i] = V[i];
                    I += (X + 1);
                    fprintf(log, "[Store the values of registers V0 to V%d inclusive in memory starting at address 0x%04X]\n\n", X, I);
                    break;
                case 0xF065:
                    for (int i = 0; i <= X; ++i)
                        V[i] = memory[I + i];
                    fprintf(log, "[Fill registers V0 to V%d inclusive with the values stored in memory starting at address 0x%04X]\n\n", X, I);
                    I += (X + 1);
                    break;
            }
            pc += 2;
            break;
        case 0xD000:
            unsigned char N = opcode & 0x000F;
            execute_DXYN(X, Y, N);
            fprintf(log, "[DXYN: x=%d, y=%d, I=0x%X, N=%d, byte=0x%02X]\n\n", V[X], V[Y], I, N, memory[I]);
            pc += 2;
            break;
        case 0xE000:
            X = (opcode & 0x0F00) >> 8;
            switch (opcode & 0xF0FF) {
                case 0xE09E:
                    if (keypad[V[X]] == 1)
                        pc += 4;
                    else 
                        pc += 2;
                    break;
                case 0xE0A1:
                    if (keypad[V[X]] == 0)
                        pc += 4;
                    else 
                        pc += 2;
                    break;
            }
            break;
        default:
            fprintf(log, "[unknown opcode : [0x%X]]\n\n", opcode);
            pc += 2;
    }
}