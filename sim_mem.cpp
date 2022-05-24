//
// Created by Ben Alaluf
//

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cassert>
#include <sys/fcntl.h>
#include "sim_mem.h"

/**************************************************************************************/
sim_mem::sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size,
                 int bss_size, int heap_stack_size, int num_of_pages, int page_size){
    int i, seek, sum;
    char *buffer = new char[page_size];
    assert(buffer != NULL);
    this->text_size = text_size;
    this->data_size = data_size;
    this->bss_size = bss_size;
    this->heap_stack_size = heap_stack_size;
    this->num_of_pages = num_of_pages;
    this->page_size = page_size;
    this->frameCounter = 0;
    this->maxFrame = MEMORY_SIZE / page_size;
    this->frameTable = new int[maxFrame];
    assert(frameTable != NULL);
    for (i = 0; i < maxFrame; i++)
        frameTable[i] = -1;
    if (exe_file_name == nullptr) {
        fputs("file does not exist \n", stderr);
        exit(EXIT_FAILURE);
    }
    this->program_fd = open(exe_file_name, O_RDONLY);
    if (this->program_fd < 0) {
        fputs("open fd failed\n", stderr);
        exit(EXIT_FAILURE);
    }
    this->swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (this->swapfile_fd < 0) {
        fputs("open fd failed\n", stderr);
        close(this->program_fd);
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < MEMORY_SIZE; i++)
        main_memory[i] = 0;
    this->page_table = new page_descriptor[num_of_pages];
    for (i = 0; i < num_of_pages; i++) {
        this->page_table[i].V = 0;
        this->page_table[i].D = 0;
        this->page_table[i].P = 0;
        if (i >= this->text_size / this->page_size)
            this->page_table[i].P = 1;
        this->page_table[i].frame = -1;
    }
    for (i = 0; i < page_size; i++)
        buffer[i] = '0';
    sum = (data_size + text_size + bss_size + heap_stack_size) / page_size;
    for (i = 0, seek = 0; i < sum; i++, seek += page_size) {
        lseek(this->swapfile_fd, seek, SEEK_SET);
        write(this->swapfile_fd, buffer, page_size);
    }
    delete[] buffer;

}
/**************************************************************************************/
sim_mem::~sim_mem() {
    close(this->program_fd);
    close(this->swapfile_fd);
    delete[] this->page_table;
    delete[] this->frameTable;
}
/**************************************************************************************/
char sim_mem::load( int address){
        int page = address / page_size;
        int offset = address % page_size;
        int textData = (text_size + data_size) / page_size;
        if (page > num_of_pages || page < 0)
        {

            fputs("unable to load from this address\n",stderr);
            return '\0';
        }
        page_descriptor *p = &page_table[page];
        if ((page > textData && (p->D != 1 || p->V != 1)))
        {
            fputs("unable to load from bss/heap/stack address\n",stderr);
            return '\0';
        }
        if (p->V != 1)
            loadMemory(*p, page);
        return main_memory[(p->frame*page_size) + offset];
}

/**************************************************************************************/
void sim_mem::store(int address, char value){
    int page = address / page_size;
    int offset = address % page_size;

    if (page > num_of_pages || page < 0)
    {
        fputs("address not found\n",stderr);
        return;
    }
    page_descriptor *p = &page_table[page];
    if (p->P==0)
    {
        fputs("no permission to store in this address\n",stderr);
        return;
    }
    if (p->V != 1)
        loadMemory(*p, page);
    main_memory[(p->frame*page_size) + offset] = value;
}

/**************************************************************************************/
void sim_mem::print_memory() {
    int i;
    printf("\n Physical memory\n");
    for(i = 0; i < MEMORY_SIZE; i++) {
        printf("[%c]\n", main_memory[i]);
    }
}
/************************************************************************************/
void sim_mem::print_swap() {
    char *str = (char *)malloc(this->page_size * sizeof(char));
    int i;
    printf("\n Swap memory\n");
    lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
    while(read(swapfile_fd, str, this->page_size) == this->page_size) {
        for(i = 0; i <page_size; i++) {
            printf("%d - [%c]\t", i, str[i]);
        }
        printf("\n");
    }
}
/***************************************************************************************/
void sim_mem::print_page_table() {
    int i;
    printf("\n page table \n");
    printf("Valid\t Dirty\t Permission \t Frame\t Swap index\n");
    for (i = 0; i < num_of_pages; i++) {
        printf("[%d]\t[%d]\t[%d]\t[%d]\t[%d]\n",
               page_table[i].V, page_table[i].D, page_table[i].P,
               page_table[i].frame,
               page_table[i].
                       swap_index);
    }
}
/***************************************************************************************/
void sim_mem::loadMemory(page_descriptor &p, int page)
{
    if (frameTable[frameCounter] != -1)
    {
        page_descriptor *p2 = &page_table[frameTable[frameCounter]];
        if (p2->P != 0)
            swapInsert(*p2, frameTable[frameCounter]);
        else
        {
            p2->V = 0;
            p2->frame = -1;
        }
    }
    memInsert(p, page);
    frameTable[frameCounter] = page;
    frameCounter++;
    if (frameCounter == maxFrame)
        frameCounter = 0;
}
/**************************************************************************************/
void sim_mem::memInsert(page_descriptor &p, int seek)
{
    int mem, i;
    bool read_needed = true;
    char *buffer = new char[page_size];
    assert(buffer != NULL);
    for (i = 0; i < page_size; i++)
        buffer[i] = '0';
    if (p.D == 0)
    {
        if (text_size + data_size >= seek * page_size)
            mem = this->program_fd;
        else
            read_needed = false;
    }
    else
        mem = this->swapfile_fd;
    if (read_needed)
    {
        lseek(mem, seek * page_size, SEEK_SET);
        read(mem, buffer, page_size);
    }
    int max = frameCounter * page_size;
    for (int i = 0; i < page_size; i++)
        main_memory[i + max] = buffer[i];
    p.V = 1;
    p.frame = max/page_size;
    delete[] buffer;
}
/**************************************************************************************/
void sim_mem::swapInsert(page_descriptor &p, int seek)
{
    char *buffer = new char[page_size];
    assert(buffer != NULL);
    int max = frameCounter * page_size;
    for (int i = 0; i < page_size; i++)
        buffer[i] = main_memory[i + max];
    lseek(this->swapfile_fd, seek * page_size, SEEK_SET);
    write(this->swapfile_fd, buffer, page_size);
    p.V = 0;
    p.D = 1;
    p.frame = -1;
    delete[] buffer;
}

