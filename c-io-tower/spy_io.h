#ifndef SPY_IO_H
#define SPY_IO_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

/* ── String type (GC-managed) ─────────────────────────────────────── */

typedef struct {
    size_t length;
    int32_t hash;
    const char utf8[];
} spy_Str;

void   *gc_malloc(size_t size);
spy_Str *spy_str_new(const char *buf, size_t len);

/* ── Layer 0: Raw IO (syscalls) ───────────────────────────────────── */

typedef struct spy_RawIO spy_RawIO;

spy_RawIO *spy_raw_open(const char *path, int flags, mode_t mode);
spy_RawIO *spy_raw_from_fd(int fd, int own_fd);
ssize_t    spy_raw_read(spy_RawIO *r, void *buf, size_t n);
ssize_t    spy_raw_write(spy_RawIO *r, const void *buf, size_t n);
off_t      spy_raw_seek(spy_RawIO *r, off_t off, int whence);
int        spy_raw_close(spy_RawIO *r);
int        spy_raw_fd(spy_RawIO *r);

/* ── Layer 1: Buffered IO ─────────────────────────────────────────── */

typedef struct spy_BufferedIO spy_BufferedIO;

#define SPY_BUF_DEFAULT 8192

typedef enum {
    SPY_BUF_READ  = 1,
    SPY_BUF_WRITE = 2,
    SPY_BUF_RW    = 3,
} spy_BufMode;

spy_BufferedIO *spy_buffered_new(spy_RawIO *raw, spy_BufMode mode, size_t bufsize);

/* Standard read: copies into caller's buffer. */
ssize_t spy_buffered_read(spy_BufferedIO *b, void *buf, size_t n);

/* Zero-copy peek: returns pointer into internal buffer + available bytes.
   Valid until next read/peek/consume call.  Returns 0 at EOF, -1 on error. */
ssize_t spy_buffered_peek(spy_BufferedIO *b, const char **out);

/* Discard `n` bytes from the front of the read buffer. */
void    spy_buffered_consume(spy_BufferedIO *b, size_t n);

ssize_t spy_buffered_write(spy_BufferedIO *b, const void *buf, size_t n);
int     spy_buffered_flush(spy_BufferedIO *b);
int     spy_buffered_close(spy_BufferedIO *b);

/* ── Layer 2: Text IO ─────────────────────────────────────────────── */

typedef struct spy_TextIO spy_TextIO;

typedef enum {
    SPY_NL_UNIVERSAL,   /* \r\n | \r | \n  →  \n  on read;  \n → platform on write */
    SPY_NL_NONE,        /* no translation */
    SPY_NL_LF,          /* \n   (Unix) */
    SPY_NL_CRLF,        /* \r\n (Windows) */
    SPY_NL_CR,          /* \r   (old Mac) */
} spy_NewlineMode;

spy_TextIO *spy_text_new(spy_BufferedIO *bio, spy_NewlineMode nl);

/* Read up to `nchars` Unicode codepoints.  Pass (size_t)-1 to read all. */
spy_Str   *spy_text_read(spy_TextIO *t, size_t nchars);

/* Read one line (including the trailing \n, if any). NULL at EOF. */
spy_Str   *spy_text_readline(spy_TextIO *t);

/* Write a string.  Returns bytes written to the buffered layer, or -1. */
ssize_t    spy_text_write(spy_TextIO *t, const spy_Str *s);

int        spy_text_flush(spy_TextIO *t);
int        spy_text_close(spy_TextIO *t);

/* ── Convenience ──────────────────────────────────────────────────── */

/* Mode string like Python: "r", "w", "rb", "wb", "r+", etc.
   Text mode uses universal newlines by default. */
spy_TextIO     *spy_open_text(const char *path, const char *mode);
spy_BufferedIO *spy_open_binary(const char *path, const char *mode);

#endif /* SPY_IO_H */
