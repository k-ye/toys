#define M61_DISABLE 1
#include "m61.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <stdbool.h>

// global variables for stats
struct m61_statistics global_stats = {0, 0, 0, 0, 0, 0, (char *)ULONG_MAX, 0};
// list node for storing active blocks
typedef struct m61_active_block {
    char * mptr;
    struct m61_active_block * prev;
    struct m61_active_block * next;
}m61_active_block;
// tail of the active block list
m61_active_block * max_actv_block = NULL;
// set the previous block ptr of the given block
#define SET_PREV_BLK(blk, p) (blk->prev = p)
// get the previous block ptr of the given block
#define GET_PREV_BLK(blk) (blk->prev)
// set the next block ptr of the given block
#define SET_NEXT_BLK(blk, n) (blk->next = n)
// get the next block ptr of the given block
#define GET_NEXT_BLK(blk) (blk->next)
// allocate a new node for active block
m61_active_block * new_active_block(char * mptr) {
    m61_active_block * new_actv_blk = (m61_active_block *)malloc(ALIGNED_SIZE(sizeof(m61_active_block)));
    new_actv_blk->mptr = mptr;
    SET_PREV_BLK(new_actv_blk, NULL); SET_NEXT_BLK(new_actv_blk, NULL);
    return new_actv_blk;
}
// link a new mem block into active block list
bool link_active_block(char * mptr) {
    if (!mptr) return false;
    m61_active_block * new_actv_blk = new_active_block(mptr);

    if (max_actv_block) {
        //find the first active mptr that is <= mptr
        m61_active_block * before_blk = max_actv_block;
        while (before_blk && (before_blk->mptr > mptr))
            before_blk = GET_PREV_BLK(before_blk);
        // set the double linked list
        SET_PREV_BLK(new_actv_blk, before_blk);
        if (before_blk) SET_NEXT_BLK(new_actv_blk, GET_NEXT_BLK(before_blk));
        if (GET_NEXT_BLK(new_actv_blk)) SET_PREV_BLK(GET_NEXT_BLK(new_actv_blk), new_actv_blk);
        if (before_blk) SET_NEXT_BLK(before_blk, new_actv_blk);
    }
    // update the max active mptr
    if (!max_actv_block || max_actv_block->mptr < mptr) max_actv_block = new_actv_blk;
    return true;
}
// unlink a mem block from the active block list
bool unlink_active_block(char * mptr) {
    if (!mptr) return false;
    // check to ensure mptr is indeed an active block
    m61_active_block * check_blk = max_actv_block;
    while (check_blk) {
        if (check_blk->mptr == mptr) {
            // do the actual unlink job
            m61_active_block * prev_blk = GET_PREV_BLK(check_blk);
            m61_active_block * next_blk = GET_NEXT_BLK(check_blk);
            if (prev_blk) SET_NEXT_BLK(prev_blk, next_blk);
            if (next_blk) SET_PREV_BLK(next_blk, prev_blk);
            if (max_actv_block == check_blk) max_actv_block = prev_blk;
            SET_PREV_BLK(check_blk, NULL); SET_NEXT_BLK(check_blk, NULL);
            free(check_blk);
            return true;
        }
        check_blk = GET_PREV_BLK(check_blk);
    }
    return false;
}
// check if a mem block is indeed active
bool is_block_active(char * mptr) {
    if (!mptr) return false;

    m61_active_block * actv_blk = max_actv_block;
    while (actv_blk) {
        if (actv_blk->mptr == mptr) return true;
        actv_blk = GET_PREV_BLK(actv_blk);
    }
    return false;
}

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
        link_active_block(ptr);

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
            } else if (!IS_TAIL_DATA_INTACT(ptr)) {
                printf("MEMORY BUG: %s:%d: detected wild write during free of pointer 0x%x\n", file, line, (unsigned int)ptr);
                abort();
            } else {
                m61_active_block * check_blk = max_actv_block;
                while (check_blk && check_blk->mptr > (char *)ptr)
                    check_blk = GET_PREV_BLK(check_blk);

                size_t ptr_diff = check_blk ? (size_t)((char *)ptr - check_blk->mptr) : 0;
                size_t actual_sz = check_blk ? GET_BLOCK_SIZE(check_blk->mptr) : 0;

                if (check_blk && (check_blk->mptr < (char *)ptr) && (ptr_diff < actual_sz)) {
                    int actual_line = GET_BLOCK_LINE(check_blk->mptr);
                    printf("MEMORY BUG: %s:%d: invalid free of pointer 0x%x, not allocated\n", file, line, (unsigned int)ptr);
                    printf("  %s:%d: 0x%x is %zu bytes inside a %zu byte region allocated here\n", 
                        file, actual_line, (unsigned int)ptr, ptr_diff, actual_sz);
                    abort();
                } else if (!is_block_active(ptr)) {
                    printf("MEMORY BUG: %s:%d: invalid free of pointer 0x%x\n", file, line, (unsigned int)ptr);
                    abort();
                } else {
                    global_stats.nactive--;
                    global_stats.active_size -= GET_BLOCK_SIZE(ptr);
                    unlink_active_block(ptr);
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
    m61_active_block * actv_blk = max_actv_block;
    while (actv_blk) {
        char * mptr = actv_blk->mptr;
        printf("LEAK CHECK: %s:%d: allocated object 0x%x with size %zu\n", 
                GET_BLOCK_FILE(mptr), GET_BLOCK_LINE(mptr), (unsigned int)DATA_PTR(mptr), GET_BLOCK_SIZE(mptr));
        actv_blk = GET_PREV_BLK(actv_blk);
    }
}