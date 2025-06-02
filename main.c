#include "func.c"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf(red"Usage: ./rom\n"dt);
        return -1;
    }

    srand(time(NULL));
    init();
    load_rom(argv[1]);

    FILE *log = fopen("logs.txt", "w");

    fprintf(log, "| ROM - %s |\n\n", argv[1]);

    putchar('\n');

    while (1) {
        if (pc >= 0x1000) {
            printf("PC out of bonds: 0x%X\n", pc);
            exit(1);
        }

        execute(log);
        render_screen();

        if (delay_timer > 0)
            delay_timer--;
        
        if (sound_timer > 0)
            sound_timer--;


        usleep(1000000 / 60);
    }

    fclose(log);
    return 0;
}