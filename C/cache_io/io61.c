#include "io61.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

// io61.c
//    YOUR CODE HERE!
#define BUFSIZE 32768
#define R_BUFSIZE BUFSIZE
#define W_BUFSIZE BUFSIZE
#define MIN(a,b) ((a) < (b) ? a : b)

// io61_file
//    Data structure for io61 file wrappers. Add your own stuff.

struct io61_file {
    int fd;                                 // file descriptor
    unsigned char g_rbuffer[R_BUFSIZE];     // global single-slot reading buffer
    ssize_t g_rbuf_next;                    // next index in read buffer
    ssize_t g_rbuf_real_size;               // the actual read buffer size, could be leq than BUFSIZE near EOF
    unsigned char g_wbuffer[W_BUFSIZE];     // global single-slot writing buffer
    ssize_t g_wbuf_cur_size;                // current write buffer size
};


// io61_fdopen(fd, mode)
//    Return a new io61_file that reads from and/or writes to the given
//    file descriptor `fd`. `mode` is either O_RDONLY for a read-only file
//    or O_WRONLY for a write-only file. You need not support read/write
//    files.

io61_file* io61_fdopen(int fd, int mode) {
    assert(fd >= 0);
    io61_file* f = (io61_file*) malloc(sizeof(io61_file));
    f->fd = fd;
    f->g_rbuf_next = 0;
    f->g_rbuf_real_size = -1;
    f->g_wbuf_cur_size = 0;
    (void) mode;
    return f;
}


// io61_close(f)
//    Close the io61_file `f` and release all its resources, including
//    any buffers.

int io61_close(io61_file* f) {
    io61_flush(f);
    int r = close(f->fd);
    free(f);
    return r;
}

/* read n chars */
size_t io61_readn(io61_file* f, char* buf, size_t sz) {
    /* ver. 1, no cache
    size_t result = read(f->fd, buf, sz);
    if (result == sz) return result;
    else return EOF;*/

    /* ver. 2, single slot reading cache */
    ssize_t has_read = 0;
    // this checks the initial condition where @g_rbuf_real_size is equal to -1
    if (f->g_rbuf_real_size >= 0) {
        // read all that nees to be read from cached buffer
        size_t buf_rsz = MIN(sz, f->g_rbuf_real_size - f->g_rbuf_next);
        memcpy(buf, f->g_rbuffer + f->g_rbuf_next, buf_rsz);
        has_read += buf_rsz;
        f->g_rbuf_next += has_read;
    }

    if (has_read < sz) {
        // read all the rest that needs to be read via IO
        size_t need_io_rsz = sz - has_read;
        ssize_t io_rres = read(f->fd, buf + has_read, need_io_rsz);
        if (io_rres < 0) return EOF;
        has_read += io_rres;
        // if this branch can be reached, that means our buffer has been read completely,
        // we need to fill in more into the cache buffer
        io_rres = read(f->fd, f->g_rbuffer, BUFSIZE);
        if (io_rres < 0) return EOF;
        else {
            f->g_rbuf_next = 0;
            f->g_rbuf_real_size = io_rres;
        }
    }
    return has_read;
}

// io61_readc(f)
//    Read a single (unsigned) character from `f` and return it. Returns EOF
//    (which is -1) on error or end-of-file.

int io61_readc(io61_file* f) {
    unsigned char buf[1];
    if (io61_readn(f, buf, 1) == 1) return buf[0];
    else return EOF;
}


// io61_read(f, buf, sz)
//    Read up to `sz` characters from `f` into `buf`. Returns the number of
//    characters read on success; normally this is `sz`. Returns a short
//    count if the file ended before `sz` characters could be read. Returns
//    -1 an error occurred before any characters were read.

ssize_t io61_read(io61_file* f, char* buf, size_t sz) {
    size_t nread = io61_readn(f, buf, sz);
    if (nread == sz || sz == 0 || io61_eof(f))
        return nread;
    else
        return -1;
}

ssize_t io61_flush_wbuf(io61_file* f) {
    if (f->g_wbuf_cur_size > 0) {
        ssize_t wres = write(f->fd, f->g_wbuffer, f->g_wbuf_cur_size);
        f->g_wbuf_cur_size = 0;
        return wres;
    }
    return 0;
}
/* write n chars */
ssize_t io61_writen(io61_file* f, const unsigned char* buf, size_t sz) {
    /* ver. 1, no cache 
    size_t result = write(f->fd, buf, sz);
    if (result == sz) return result;
    else return -1;*/

    /* ver. 2, single slot writing cache */
    ssize_t has_written = 0;
    size_t buf_wsz = MIN(sz, W_BUFSIZE - f->g_wbuf_cur_size);
    memcpy(f->g_wbuffer + f->g_wbuf_cur_size, buf, buf_wsz);
    has_written += buf_wsz;
    f->g_wbuf_cur_size += has_written;

    if (f->g_wbuf_cur_size == W_BUFSIZE) {
        ssize_t io_wres = io61_flush_wbuf(f);
        if (io_wres < 0) return io_wres;
    }
    if (has_written < sz) {
        size_t need_io_wsz = sz - has_written;
        ssize_t io_wres = write(f->fd, buf + has_written, need_io_wsz);
        if (io_wres < 0) return io_wres;
        has_written += io_wres;
    }
    return has_written;
}


// io61_writec(f)
//    Write a single character `ch` to `f`. Returns 0 on success or
//    -1 on error.

int io61_writec(io61_file* f, int ch) {
    unsigned char buf[1];
    buf[0] = ch;
    if (io61_writen(f, buf, 1) == 1) return 1;
    else return -1;
}


// io61_write(f, buf, sz)
//    Write `sz` characters from `buf` to `f`. Returns the number of
//    characters written on success; normally this is `sz`. Returns -1 if
//    an error occurred before any characters were written.

ssize_t io61_write(io61_file* f, const char* buf, size_t sz) {
    ssize_t nwritten = io61_writen(f, buf, sz);
    if (nwritten == sz || sz == 0)
        return nwritten;
    else
        return -1;
}


// io61_flush(f)
//    Forces a write of all buffered data written to `f`.
//    If `f` was opened read-only, io61_flush(f) may either drop all
//    data buffered for reading, or do nothing.

int io61_flush(io61_file* f) {
    //(void) f;
    io61_flush_wbuf(f);
    return 0;
}


// io61_seek(f, pos)
//    Change the file pointer for file `f` to `pos` bytes into the file.
//    Returns 0 on success and -1 on failure.

int io61_seek(io61_file* f, off_t pos) {
    off_t r = lseek(f->fd, (off_t) pos, SEEK_SET);
    if (r == (off_t) pos)
        return 0;
    else
        return -1;
}


// You shouldn't need to change these functions.

// io61_open_check(filename, mode)
//    Open the file corresponding to `filename` and return its io61_file.
//    If `filename == NULL`, returns either the standard input or the
//    standard output, depending on `mode`. Exits with an error message if
//    `filename != NULL` and the named file cannot be opened.

io61_file* io61_open_check(const char* filename, int mode) {
    int fd;
    if (filename)
        fd = open(filename, mode, 0666);
    else if ((mode & O_ACCMODE) == O_RDONLY)
        fd = STDIN_FILENO;
    else
        fd = STDOUT_FILENO;
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    return io61_fdopen(fd, mode & O_ACCMODE);
}


// io61_filesize(f)
//    Return the size of `f` in bytes. Returns -1 if `f` does not have a
//    well-defined size (for instance, if it is a pipe).

off_t io61_filesize(io61_file* f) {
    struct stat s;
    int r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode))
        return s.st_size;
    else
        return -1;
}


// io61_eof(f)
//    Test if readable file `f` is at end-of-file. Should only be called
//    immediately after a `read` call that returned 0 or -1.

int io61_eof(io61_file* f) {
    char x;
    ssize_t nread = read(f->fd, &x, 1);
    if (nread == 1) {
        fprintf(stderr, "Error: io61_eof called improperly\n\
  (Only call immediately after a read() that returned 0 or -1.)\n");
        abort();
    }
    return nread == 0;
}
