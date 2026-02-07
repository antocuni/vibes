CC = clang
CFLAGS = --target=wasm32-wasi \
         -L/usr/lib/wasm32-wasi \
         -B/usr/lib/wasm32-wasi \
         --sysroot=/usr \
         -I/usr/include/wasm32-wasi \
         -O2

# Allow undefined symbols (they'll be provided by the host)
LDFLAGS = -Wl,--allow-undefined

all: http_client.wasm echo_server.wasm

http_client.wasm: http_client.c posix_sockets.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ http_client.c

echo_server.wasm: echo_server.c posix_sockets.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ echo_server.c

clean:
	rm -f *.wasm

.PHONY: all clean
