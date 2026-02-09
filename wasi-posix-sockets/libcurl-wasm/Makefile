CC = clang
CFLAGS = --target=wasm32-wasi \
         -L/usr/lib/wasm32-wasi \
         -B/usr/lib/wasm32-wasi \
         --sysroot=/usr \
         -I/usr/include/wasm32-wasi \
         -O2
LDFLAGS = -Wl,--allow-undefined

all: curl_demo.wasm

curl_demo.wasm: curl_demo.c curl_shim.c curl_shim.h ../posix_sockets.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ curl_demo.c curl_shim.c

clean:
	rm -f *.wasm

.PHONY: all clean
