/*
 * mm.c
 * use segregated free list 
 *      allocated block                      free block
 * head  size + alloc_bit         head  size + alloc_bit
 *  bp->                          bp->  ptr point to next free block's bp (real addr = ptr + heap_listp)
 *                                      ptr point to prev free block's bp
 * tail  size + alloc_bit         tail  size + alloc_bit                                                    
 * comment that gives a high level description of your solution.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(p) (((size_t)(p) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define SIZE_PTR(p)  ((size_t*)(((char*)(p)) - SIZE_T_SIZE))

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<9)  /* Extend heap by this amount (bytes) */  

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            
#define PUT(p, val)  (*(unsigned int *)(p) = (val)) 

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                    

/* Given allocated block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given free block ptr bp, compute address of its prev ptr and succ ptr */
#define FPREVP(bp)      ((char *)(bp) + WSIZE)
#define FSUCCP(bp)      ((char *)(bp))

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
void add_list(void* bp,size_t size);
void delete_list(void *bp);

/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {
  /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(16*WSIZE)) == (void *)-1) 
        return -1;
    PUT(heap_listp, 0);                              /* Alignment padding */
    PUT(heap_listp + (1*WSIZE),0);                    /* save length 16 head */
    PUT(heap_listp + (2*WSIZE),0);                    /* save length 24 head */
    PUT(heap_listp + (3*WSIZE),0);                    /* save length 32 head */
    PUT(heap_listp + (4*WSIZE),0);                    /* save length 64 head */
    PUT(heap_listp + (5*WSIZE),0);                    /* save length 128 head */
    PUT(heap_listp + (6*WSIZE),0);                    /* save length 256 head */
    PUT(heap_listp + (7*WSIZE),0);                    /* save length 512 head */
    PUT(heap_listp + (8*WSIZE),0);                    /* save length 1024 head */
    PUT(heap_listp + (9*WSIZE),0);                    /* save length 2048 head */
    PUT(heap_listp + (10*WSIZE),0);                   /* save length 4096 head */
    PUT(heap_listp + (11*WSIZE),0);                   /* save length 8192 head */
    PUT(heap_listp + (12*WSIZE),0);                   /* save length > 8192 head */
    PUT(heap_listp + (13*WSIZE), PACK(DSIZE, 1));     /* Prologue header */ 
    PUT(heap_listp + (14*WSIZE), PACK(DSIZE, 1));     /* Prologue footer */ 
    PUT(heap_listp + (15*WSIZE), PACK(0, 1));    /* Epilogue header */               
   
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
        return -1;
    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      
    if (heap_listp == 0){
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    /* min free block size is 16 */
    if (size <= DSIZE)
        asize = 2*DSIZE;                                                                         
    else
        asize = DSIZE * ((size + 2*DSIZE - 1)/DSIZE);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  
        place(bp, asize);                  
        return bp;   
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);                 
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
        return NULL;                                  
    place(bp, asize);                                
    return bp;
}

/*
 * free
 */
void free (void *ptr) {  /* ptr is alloc ptr */
     if (!ptr)  return;
   
    size_t size = GET_SIZE(HDRP(ptr));
    if (heap_listp == 0){
        mm_init();
    }

    PUT(HDRP(ptr), PACK(size, 0));  
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        free(oldptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(oldptr == NULL) {
        return malloc(size);
    }

    newptr = malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = *SIZE_PTR(oldptr);
    if(size < oldsize) oldsize = size;
    memcpy(newptr, oldptr, oldsize);

    /* Free the old block. */
    free(oldptr);

    return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {
    size_t bytes = nmemb * size;
    void *newptr;

    newptr = malloc(bytes);
    memset(newptr, 0, bytes);

    return newptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t)ALIGN(p) == (size_t)p;
}

/*
 * mm_checkheap
 */
void mm_checkheap(int lineno) {
    if(lineno){
        unsigned long ptr;
        unsigned int delta_ptr;
        /* print the whole lists */
        for(int i = 1;i <= 12;i++){
            printf("index : %d\n",i);
            delta_ptr = *(unsigned int *)(heap_listp + i* WSIZE);
            ptr = (unsigned long)(delta_ptr + heap_listp);
            while(ptr && ptr != (unsigned long)heap_listp){
                size_t alloc = GET_ALLOC(HDRP(ptr));
                size_t size = GET_SIZE(HDRP(ptr));
                printf("alloc : %ld\n",alloc);
                printf("size : %ld\n",size);
                printf("bp : %ld\n",ptr);
                delta_ptr = *(unsigned int *)ptr;
                ptr = (unsigned long)(delta_ptr + heap_listp);
            }
            printf("\n");
        }
    }
}


/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
static void *coalesce(void *bp)               /* here bp alloc ptr */
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        bp = bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */        
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_list(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        delete_list(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        delete_list(NEXT_BLKP(bp)); 
        delete_list(PREV_BLKP(bp));        
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* add this free block to the lists */
    add_list(bp,size);
    return bp;
}
/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)       
{
    size_t csize = GET_SIZE(HDRP(bp));
    /* delete bp from the lists */
    delete_list(bp);  
    /* split */
    if ((csize - asize) >= (2*DSIZE)) { 
        PUT(HDRP(bp), PACK(asize, 1));                
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        add_list(bp,csize - asize);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else { 
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

int find_index(size_t size){
    if(size <= 16) return 1;
    if(size <= 32) return 2;
    if(size <= 40) return 3;
    if(size <= 64) return 4;
    if(size <= 128) return 5;
    if(size <= 256) return 6;
    if(size <= 512) return 7;
    if(size <= 1024) return 8;
    if(size <= 2048) return 9;
    if(size <= 4096) return 10;
    if(size <= 8192) return 11;
    return 12;
}
/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize)
{
    unsigned long ptr;
    unsigned int delta_ptr;
    int index = find_index(asize);
    while(index <= 12){
            /* compute the next ptr */
            delta_ptr = *(unsigned int *)(heap_listp + index * WSIZE);
            ptr = (unsigned long)(delta_ptr + heap_listp);
            while(ptr && ptr != (unsigned long)heap_listp){
                /*first fit */
                if(GET_SIZE(HDRP(ptr)) >= asize){
                    return (void *)ptr;
                }
                delta_ptr = *(unsigned int *)ptr;
                ptr = (unsigned long)(delta_ptr + heap_listp);
            }
        index++;
    }
    return NULL; /* No fit */
}

void add_list(void *bp,size_t size){          
    int index = find_index(size);
    /* delta ,bp is 4 bytes long */
    unsigned int delta = *(unsigned int *)(heap_listp + index*WSIZE);
    unsigned int bp_addr = (unsigned int)((unsigned long)bp - (unsigned long)heap_listp);   
        if(delta){                                                 /* head -> next != NULL */
            unsigned long head_next_prev = (unsigned long)FPREVP(delta + (unsigned long)heap_listp);
            *(unsigned int *)bp = delta;                           /* bp -> next = head -> next */
            *(unsigned int *)FPREVP(bp) = index*WSIZE;             /* bp -> prev = head */  
            *(unsigned int *)head_next_prev = bp_addr;             /* head -> next -> prev = bp */    
            *(unsigned int *)(heap_listp + index*WSIZE) = bp_addr; /* head -> next = bp */
        }
        else{                       
            *(unsigned int *)bp = 0;                                /*bp -> next = NULL */ 
            *(unsigned int *)FPREVP(bp) = index*WSIZE;              /*bp -> prev = head */          
            *(unsigned int *)(heap_listp + index*WSIZE) = bp_addr;  /* head -> next = bp */
        }
}

void delete_list(void *bp){
   
    unsigned int bp_prev = *(unsigned int *)FPREVP(bp);
    unsigned int bp_next = *(unsigned int *)bp; 
    if(!bp_prev)  return; 

    if(bp_next){                                                    /* bp -> next != NULL */         
        *(unsigned int *)(FPREVP(heap_listp + bp_next)) = bp_prev;  /* bp -> next -> prev = bp -> prev */
        *(unsigned int *)(FSUCCP(heap_listp + bp_prev)) = bp_next;  /* bp -> prev -> next = bp -> next */
        *(unsigned int *)bp = 0;                                    /* bp -> next = NULL */
        *(unsigned int *)FPREVP(bp) = 0;                            /* bp -> prev = NULL */
    }
    else{                      
        //PUT_PTR(FSUCCP(bp_prev),0);                  
        *(unsigned int *)(FSUCCP(heap_listp + bp_prev)) = 0; /* bp -> prev -> next = NULL */
        *(unsigned int *)FPREVP(bp) = 0;                     /* bp -> prev = NULL */
    }
}
