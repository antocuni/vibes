#define _GNU_SOURCE
#include "spy_io.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define TMPFILE "/tmp/spy_io_test.tmp"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)  do { \
    tests_run++; \
    printf("  %-50s ", #name); \
    fflush(stdout); \
} while(0)

#define PASS()  do { tests_passed++; printf("OK\n"); } while(0)

#define ASSERT_STR(s, expected) do { \
    assert((s) != NULL); \
    assert((s)->length == strlen(expected)); \
    assert(memcmp((s)->utf8, (expected), (s)->length) == 0); \
} while(0)

/* ── Test: Raw IO ─────────────────────────────────────────────────── */

static void test_raw_io(void)
{
    printf("Raw IO:\n");

    TEST(write_and_read_back);
    {
        spy_RawIO *w = spy_raw_open(TMPFILE, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        assert(w != NULL);
        const char *msg = "Hello, raw IO!";
        ssize_t n = spy_raw_write(w, msg, strlen(msg));
        assert(n == (ssize_t)strlen(msg));
        spy_raw_close(w);

        spy_RawIO *r = spy_raw_open(TMPFILE, O_RDONLY, 0);
        assert(r != NULL);
        char buf[64] = {0};
        n = spy_raw_read(r, buf, sizeof(buf));
        assert(n == (ssize_t)strlen(msg));
        assert(memcmp(buf, msg, (size_t)n) == 0);
        spy_raw_close(r);
    }
    PASS();

    TEST(seek);
    {
        spy_RawIO *r = spy_raw_open(TMPFILE, O_RDONLY, 0);
        off_t pos = spy_raw_seek(r, 7, SEEK_SET);
        assert(pos == 7);
        char buf[16] = {0};
        ssize_t n = spy_raw_read(r, buf, 7);
        assert(n == 7);
        assert(memcmp(buf, "raw IO!", 7) == 0);
        spy_raw_close(r);
    }
    PASS();
}

/* ── Test: Buffered IO ────────────────────────────────────────────── */

static void test_buffered_io(void)
{
    printf("Buffered IO:\n");

    TEST(buffered_write_and_read);
    {
        spy_RawIO *raw = spy_raw_open(TMPFILE, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        spy_BufferedIO *bw = spy_buffered_new(raw, SPY_BUF_WRITE, 32);
        const char *line1 = "first line\n";
        const char *line2 = "second line\n";
        spy_buffered_write(bw, line1, strlen(line1));
        spy_buffered_write(bw, line2, strlen(line2));
        spy_buffered_close(bw);

        raw = spy_raw_open(TMPFILE, O_RDONLY, 0);
        spy_BufferedIO *br = spy_buffered_new(raw, SPY_BUF_READ, 32);
        char buf[64] = {0};
        ssize_t n = spy_buffered_read(br, buf, sizeof(buf));
        assert(n == (ssize_t)(strlen(line1) + strlen(line2)));
        assert(memcmp(buf, "first line\nsecond line\n", (size_t)n) == 0);
        spy_buffered_close(br);
    }
    PASS();

    TEST(peek_and_consume);
    {
        spy_RawIO *raw = spy_raw_open(TMPFILE, O_RDONLY, 0);
        spy_BufferedIO *br = spy_buffered_new(raw, SPY_BUF_READ, 64);

        const char *p;
        ssize_t avail = spy_buffered_peek(br, &p);
        assert(avail > 0);
        assert(memcmp(p, "first", 5) == 0);

        spy_buffered_consume(br, 6);  /* consume "first " */
        avail = spy_buffered_peek(br, &p);
        assert(avail > 0);
        assert(memcmp(p, "line\n", 5) == 0);

        spy_buffered_close(br);
    }
    PASS();

    TEST(small_buffer_forces_multiple_fills);
    {
        /* Write a longer string, read with tiny buffer */
        spy_RawIO *raw = spy_raw_open(TMPFILE, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        spy_BufferedIO *bw = spy_buffered_new(raw, SPY_BUF_WRITE, 8);
        const char *data = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        spy_buffered_write(bw, data, 26);
        spy_buffered_close(bw);

        raw = spy_raw_open(TMPFILE, O_RDONLY, 0);
        spy_BufferedIO *br = spy_buffered_new(raw, SPY_BUF_READ, 8);
        char buf[64] = {0};
        ssize_t n = spy_buffered_read(br, buf, 26);
        assert(n == 26);
        assert(memcmp(buf, data, 26) == 0);
        spy_buffered_close(br);
    }
    PASS();
}

/* ── Test: Text IO – readline ─────────────────────────────────────── */

static void write_raw_file(const char *path, const char *data, size_t len)
{
    spy_RawIO *w = spy_raw_open(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    spy_raw_write(w, data, len);
    spy_raw_close(w);
}

static spy_TextIO *open_text_reader(const char *path, spy_NewlineMode nl, size_t bufsz)
{
    spy_RawIO *raw = spy_raw_open(path, O_RDONLY, 0);
    spy_BufferedIO *bio = spy_buffered_new(raw, SPY_BUF_READ, bufsz);
    return spy_text_new(bio, nl);
}

static void test_text_readline(void)
{
    printf("Text IO readline:\n");

    TEST(unix_newlines);
    {
        write_raw_file(TMPFILE, "hello\nworld\n", 12);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_LF, 64);
        spy_Str *s;

        s = spy_text_readline(t);
        ASSERT_STR(s, "hello\n");

        s = spy_text_readline(t);
        ASSERT_STR(s, "world\n");

        s = spy_text_readline(t);
        assert(s == NULL);

        spy_text_close(t);
    }
    PASS();

    TEST(no_trailing_newline);
    {
        write_raw_file(TMPFILE, "hello\nworld", 11);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_LF, 64);

        spy_Str *s = spy_text_readline(t);
        ASSERT_STR(s, "hello\n");

        s = spy_text_readline(t);
        ASSERT_STR(s, "world");

        s = spy_text_readline(t);
        assert(s == NULL);

        spy_text_close(t);
    }
    PASS();

    TEST(universal_crlf);
    {
        write_raw_file(TMPFILE, "aaa\r\nbbb\r\n", 10);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_UNIVERSAL, 64);

        spy_Str *s = spy_text_readline(t);
        ASSERT_STR(s, "aaa\n");

        s = spy_text_readline(t);
        ASSERT_STR(s, "bbb\n");

        s = spy_text_readline(t);
        assert(s == NULL);

        spy_text_close(t);
    }
    PASS();

    TEST(universal_bare_cr);
    {
        write_raw_file(TMPFILE, "xx\ryy\r", 6);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_UNIVERSAL, 64);

        spy_Str *s = spy_text_readline(t);
        ASSERT_STR(s, "xx\n");

        s = spy_text_readline(t);
        ASSERT_STR(s, "yy\n");

        s = spy_text_readline(t);
        assert(s == NULL);

        spy_text_close(t);
    }
    PASS();

    TEST(universal_mixed);
    {
        /* Mix of \n, \r\n, and \r */
        write_raw_file(TMPFILE, "a\nb\r\nc\rd", 8);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_UNIVERSAL, 64);

        spy_Str *s;
        s = spy_text_readline(t); ASSERT_STR(s, "a\n");
        s = spy_text_readline(t); ASSERT_STR(s, "b\n");
        s = spy_text_readline(t); ASSERT_STR(s, "c\n");
        s = spy_text_readline(t); ASSERT_STR(s, "d");
        s = spy_text_readline(t); assert(s == NULL);

        spy_text_close(t);
    }
    PASS();

    TEST(line_spans_buffer_boundary);
    {
        /* Use a 4-byte buffer so lines must span multiple fills */
        write_raw_file(TMPFILE, "hello world\ndone\n", 17);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_LF, 4);

        spy_Str *s;
        s = spy_text_readline(t); ASSERT_STR(s, "hello world\n");
        s = spy_text_readline(t); ASSERT_STR(s, "done\n");
        s = spy_text_readline(t); assert(s == NULL);

        spy_text_close(t);
    }
    PASS();

    TEST(crlf_split_across_buffer);
    {
        /* \r\n split exactly at buffer boundary (buf=5: "abc\r" then "\n...") */
        write_raw_file(TMPFILE, "abc\r\nxyz\n", 9);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_UNIVERSAL, 4);

        spy_Str *s;
        s = spy_text_readline(t); ASSERT_STR(s, "abc\n");
        s = spy_text_readline(t); ASSERT_STR(s, "xyz\n");
        s = spy_text_readline(t); assert(s == NULL);

        spy_text_close(t);
    }
    PASS();

    TEST(no_translation_mode);
    {
        write_raw_file(TMPFILE, "aaa\r\nbbb\n", 9);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_NONE, 64);

        spy_Str *s;
        /* SPY_NL_NONE uses \n as delimiter but doesn't translate */
        s = spy_text_readline(t);
        /* The \r stays, line ends at \n */
        ASSERT_STR(s, "aaa\r\n");

        s = spy_text_readline(t);
        ASSERT_STR(s, "bbb\n");

        s = spy_text_readline(t);
        assert(s == NULL);

        spy_text_close(t);
    }
    PASS();
}

/* ── Test: Text IO – read(nchars) ─────────────────────────────────── */

static void test_text_read(void)
{
    printf("Text IO read:\n");

    TEST(read_ascii_chars);
    {
        write_raw_file(TMPFILE, "Hello, World!", 13);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_NONE, 64);

        spy_Str *s = spy_text_read(t, 5);
        ASSERT_STR(s, "Hello");

        s = spy_text_read(t, 100);
        ASSERT_STR(s, ", World!");

        s = spy_text_read(t, 1);
        assert(s == NULL);

        spy_text_close(t);
    }
    PASS();

    TEST(read_utf8_multibyte);
    {
        /* "café" = 63 61 66 c3a9 = 5 bytes, 4 codepoints */
        const char cafe[] = "caf\xc3\xa9";
        write_raw_file(TMPFILE, cafe, 5);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_NONE, 64);

        spy_Str *s = spy_text_read(t, 3);
        ASSERT_STR(s, "caf");

        s = spy_text_read(t, 1);  /* should get the é (2 bytes) */
        assert(s != NULL);
        assert(s->length == 2);
        assert(memcmp(s->utf8, "\xc3\xa9", 2) == 0);

        spy_text_close(t);
    }
    PASS();

    TEST(read_with_newline_translation);
    {
        write_raw_file(TMPFILE, "a\r\nb\r\n", 6);
        spy_TextIO *t = open_text_reader(TMPFILE, SPY_NL_UNIVERSAL, 64);

        spy_Str *s = spy_text_read(t, 100);
        ASSERT_STR(s, "a\nb\n");

        spy_text_close(t);
    }
    PASS();
}

/* ── Test: Text IO – write ────────────────────────────────────────── */

static void test_text_write(void)
{
    printf("Text IO write:\n");

    TEST(write_no_translation);
    {
        spy_RawIO *raw = spy_raw_open(TMPFILE, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        spy_BufferedIO *bio = spy_buffered_new(raw, SPY_BUF_WRITE, 64);
        spy_TextIO *t = spy_text_new(bio, SPY_NL_NONE);

        spy_Str *s = spy_str_new("hello\nworld\n", 12);
        spy_text_write(t, s);
        spy_text_close(t);

        /* Verify raw content */
        char buf[64] = {0};
        spy_RawIO *r = spy_raw_open(TMPFILE, O_RDONLY, 0);
        ssize_t n = spy_raw_read(r, buf, sizeof(buf));
        spy_raw_close(r);
        assert(n == 12);
        assert(memcmp(buf, "hello\nworld\n", 12) == 0);
    }
    PASS();

    TEST(write_crlf_translation);
    {
        spy_RawIO *raw = spy_raw_open(TMPFILE, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        spy_BufferedIO *bio = spy_buffered_new(raw, SPY_BUF_WRITE, 64);
        spy_TextIO *t = spy_text_new(bio, SPY_NL_CRLF);

        spy_Str *s = spy_str_new("a\nb\n", 4);
        spy_text_write(t, s);
        spy_text_close(t);

        char buf[64] = {0};
        spy_RawIO *r = spy_raw_open(TMPFILE, O_RDONLY, 0);
        ssize_t n = spy_raw_read(r, buf, sizeof(buf));
        spy_raw_close(r);
        assert(n == 6);
        assert(memcmp(buf, "a\r\nb\r\n", 6) == 0);
    }
    PASS();

    TEST(write_cr_translation);
    {
        spy_RawIO *raw = spy_raw_open(TMPFILE, O_WRONLY|O_CREAT|O_TRUNC, 0666);
        spy_BufferedIO *bio = spy_buffered_new(raw, SPY_BUF_WRITE, 64);
        spy_TextIO *t = spy_text_new(bio, SPY_NL_CR);

        spy_Str *s = spy_str_new("x\ny\n", 4);
        spy_text_write(t, s);
        spy_text_close(t);

        char buf[64] = {0};
        spy_RawIO *r = spy_raw_open(TMPFILE, O_RDONLY, 0);
        ssize_t n = spy_raw_read(r, buf, sizeof(buf));
        spy_raw_close(r);
        assert(n == 4);
        assert(memcmp(buf, "x\ry\r", 4) == 0);
    }
    PASS();
}

/* ── Test: Convenience open ───────────────────────────────────────── */

static void test_convenience(void)
{
    printf("Convenience:\n");

    TEST(spy_open_text_write_and_read);
    {
        spy_TextIO *w = spy_open_text(TMPFILE, "w");
        assert(w != NULL);
        spy_Str *s = spy_str_new("line one\nline two\n", 18);
        spy_text_write(w, s);
        spy_text_close(w);

        spy_TextIO *r = spy_open_text(TMPFILE, "r");
        assert(r != NULL);

        s = spy_text_readline(r);
        ASSERT_STR(s, "line one\n");

        s = spy_text_readline(r);
        ASSERT_STR(s, "line two\n");

        s = spy_text_readline(r);
        assert(s == NULL);

        spy_text_close(r);
    }
    PASS();
}

/* ── Test: Socket IO ──────────────────────────────────────────────── */

static void test_socket_io(void)
{
    printf("Socket IO:\n");

    TEST(socketpair_text_io);
    {
        int fds[2];
        int rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
        assert(rc == 0);

        /* Writer side */
        spy_RawIO *raw_w = spy_raw_from_fd(fds[0], 1);
        spy_BufferedIO *bio_w = spy_buffered_new(raw_w, SPY_BUF_WRITE, 32);
        spy_TextIO *tw = spy_text_new(bio_w, SPY_NL_NONE);

        spy_Str *s = spy_str_new("socket line 1\nsocket line 2\n", 28);
        spy_text_write(tw, s);
        spy_text_close(tw);    /* flushes + closes fd */

        /* Reader side */
        spy_RawIO *raw_r = spy_raw_from_fd(fds[1], 1);
        spy_BufferedIO *bio_r = spy_buffered_new(raw_r, SPY_BUF_READ, 16);
        spy_TextIO *tr = spy_text_new(bio_r, SPY_NL_UNIVERSAL);

        s = spy_text_readline(tr);
        ASSERT_STR(s, "socket line 1\n");

        s = spy_text_readline(tr);
        ASSERT_STR(s, "socket line 2\n");

        s = spy_text_readline(tr);
        assert(s == NULL);

        spy_text_close(tr);
    }
    PASS();
}

/* ── Main ─────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== spy_io test suite ===\n\n");

    test_raw_io();
    test_buffered_io();
    test_text_readline();
    test_text_read();
    test_text_write();
    test_convenience();
    test_socket_io();

    printf("\n%d / %d tests passed.\n", tests_passed, tests_run);
    unlink(TMPFILE);
    return (tests_passed == tests_run) ? 0 : 1;
}
