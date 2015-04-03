#ifndef M61_H
#define M61_H 1
#include <stdlib.h>
#include <time.h>

void* m61_malloc(size_t sz, const char* file, int line);
void m61_free(void* ptr, const char* file, int line);
void* m61_realloc(void* ptr, size_t sz, const char* file, int line);
void* m61_calloc(size_t nmemb, size_t sz, const char* file, int line);

struct m61_statistics {
    unsigned long long nactive;         // # active allocations
    unsigned long long active_size;     // # bytes in active allocations
    unsigned long long ntotal;          // # total allocations
    unsigned long long total_size;      // # bytes in total allocations
    unsigned long long nfail;           // # failed allocation attempts
    unsigned long long fail_size;       // # bytes in failed alloc attempts
    char* heap_min;                     // smallest allocated addr
    char* heap_max;                     // largest allocated addr
};

typedef unsigned long taildata_type;

typedef struct m61_block_meta {
	size_t block_size; // size requested, not adjusted aligned size
	char alloced; // flag of alloc/free status
    taildata_type tail_data; // the data beyond the tail as an intact checker, for boundary write error detection
    char * prev_mptr; // malloced ptr pointing to the previous block
    const char* file;
    int line;
}m61_blockmeta;

/* Macros Definition */
// double word size, for alignment
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
#define SET_BLOCK_ALLOC(mp) ((BLK_META_PTR(mp)->alloced) = 1)
// flag the block as freed
#define SET_BLOCK_FREE(mp) ((BLK_META_PTR(mp)->alloced) = 0)
// check if the block is being freed
#define IS_BLOCK_ALLOC(mp) ((BLK_META_PTR(mp)->alloced) == 1)
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

void m61_getstatistics(struct m61_statistics* stats);
void m61_printstatistics(void);
void m61_printleakreport(void);

#if !M61_DISABLE
#define malloc(sz)              m61_malloc((sz), __FILE__, __LINE__)
#define free(ptr)               m61_free((ptr), __FILE__, __LINE__)
#define realloc(ptr, sz)        m61_realloc((ptr), (sz), __FILE__, __LINE__)
#define calloc(nmemb, sz)       m61_calloc((nmemb), (sz), __FILE__, __LINE__)
#endif

#endif
