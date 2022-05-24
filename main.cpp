
#include "sim_mem.h"

char main_memory[MEMORY_SIZE];

int main() {
    sim_mem mem_sm((char *) "/exec_file.txt", (char *) "swap_file", 25,
                25, 25, 25, 25, 5);
    int i, j;
    char value = 'X';
    for (i = 0; i < 25; i++) {
        mem_sm.load(i);
    }
    //Swap
    for (j = 25; j < 100; j++) {
        mem_sm.store(j, value);
        mem_sm.load(j);
    }
    mem_sm.print_memory();
    mem_sm.print_page_table();
    mem_sm.print_swap();
    return 0;
}
