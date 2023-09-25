#define _DEFAULT_SOURCE
#define _BSD_SOURCE 
#include <malloc.h> 
#include <stdio.h>  // Any other headers we need here
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <pthread.h>
#include <debug.h> // definition of debug_printf

#include "block.h" // definition of block_t

//self-explanatory, block size
#define BLOCK_SIZE sizeof(block_t)
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

//the head of the linked list
block_t *head = NULL;

pthread_mutex_t mutex;

// SEE BELOW: helper for mmap utilization
block_t *overPage(size_t s);


//I am aware that there are two repetitive sections,
//however abstracting it out caused serious problems
//(i.e., program got killed by memory management)
void *mymalloc(size_t s) { 

    block_t *p;
    size_t total = BLOCK_SIZE + s;
    //head has not been made yet, make it!
    
    //REASON: once we come to the conclusion that something
    //is empty or we are searching the list, we cannot have it
    //modified
    pthread_mutex_lock(&mutex);
    if (head == NULL) {
        if (total >= PAGE_SIZE) {
            head = overPage(s);
        } else {
            head = sbrk(total);
            fillB(head, s);
        }
        p = head;
    } else {
        block_t *temp = head;
        block_t *last;
        while (temp != NULL) {
            //if we already have a free space, take it
            if (temp->free == 1 && temp->size >= s) {
                p = temp;
                p->free = 0;
                debug_printf("Malloc %zu bytes\n", s); 
                return p + 1;
            }
            last = temp;
            temp = temp->next;
        }

        if (total >= PAGE_SIZE) {
            last->next = overPage(s);
        } else {
            last->next = sbrk(total);
            fillB(last->next, s);
        }
        p = last->next;
    }
    //at this point, we no longer need the list and have updated
    //it for everyone else, unlock.
    pthread_mutex_unlock(&mutex);

    debug_printf("Malloc %zu bytes\n", s); 

    return p + 1;
}

void *mycalloc(size_t nmemb, size_t s) { 
    if (nmemb == 0 || s == 0) {
        return NULL;
    }
    size_t size = nmemb * s;
    void *p;

    if (s != size / nmemb) {
        return NULL;
    }

    //mymalloc should take care of the mutex locks
    p = mymalloc(size);

    if (p == NULL) {
        return NULL;
    }
    memset(p, 0, size);

    debug_printf("Calloc %zu bytes\n", s);

  return p;
}

//TODO: mutex implementation
void myfree(void *ptr) {

    block_t *block = ptr - BLOCK_SIZE;
    pthread_mutex_lock(&mutex);
    block->free = 1;    
    pthread_mutex_unlock(&mutex);
    
    debug_printf("Freed %zu bytes\n", block->size);
}

block_t *overPage(size_t s) {
    block_t *p;
    //the total amount of needed memory
    size_t total = BLOCK_SIZE + s;
    size_t extra = total % PAGE_SIZE;

    if (extra > BLOCK_SIZE) {
       //create the memory space
       p = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, (off_t)0);
       verify(p);
       p->size = total;
       p->free = 0;

       //setting up the extra space
       char *temp = (char *) p;
       temp += total;
       block_t *n = (block_t *) temp;
       n->free = 1;
       n->size = extra - BLOCK_SIZE;
       n->next = NULL;

       p->next = n;
        
    } else {
        p = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, (off_t)0);
        verify(p);
        p->size = s;
        p->free = 0;
        p->next = NULL;
    }

    return p;
}


