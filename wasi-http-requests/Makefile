# Makefile for WASI HTTP request examples
#
# Prerequisites:
#   - wasi-sdk (set WASI_SDK_PATH or will auto-download)
#   - wasmtime (must be in PATH)
#   - wit-bindgen (cargo install wit-bindgen-cli)
#   - wasm-tools (cargo install wasm-tools)

WASI_SDK_PATH ?= tools/wasi-sdk-25.0-x86_64-linux
WASI_SDK_URL  := https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-25/wasi-sdk-25.0-x86_64-linux.tar.gz
WASI_HTTP_URL := https://github.com/WebAssembly/wasi-http/archive/refs/tags/v0.2.8.tar.gz

CC       := $(WASI_SDK_PATH)/bin/clang
SYSROOT  := $(WASI_SDK_PATH)/share/wasi-sysroot
TARGET   := wasm32-wasip2
CFLAGS   := --target=$(TARGET) --sysroot=$(SYSROOT) -I. -O2

GENERATED := generated/imports.c generated/imports.h generated/imports_component_type.o

.PHONY: all clean setup run-get run-post

all: http_get.wasm http_post.wasm

# Generate C bindings from WIT definitions
$(GENERATED) &: wit/proxy.wit
	wit-bindgen c ./wit --world imports --out-dir ./generated

# Build HTTP GET example
http_get.wasm: http_get.c $(GENERATED)
	$(CC) $(CFLAGS) -o $@ http_get.c generated/imports.c generated/imports_component_type.o

# Build HTTP POST example
http_post.wasm: http_post.c $(GENERATED)
	$(CC) $(CFLAGS) -o $@ http_post.c generated/imports.c generated/imports_component_type.o

# Run examples (requires wasmtime in PATH)
run-get: http_get.wasm
	wasmtime run -S http -S inherit-network $<

run-post: http_post.wasm
	wasmtime run -S http -S inherit-network $<

# Download and set up prerequisites
setup: wit/proxy.wit $(WASI_SDK_PATH)/bin/clang
	@echo "Setup complete. Run 'make' to build."

$(WASI_SDK_PATH)/bin/clang:
	@echo "Downloading wasi-sdk..."
	@mkdir -p tools
	@curl -sL $(WASI_SDK_URL) | tar xz -C tools

wit/proxy.wit:
	@echo "Downloading wasi-http WIT files..."
	@curl -sL $(WASI_HTTP_URL) | tar xz
	@cp -r wasi-http-0.2.8/wit ./wit
	@rm -rf wasi-http-0.2.8

clean:
	rm -f http_get.wasm http_post.wasm
	rm -rf generated
