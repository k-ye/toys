#define M61_DISABLE 1
#include "m61.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>

// global variables for stats
struct m61_statistics global_stats = {0, 0, 0, 0, 0, 0, (char *)ULONG_MAX, 0};
// the latest malloced ptr
char * current_blk_mptr = NULL;

#define DSIZE 8
// check if an address is DSIZE aligned
#define IS_ALIGNED(addr) (addr % DSIZE == 0)
// calculate the size that satisfies the alignment requirement
#define ALIGNED_SIZE(sz) (IS_ALIGNED(sz) ? sz : ((sz + DSIZE - 1) / DSIZE) * DSIZE)
// aligned block metadata size has to be aligned so that data address is correctly aligned
#define META_SIZE (ALIGNED_SIZE(sizeof(m61_blockmeta)))
// tail data size, do not need to be aligned, this will append immediately after the last byte of the data
#define TAIL_SIZE (sizeof(taildata_type))
// convert from the malloced ptr to the data ptr
#define DATA_PTR(mp) (mp + META_SIZE)
// convert from the data ptr to actual malloced ptr
#define MALLOC_PTR(dp) (dp - META_SIZE)
// return the metadata pointer of a given block, using malloced ptr (NOT data ptr)
#define BLK_META_PTR(mp) ((m61_blockmeta *)(mp))
// set the block size meta information
#define SET_BLOCK_SIZE(mp, sz) (BLK_META_PTR(mp)->block_size = sz)
// get the malloced block size stored in m61_blockmeta struct from malloced ptr (NOT data ptr)
#define GET_BLOCK_SIZE(mp) (BLK_META_PTR(mp)->block_size)
// flag the block as allocated
#define SET_BLOCK_ALLOC(mp) (BLK_META_PTR(mp)->alloced = 1)
// flag the block as freed
#define SET_BLOCK_FREE(mp) (BLK_META_PTR(mp)->alloced = 0)
// check if the block is being freed
#define IS_BLOCK_ALLOC(mp) (BLK_META_PTR(mp)->alloced == 1)
// set the file that created this mem block
#define SET_BLOCK_FILE(mp, file) (BLK_META_PTR(mp)->file = file)
// read the file that created this mem block
#define GET_BLOCK_FILE(mp) (BLK_META_PTR(mp)->file)
// set the line of code that created this mem block
#define SET_BLOCK_LINE(mp, line) (BLK_META_PTR(mp)->line = line)
// read the line of code that created this mem block
#define GET_BLOCK_LINE(mp) (BLK_META_PTR(mp)->line)
// get the pointer to the tail data
#define TAIL_DATA_PTR(mp) ((taildata_type *)(mp + META_SIZE + GET_BLOCK_SIZE(mp)))
// peek the current data stored in tail
#define OBSV_TAIL_DATA(mp) (*TAIL_DATA_PTR(mp))
// initiliaze a random tail data
#define INIT_TAIL_DATA(mp) {\
    srand(time(NULL)); \
    *TAIL_DATA_PTR(mp) = rand(); \
    BLK_META_PTR(mp)->tail_data = OBSV_TAIL_DATA(mp); }
// the tail data should be same as what we stored in metadata, 
// otherwise it's modified out of boundary
#define IS_TAIL_DATA_INTACT(mp) (BLK_META_PTR(mp)->tail_data == OBSV_TAIL_DATA(mp))
// get the pointer to the previous malloced memory block
#define GET_PREV_MPTR(mp) (BLK_META_PTR(mp)->prev_mptr)
// initialize the metadata stored in a block
#define INIT_BLOCK_META(mp, sz, file, line) {\
    m61_blockmeta meta = {0, 0, 0, current_blk_mptr, NULL, 0}; \
    memcpy(mp, &meta, META_SIZE); \
    SET_BLOCK_SIZE(ptr, sz); SET_BLOCK_ALLOC(ptr); \
    SET_BLOCK_FILE(ptr, file); SET_BLOCK_LINE(ptr, line); \
    INIT_TAIL_DATA(ptr); current_blk_mptr = mp; }

void* m61_malloc(size_t sz, const char* file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.
    if (sz == 0) return NULL;
    // adjusted size for 8 byte alignment
    size_t asize = META_SIZE + ALIGNED_SIZE(sz + TAIL_SIZE);
    // asize < sz means sz is too large that asize overflows
    void* ptr = (asize < sz) ? NULL : malloc(asize);
    if (ptr == NULL) {
        // malloc failed
        global_stats.nfail++;
        global_stats.fail_size += sz;
    } else {
        global_stats.ntotal++;
        global_stats.total_size += sz;
        global_stats.nactive++;
        global_stats.active_size += sz;
        // store the metadata at the beginning
        INIT_BLOCK_META(ptr, sz, file, line);

        if ((char *)ptr < global_stats.heap_min) global_stats.heap_min = ptr;
        if ((char *)(ptr+asize) > global_stats.heap_max) global_stats.heap_max = (ptr+asize);
        ptr = DATA_PTR(ptr);
    }
    return ptr;
}

void m61_free(void *ptr, const char *file, int line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    // Your code here.
    if (ptr) {
        ptr = MALLOC_PTR(ptr);
        if (ptr) {
            if ((char *)ptr > global_stats.heap_max || (char *)ptr < global_stats.heap_min) {
                printf("MEMORY BUG: %s:%d: invalid free of pointer 0x%x, not in heap\n", file, line, (unsigned int)ptr);
            } else {
                if (!IS_ALIGNED((unsigned int)ptr)) {
                    printf("MEMORY BUG: %s:%d: invalid free of pointer 0x%x, not allocated\n", file, line, (unsigned int)ptr);
                } else if (!IS_BLOCK_ALLOC(ptr)) {
                    printf("MEMORY BUG: %s:%d: invalid free of pointer 0x%x\n", file, line, (unsigned int)ptr);
                } else if (!IS_TAIL_DATA_INTACT(ptr)) {
                    printf("MEMORY BUG: %s:%d: detected wild write during free of pointer 0x%x\n", file, line, (unsigned int)ptr);
                } else {
                    global_stats.nactive--;
                    global_stats.active_size -= GET_BLOCK_SIZE(ptr);
                    SET_BLOCK_FREE(ptr);
                    free(ptr);
                }
            }
        }
    }
}

void* m61_realloc(void* ptr, size_t sz, const char* file, int line) {
    void* new_ptr = NULL;
    if (sz)
        new_ptr = m61_malloc(sz, file, line);
    if (ptr && new_ptr) {
        // Copy the data from `ptr` into `new_ptr`.
        // To do that, we must figure out the size of allocation `ptr`.
        // Your code here (to fix test012).
        size_t old_sz = GET_BLOCK_SIZE(MALLOC_PTR(ptr));
        if (old_sz < sz) memcpy(new_ptr, ptr, old_sz);
        else memcpy(new_ptr, ptr, sz);
    }
    m61_free(ptr, file, line);
    return new_ptr;
}

void* m61_calloc(size_t nmemb, size_t sz, const char* file, int line) {
    // Your code here (to fix test014).
    size_t total_sz = nmemb * sz;
    // check if total_sz overflows
    total_sz = (total_sz / sz == nmemb) ? total_sz : -1;
    void* ptr = m61_malloc(total_sz, file, line);
    if (ptr)
        memset(ptr, 0, total_sz);
    return ptr;
}

void m61_getstatistics(struct m61_statistics* stats) {
    // Your code here.
    *stats = global_stats;
}

void m61_printstatistics(void) {
    struct m61_statistics stats;
    m61_getstatistics(&stats);

    printf("malloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("malloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}

void m61_printleakreport(void) {
    // Your code here.
    char * mptr = current_blk_mptr;
    while (mptr) {
        if (IS_BLOCK_ALLOC(mptr))
            printf("LEAK CHECK: %s:%d: allocated object 0x%x with size %zu\n", 
                GET_BLOCK_FILE(mptr), GET_BLOCK_LINE(mptr), (unsigned int)DATA_PTR(mptr), GET_BLOCK_SIZE(mptr));
        mptr = GET_PREV_MPTR(mptr);
    }
}
