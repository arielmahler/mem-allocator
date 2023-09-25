#ifndef _BLOCK_H
#define _BLOCK_H

#include <stddef.h>

typedef struct block {
  size_t size;        // How many bytes beyond this block have been allocated in the heap
  struct block *next; // Where is the next block in your linked list
  int free;           // Is this memory free, i.e., available to give away?
  int debug;         
                    
} block_t;

void fillB(block_t *b, size_t s) {
    b->size = s;
    b->free = 0;
    b->next = NULL;
}

void verify(block_t *b) {
    if(b == (void *) -1) {
        perror("Memory Alloc Failed.");
    }
}

#endif
