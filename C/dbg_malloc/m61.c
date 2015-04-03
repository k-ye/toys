#define M61_DISABLE 1
#include "m61.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>

// global variables for stats
struct m61_statistics global_stats = {0, 0, 0, 0, 0, 0, (char *)ULONG_MAX, 0};
// the latest malloced ptr
char * current_blk_mptr = NULL;

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
