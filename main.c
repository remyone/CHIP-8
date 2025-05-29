#include "func.c"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>


#define dt "\x1b[0m"
#define red "\x1b[31m"

struct termios original_termios;

void disable_input_buffering() {
    tcgetattr(STDIN_FILENO, &original_termios);
    struct termios new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

void restore_input_buffering() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

int kbhit() {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

const char keymap[16] = {
    'x', '1', '2', '3',   // 0-3
    'q', 'w', 'e', 'a',   // 4-7
    's', 'd', 'z', 'c',   // 8-B
    '4', 'r', 'f', 'v'    // C-F
};

int main(int argc, char **argv) {
    if (argc < 2) {
        printf(red"Usage: ./rom\n"dt);
        return -1;
    }
    srand(time(NULL));
    init();
    load_rom(argv[1]);
    //test();
    FILE *log = fopen("logs.txt", "w");

    fprintf(log, "| ROM - %s |\n\n", argv[1]);

    putchar('\n');

    while (1) {
        if (pc >= 0x1000) {
            printf("PC out of bonds: 0x%X\n", pc);
            exit(1);
        }

        disable_input_buffering();

        if (kbhit()) {
            char key = getchar();
            // ESC to exit
            if (key == 27) break;
            
            for (int i = 0; i < 16; i++) {
                if (key == keymap[i]) {
                    keypad[i] = 1;
                    break;
                }
            }
        }
        
        execute(log);
        render_screen();

        if (delay_timer > 0)
            delay_timer--;
        
        if (sound_timer > 0)
            sound_timer--;


        usleep(1000000 / 60);
    }

    restore_input_buffering();

    fclose(log);
    return 0;
}