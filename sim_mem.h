//
// Created by Ben Alaluf
//

#ifndef EX6_SIM_MEM_H
#define EX6_SIM_MEM_H

#define MEMORY_SIZE 50
extern char main_memory[MEMORY_SIZE];
typedef struct page_descriptor
{
    int V; // valid
    int D; // dirty
    int P; // permission
    int frame; //the number of a frame if in case it is page-mapped
    int swap_index; // where the page is located in the swap file.
} page_descriptor;

class sim_mem {
    int swapfile_fd;
    int program_fd;
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int num_of_pages;
    int page_size;
    page_descriptor *page_table;
    int frameCounter;
    int *frameTable;
    int maxFrame;
    void memInsert(page_descriptor &p,int address);
    void swapInsert(page_descriptor &p,int seek);
    void loadMemory(page_descriptor &p, int page);

public:
    sim_mem(char exe_file_name[], char swap_file_name[], int text_size,
                     int data_size, int bss_size, int heap_stack_size,
                     int num_of_pages, int page_size);
    ~sim_mem();
    char load(int address);
    void store(int address, char value);
    void print_memory();
    void print_swap ();
    void print_page_table();
};


#endif //EX6_SIM_MEM_H
