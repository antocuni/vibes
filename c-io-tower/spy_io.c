#define _GNU_SOURCE
#include "spy_io.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

/* ══════════════════════════════════════════════════════════════════════
   GC stub + spy_Str helpers
   ══════════════════════════════════════════════════════════════════════ */

void *gc_malloc(size_t size)
{
    void *p = malloc(size);
    if (!p) abort();
    return p;
}

spy_Str *spy_str_new(const char *buf, size_t len)
{
    spy_Str *s = gc_malloc(sizeof(spy_Str) + len + 1);
    *(size_t *)&s->length = len;
    *(int32_t *)&s->hash = -1;          /* not computed */
    memcpy((char *)s->utf8, buf, len);
    ((char *)s->utf8)[len] = '\0';
    return s;
}

/* ══════════════════════════════════════════════════════════════════════
   Layer 0 – Raw IO  (thin wrapper around libc read/write/open/close)
   ══════════════════════════════════════════════════════════════════════ */

struct spy_RawIO {
    int fd;
    int own_fd;     /* close fd on spy_raw_close? */
};

spy_RawIO *spy_raw_open(const char *path, int flags, mode_t mode)
{
    int fd = open(path, flags, mode);
    if (fd < 0) return NULL;
    spy_RawIO *r = malloc(sizeof(*r));
    r->fd = fd;
    r->own_fd = 1;
    return r;
}

spy_RawIO *spy_raw_from_fd(int fd, int own_fd)
{
    spy_RawIO *r = malloc(sizeof(*r));
    r->fd = fd;
    r->own_fd = own_fd;
    return r;
}

ssize_t spy_raw_read(spy_RawIO *r, void *buf, size_t n)
{
    return read(r->fd, buf, n);
}

ssize_t spy_raw_write(spy_RawIO *r, const void *buf, size_t n)
{
    const char *p = buf;
    size_t rem = n;
    while (rem > 0) {
        ssize_t w = write(r->fd, p, rem);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += w;
        rem -= (size_t)w;
    }
    return (ssize_t)n;
}

off_t spy_raw_seek(spy_RawIO *r, off_t off, int whence)
{
    return lseek(r->fd, off, whence);
}

int spy_raw_close(spy_RawIO *r)
{
    int ret = 0;
    if (r->own_fd)
        ret = close(r->fd);
    free(r);
    return ret;
}

int spy_raw_fd(spy_RawIO *r)
{
    return r->fd;
}

/* ══════════════════════════════════════════════════════════════════════
   Layer 1 – Buffered IO
   ══════════════════════════════════════════════════════════════════════ */

struct spy_BufferedIO {
    spy_RawIO  *raw;
    spy_BufMode mode;

    /* read buffer */
    char   *rbuf;
    size_t  rbuf_size;
    size_t  rbuf_start;     /* first unconsumed byte */
    size_t  rbuf_end;       /* one past last valid byte */
    int     eof;

    /* write buffer */
    char   *wbuf;
    size_t  wbuf_size;
    size_t  wbuf_len;
};

spy_BufferedIO *spy_buffered_new(spy_RawIO *raw, spy_BufMode mode, size_t bufsize)
{
    if (bufsize == 0) bufsize = SPY_BUF_DEFAULT;

    spy_BufferedIO *b = calloc(1, sizeof(*b));
    b->raw  = raw;
    b->mode = mode;

    if (mode & SPY_BUF_READ) {
        b->rbuf = malloc(bufsize);
        b->rbuf_size = bufsize;
    }
    if (mode & SPY_BUF_WRITE) {
        b->wbuf = malloc(bufsize);
        b->wbuf_size = bufsize;
    }
    return b;
}

/* Fill the read buffer from raw IO.  Returns bytes available, 0 on EOF, -1 on error. */
static ssize_t buf_fill(spy_BufferedIO *b)
{
    if (b->eof) return 0;

    /* Compact: move unconsumed data to front */
    size_t avail = b->rbuf_end - b->rbuf_start;
    if (avail > 0 && b->rbuf_start > 0) {
        memmove(b->rbuf, b->rbuf + b->rbuf_start, avail);
    }
    b->rbuf_start = 0;
    b->rbuf_end = avail;

    ssize_t n = spy_raw_read(b->raw, b->rbuf + b->rbuf_end,
                             b->rbuf_size - b->rbuf_end);
    if (n < 0) return -1;
    if (n == 0) { b->eof = 1; return (ssize_t)avail; }
    b->rbuf_end += (size_t)n;
    return (ssize_t)(b->rbuf_end - b->rbuf_start);
}

ssize_t spy_buffered_peek(spy_BufferedIO *b, const char **out)
{
    size_t avail = b->rbuf_end - b->rbuf_start;
    if (avail == 0) {
        ssize_t n = buf_fill(b);
        if (n <= 0) return n;
        avail = b->rbuf_end - b->rbuf_start;
    }
    *out = b->rbuf + b->rbuf_start;
    return (ssize_t)avail;
}

void spy_buffered_consume(spy_BufferedIO *b, size_t n)
{
    b->rbuf_start += n;
    if (b->rbuf_start > b->rbuf_end)
        b->rbuf_start = b->rbuf_end;
}

ssize_t spy_buffered_read(spy_BufferedIO *b, void *buf, size_t n)
{
    char *dst = buf;
    size_t total = 0;

    while (total < n) {
        const char *src;
        ssize_t avail = spy_buffered_peek(b, &src);
        if (avail < 0) return -1;
        if (avail == 0) break;

        size_t chunk = n - total;
        if (chunk > (size_t)avail) chunk = (size_t)avail;
        memcpy(dst + total, src, chunk);
        spy_buffered_consume(b, chunk);
        total += chunk;
    }
    return (ssize_t)total;
}

ssize_t spy_buffered_write(spy_BufferedIO *b, const void *buf, size_t n)
{
    const char *src = buf;
    size_t total = 0;

    while (total < n) {
        size_t space = b->wbuf_size - b->wbuf_len;
        if (space == 0) {
            if (spy_buffered_flush(b) < 0) return -1;
            space = b->wbuf_size;
        }
        size_t chunk = n - total;
        if (chunk > space) chunk = space;
        memcpy(b->wbuf + b->wbuf_len, src + total, chunk);
        b->wbuf_len += chunk;
        total += chunk;
    }
    return (ssize_t)total;
}

int spy_buffered_flush(spy_BufferedIO *b)
{
    if (b->wbuf_len == 0) return 0;
    ssize_t w = spy_raw_write(b->raw, b->wbuf, b->wbuf_len);
    if (w < 0) return -1;
    b->wbuf_len = 0;
    return 0;
}

int spy_buffered_close(spy_BufferedIO *b)
{
    int ret = 0;
    if (b->mode & SPY_BUF_WRITE)
        ret = spy_buffered_flush(b);
    int r2 = spy_raw_close(b->raw);
    if (r2 < 0) ret = r2;
    free(b->rbuf);
    free(b->wbuf);
    free(b);
    return ret;
}

/* ══════════════════════════════════════════════════════════════════════
   Layer 2 – Text IO
   ══════════════════════════════════════════════════════════════════════ */

struct spy_TextIO {
    spy_BufferedIO *bio;
    spy_NewlineMode nl;

    /* Accumulation buffer for readline (used when line spans buffer fills) */
    char   *accum;
    size_t  accum_len;
    size_t  accum_cap;

    /* Universal newlines: was the previous byte a bare \r? */
    int     pending_cr;
};

spy_TextIO *spy_text_new(spy_BufferedIO *bio, spy_NewlineMode nl)
{
    spy_TextIO *t = calloc(1, sizeof(*t));
    t->bio = bio;
    t->nl  = nl;
    return t;
}

/* ── Accumulation buffer helpers ──────────────────────────────────── */

static void accum_ensure(spy_TextIO *t, size_t extra)
{
    size_t need = t->accum_len + extra;
    if (need <= t->accum_cap) return;
    size_t cap = t->accum_cap ? t->accum_cap : 256;
    while (cap < need) cap *= 2;
    t->accum = realloc(t->accum, cap);
    t->accum_cap = cap;
}

static void accum_append(spy_TextIO *t, const char *data, size_t len)
{
    accum_ensure(t, len);
    memcpy(t->accum + t->accum_len, data, len);
    t->accum_len += len;
}

static void accum_reset(spy_TextIO *t)
{
    t->accum_len = 0;
}

/* ── Newline scanning ─────────────────────────────────────────────── */

/*
 * Scan `buf[0..len)` for a line ending according to newline mode.
 * Returns the offset of the first byte PAST the line ending, or 0 if no
 * complete line ending was found.  Sets *nl_start to the offset where the
 * newline sequence starts (so the "line content" is buf[0..*nl_start)).
 *
 * For UNIVERSAL mode, if the buffer ends with \r and we can't tell whether
 * \n follows, returns 0 (caller must refill).  Set `at_eof` to force
 * acceptance of a trailing \r.
 */
static size_t scan_newline(const char *buf, size_t len,
                           spy_NewlineMode nl, int at_eof,
                           size_t *nl_start)
{
    for (size_t i = 0; i < len; i++) {
        switch (nl) {
        case SPY_NL_LF:
        case SPY_NL_NONE:
            if (buf[i] == '\n') {
                *nl_start = i;
                return i + 1;
            }
            break;

        case SPY_NL_CR:
            if (buf[i] == '\r') {
                *nl_start = i;
                return i + 1;
            }
            break;

        case SPY_NL_CRLF:
            if (buf[i] == '\r') {
                if (i + 1 < len) {
                    if (buf[i + 1] == '\n') {
                        *nl_start = i;
                        return i + 2;
                    }
                    /* \r not followed by \n – not a line ending in CRLF mode */
                } else if (!at_eof) {
                    return 0;   /* need more data */
                }
            }
            break;

        case SPY_NL_UNIVERSAL:
            if (buf[i] == '\n') {
                *nl_start = i;
                return i + 1;
            }
            if (buf[i] == '\r') {
                if (i + 1 < len) {
                    *nl_start = i;
                    /* \r\n or bare \r – both are a line ending */
                    return (buf[i + 1] == '\n') ? i + 2 : i + 1;
                }
                if (at_eof) {
                    *nl_start = i;
                    return i + 1;
                }
                return 0;  /* need more data to distinguish \r vs \r\n */
            }
            break;
        }
    }
    return 0;
}

/* ── readline ─────────────────────────────────────────────────────── */

spy_Str *spy_text_readline(spy_TextIO *t)
{
    accum_reset(t);

    for (;;) {
        const char *chunk;
        ssize_t avail = spy_buffered_peek(t->bio, &chunk);
        if (avail < 0) return NULL;

        if (avail == 0) {
            /* True EOF – return whatever we have accumulated */
            if (t->accum_len == 0) return NULL;
            return spy_str_new(t->accum, t->accum_len);
        }

        size_t nl_start;
        size_t nl_end = scan_newline(chunk, (size_t)avail, t->nl,
                                     /*at_eof=*/0, &nl_start);

        if (nl_end > 0) {
            /* Found a complete line. */
            if (t->accum_len == 0 && t->nl == SPY_NL_NONE) {
                /* Fast path: entire line in one peek, no NL translation.
                   Build spy_Str directly from peek buffer (one copy). */
                spy_Str *s = spy_str_new(chunk, nl_end);
                spy_buffered_consume(t->bio, nl_end);
                return s;
            }

            /* Append line content (before NL) */
            accum_append(t, chunk, nl_start);

            /* Translate newline: always emit \n for universal/lf/crlf/cr */
            if (t->nl != SPY_NL_NONE) {
                accum_append(t, "\n", 1);
            } else {
                /* no translation: keep original newline bytes */
                accum_append(t, chunk + nl_start, nl_end - nl_start);
            }

            spy_buffered_consume(t->bio, nl_end);
            return spy_str_new(t->accum, t->accum_len);
        }

        /* No newline found in current buffer.  Need to check if the buffer
           ends with \r in universal/CRLF mode – the \n might come next. */
        size_t safe = (size_t)avail;
        if ((t->nl == SPY_NL_UNIVERSAL || t->nl == SPY_NL_CRLF) &&
            safe > 0 && chunk[safe - 1] == '\r') {
            safe--;     /* don't consume the \r yet */
        }

        if (safe > 0) {
            accum_append(t, chunk, safe);
            spy_buffered_consume(t->bio, safe);
        }

        if (safe == 0) {
            /* Only a trailing \r in the buffer.  Consume it, then peek
               to see if \n follows (could be \r\n split across fills). */
            spy_buffered_consume(t->bio, (size_t)avail);

            const char *next;
            ssize_t navail = spy_buffered_peek(t->bio, &next);
            if (navail > 0 && next[0] == '\n') {
                /* It was \r\n – consume the \n */
                spy_buffered_consume(t->bio, 1);
            }
            /* Either way, the \r (or \r\n) is a line ending */
            if (t->nl != SPY_NL_NONE)
                accum_append(t, "\n", 1);
            else
                accum_append(t, "\r", 1);  /* keep raw in NONE mode */
            return spy_str_new(t->accum, t->accum_len);
        }
    }
}

/* ── read(nchars) ─────────────────────────────────────────────────── */

/* Count UTF-8 codepoints in buf[0..len).  Returns the byte offset where
   the `nchars`-th codepoint ends, or `len` if fewer than nchars codepoints. */
static size_t utf8_byte_offset(const char *buf, size_t len, size_t nchars)
{
    size_t chars = 0, i = 0;
    while (i < len && chars < nchars) {
        unsigned char c = (unsigned char)buf[i];
        if (c < 0x80)       i += 1;
        else if (c < 0xE0)  i += 2;
        else if (c < 0xF0)  i += 3;
        else                 i += 4;
        chars++;
    }
    return (i > len) ? len : i;
}

spy_Str *spy_text_read(spy_TextIO *t, size_t nchars)
{
    accum_reset(t);

    size_t chars_left = nchars;

    for (;;) {
        const char *chunk;
        ssize_t avail = spy_buffered_peek(t->bio, &chunk);
        if (avail < 0) return NULL;
        if (avail == 0) break;

        /* How many bytes cover `chars_left` codepoints? */
        size_t bytes = utf8_byte_offset(chunk, (size_t)avail, chars_left);

        /* Count actual codepoints we're taking */
        size_t got = 0;
        for (size_t i = 0; i < bytes; ) {
            unsigned char c = (unsigned char)chunk[i];
            if (c < 0x80)       i += 1;
            else if (c < 0xE0)  i += 2;
            else if (c < 0xF0)  i += 3;
            else                 i += 4;
            got++;
        }

        /* Newline translation on the chunk */
        if (t->nl == SPY_NL_NONE) {
            accum_append(t, chunk, bytes);
        } else {
            /* Translate in-place into accum */
            for (size_t i = 0; i < bytes; i++) {
                if (chunk[i] == '\r') {
                    if (t->nl == SPY_NL_UNIVERSAL || t->nl == SPY_NL_CRLF) {
                        accum_append(t, "\n", 1);
                        /* skip \n after \r */
                        if (i + 1 < bytes && chunk[i + 1] == '\n') i++;
                    } else if (t->nl == SPY_NL_CR) {
                        accum_append(t, "\n", 1);
                    } else {
                        accum_append(t, &chunk[i], 1);
                    }
                } else {
                    accum_append(t, &chunk[i], 1);
                }
            }
        }

        spy_buffered_consume(t->bio, bytes);
        chars_left -= got;
        if (chars_left == 0) break;
    }

    if (t->accum_len == 0) return NULL;
    return spy_str_new(t->accum, t->accum_len);
}

/* ── write ────────────────────────────────────────────────────────── */

ssize_t spy_text_write(spy_TextIO *t, const spy_Str *s)
{
    if (t->nl == SPY_NL_NONE || t->nl == SPY_NL_LF) {
        /* No translation needed */
        return spy_buffered_write(t->bio, s->utf8, s->length);
    }

    /* Translate \n → target newline */
    const char *nl_seq;
    size_t nl_len;
    switch (t->nl) {
    case SPY_NL_CRLF:      nl_seq = "\r\n"; nl_len = 2; break;
    case SPY_NL_CR:         nl_seq = "\r";   nl_len = 1; break;
    case SPY_NL_UNIVERSAL:  nl_seq = "\n";   nl_len = 1; break;  /* platform default */
    default:                nl_seq = "\n";   nl_len = 1; break;
    }

    const char *src = s->utf8;
    size_t rem = s->length;
    ssize_t total = 0;

    while (rem > 0) {
        const char *nl = memchr(src, '\n', rem);
        if (!nl) {
            ssize_t w = spy_buffered_write(t->bio, src, rem);
            if (w < 0) return -1;
            total += w;
            break;
        }
        /* Write up to the \n */
        size_t before = (size_t)(nl - src);
        if (before > 0) {
            ssize_t w = spy_buffered_write(t->bio, src, before);
            if (w < 0) return -1;
            total += w;
        }
        /* Write translated newline */
        ssize_t w = spy_buffered_write(t->bio, nl_seq, nl_len);
        if (w < 0) return -1;
        total += w;
        src = nl + 1;
        rem -= before + 1;
    }
    return total;
}

int spy_text_flush(spy_TextIO *t)
{
    return spy_buffered_flush(t->bio);
}

int spy_text_close(spy_TextIO *t)
{
    int ret = spy_buffered_close(t->bio);
    free(t->accum);
    free(t);
    return ret;
}

/* ══════════════════════════════════════════════════════════════════════
   Convenience: spy_open_text / spy_open_binary
   ══════════════════════════════════════════════════════════════════════ */

static void parse_mode(const char *mode, int *flags, int *is_binary)
{
    *is_binary = 0;
    int has_read = 0, has_write = 0, has_append = 0, has_plus = 0;

    for (const char *p = mode; *p; p++) {
        switch (*p) {
        case 'r': has_read   = 1; break;
        case 'w': has_write  = 1; break;
        case 'a': has_append = 1; break;
        case '+': has_plus   = 1; break;
        case 'b': *is_binary = 1; break;
        }
    }

    if (has_read && !has_plus)
        *flags = O_RDONLY;
    else if (has_read && has_plus)
        *flags = O_RDWR;
    else if (has_write && !has_plus)
        *flags = O_WRONLY | O_CREAT | O_TRUNC;
    else if (has_write && has_plus)
        *flags = O_RDWR | O_CREAT | O_TRUNC;
    else if (has_append && !has_plus)
        *flags = O_WRONLY | O_CREAT | O_APPEND;
    else if (has_append && has_plus)
        *flags = O_RDWR | O_CREAT | O_APPEND;
    else
        *flags = O_RDONLY;
}

spy_TextIO *spy_open_text(const char *path, const char *mode)
{
    int flags, is_binary;
    parse_mode(mode, &flags, &is_binary);

    spy_RawIO *raw = spy_raw_open(path, flags, 0666);
    if (!raw) return NULL;

    spy_BufMode bmode = SPY_BUF_READ;
    if (flags & (O_WRONLY | O_RDWR | O_APPEND))
        bmode = (flags & O_RDONLY) ? SPY_BUF_RW :
                (flags & O_RDWR)   ? SPY_BUF_RW : SPY_BUF_WRITE;

    spy_BufferedIO *bio = spy_buffered_new(raw, bmode, 0);
    return spy_text_new(bio, SPY_NL_UNIVERSAL);
}

spy_BufferedIO *spy_open_binary(const char *path, const char *mode)
{
    int flags, is_binary;
    parse_mode(mode, &flags, &is_binary);

    spy_RawIO *raw = spy_raw_open(path, flags, 0666);
    if (!raw) return NULL;

    spy_BufMode bmode = SPY_BUF_READ;
    if (flags & (O_WRONLY | O_RDWR | O_APPEND))
        bmode = (flags & O_RDONLY) ? SPY_BUF_RW :
                (flags & O_RDWR)   ? SPY_BUF_RW : SPY_BUF_WRITE;

    return spy_buffered_new(raw, bmode, 0);
}
