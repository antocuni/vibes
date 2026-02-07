#!/bin/bash
# Test script that starts a local HTTP server and runs the examples against it.
# Usage: ./test_local.sh

set -e

# Start a local test server
python3 << 'PYEOF' &
from http.server import HTTPServer, BaseHTTPRequestHandler
import json

class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        te = self.headers.get('Transfer-Encoding', '')
        cl = self.headers.get('Content-Length')
        body = b''
        if 'chunked' in te.lower():
            while True:
                line = self.rfile.readline().strip()
                if not line: continue
                size = int(line, 16)
                if size == 0:
                    self.rfile.readline()
                    break
                body += self.rfile.read(size)
                self.rfile.readline()
        elif cl:
            body = self.rfile.read(int(cl))
        response = json.dumps({'method':'POST','path':self.path,'body':body.decode(),'body_length':len(body)}, indent=2)
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(response)))
        self.end_headers()
        self.wfile.write(response.encode())
    def do_GET(self):
        response = json.dumps({'method':'GET','path':self.path,'message':'Hello from test server!'}, indent=2)
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(response)))
        self.end_headers()
        self.wfile.write(response.encode())
    def log_message(self, *a): pass

server = HTTPServer(('127.0.0.1', 8080), Handler)
server.allow_reuse_address = True
server.serve_forever()
PYEOF
SERVER_PID=$!
sleep 1

cleanup() { kill $SERVER_PID 2>/dev/null; }
trap cleanup EXIT

echo "=== Testing HTTP GET ==="
# Build local test version (HTTP to localhost:8080)
WASI_SDK_PATH=${WASI_SDK_PATH:-tools/wasi-sdk-25.0-x86_64-linux}
CFLAGS="--target=wasm32-wasip2 --sysroot=$WASI_SDK_PATH/share/wasi-sysroot -I. -O2"

# Create temp GET test targeting localhost
sed 's/WASI_HTTP_TYPES_SCHEME_HTTPS/WASI_HTTP_TYPES_SCHEME_HTTP/;s/"httpbin.org"/"127.0.0.1:8080"/' http_get.c > /tmp/http_get_test.c
$WASI_SDK_PATH/bin/clang $CFLAGS -o /tmp/http_get_test.wasm /tmp/http_get_test.c generated/imports.c generated/imports_component_type.o
wasmtime run -S http -S inherit-network /tmp/http_get_test.wasm
echo ""

echo "=== Testing HTTP POST ==="
sed 's/WASI_HTTP_TYPES_SCHEME_HTTPS/WASI_HTTP_TYPES_SCHEME_HTTP/;s/"httpbin.org"/"127.0.0.1:8080"/' http_post.c > /tmp/http_post_test.c
$WASI_SDK_PATH/bin/clang $CFLAGS -o /tmp/http_post_test.wasm /tmp/http_post_test.c generated/imports.c generated/imports_component_type.o
wasmtime run -S http -S inherit-network /tmp/http_post_test.wasm
echo ""

echo "=== All tests passed! ==="
