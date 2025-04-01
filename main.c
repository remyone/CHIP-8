#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 4096
#define dt "\x1b[0m"
#define red "\x1b[31m"

unsigned short opcode;
unsigned char memory[MEMORY_SIZE];
unsigned short stack[16];
unsigned short I;
unsigned char V[16] = {0};
unsigned char display[64 * 32];
unsigned short pc = 0x200;
unsigned short sp;

void load_rom(char *name) {
    FILE *ROM = fopen(name, "rb");
    fseek(ROM, 0, SEEK_END);
    int size = ftell(ROM);
    fseek(ROM, 0, SEEK_SET);
    fread(memory + 0x200, 1, size, ROM);
    fclose(ROM);
}

//emaculate cycle
void execute(FILE *log) {
    opcode = memory[pc] << 8 | memory[pc + 1];
    fprintf(log, "PC : [0x%04X] | OPCODE : [0x%04x]\n", pc, opcode);

    unsigned char X, NN;
    //printf("NN : %02X, Register : V%01X\n", NN, X);

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                memset(display, 0, sizeof(display));
                fprintf(log, "[Clear screen]\n\n");
            }
            pc += 2;
            break;
        case 0x1000:
            pc = opcode & 0x0FFF;
            fprintf(log, "Jump to 0x%03X\n\n", pc);
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
                fprintf(log, "Jump to 0x%03X\n\n", pc);
            }
            break;
        case 0x4000:
            X = (opcode & 0x0F00) >> 8;
            NN = opcode & 0x00FF;
            if (V[X] != NN) {
                pc += 4;
                fprintf(log, "Jump to 0x%03X\n\n", pc);
            }
            break;
        case 0x6000:
            X = (opcode & 0x0F00) >> 8;
            NN = opcode & 0x00FF;
            V[X] = NN;
            printf("Store number %02X in register V%01X\n\n", NN, X);
            fprintf(log, "Store number %02X in register V%01X\n\n", NN, X);
            pc += 2;
            break;
        case 0xA000:
            I = opcode & 0x0FFF;
            printf("Store %02X in register I", opcode & 0x0FFF);
            fprintf(log, "Store %02X in register I\n\n", opcode & 0x0FFF);
            pc += 2;
            break;
        case 0xB000:
            pc = (opcode & 0x0FFF) + V[0];
            fprintf(log, "Jump to 0x%03X\n\n", pc);
            break;
        default:
            fprintf(log, "unknown opcode : [0x%X]\n\n", opcode);
            pc += 2;
    }
}

void test() {
    unsigned char program[] = {0x60, 0x04,
                               0xB2, 0x02,
                               0xA2, 0x06,
                               0x00, 0xE0};
    memcpy(memory + 0x200, program, sizeof(program));
}

int main(int argc, char **argv) {
    /*if (argc < 2) {
        printf(red"Usage: ./rom\n"dt);
        return -1;
    }*/
    FILE *log = fopen("logs.txt", "a");
    
    test();
    
    for (int i = 0; i < 3; ++i) 
        execute(log);

    fclose(log);
    return 0;
}