#include "func.c"
#include <time.h>
#include <stdlib.h>

#define dt "\x1b[0m"
#define red "\x1b[31m"

int main(int argc, char **argv) {
    /*if (argc < 2) {
        printf(red"Usage: ./rom\n"dt);
        return -1;
    }*/
    srand(time(NULL));
    init();
    //load_rom(argv[1]);
    test();

    FILE *log = fopen("logs.txt", "w");
    
    for (int i = 0; i < 7; ++i) 
        execute(log);//I've completed just 27/35 opcodes so far

    for (int i = 0; i < 3; ++i)
        printf("V%d = %d\n", i, V[i]);


    fclose(log);

    return 0;
}