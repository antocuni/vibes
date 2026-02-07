#!/usr/bin/env python3
"""
test_demo.py - End-to-end test for the WASM POSIX sockets demo

Starts a local HTTP server, then runs the WASM HTTP client against it.
Also tests the echo server by running it in a thread and connecting to it.
"""

import sys
import socket
import threading
import time
import os

# Add project directory to path
sys.path.insert(0, os.path.dirname(__file__))
from run_wasm import run_wasm_module, SocketShim

import wasmtime


def find_free_port():
    """Find a free TCP port."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(('127.0.0.1', 0))
        return s.getsockname()[1]


def run_test_http_server(port, ready_event):
    """A tiny HTTP server that serves one request then stops."""
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(('127.0.0.1', port))
    srv.listen(1)
    srv.settimeout(10)
    ready_event.set()

    try:
        conn, addr = srv.accept()
        request = conn.recv(4096)
        print(f"[test-server] Got request ({len(request)} bytes) from {addr}", file=sys.stderr)

        body = "Hello from the test HTTP server!\nThis response was served to a WASM module.\n"
        response = (
            f"HTTP/1.0 200 OK\r\n"
            f"Content-Type: text/plain\r\n"
            f"Content-Length: {len(body)}\r\n"
            f"\r\n"
            f"{body}"
        )
        conn.sendall(response.encode())
        conn.close()
    except socket.timeout:
        print("[test-server] Timed out waiting for connection", file=sys.stderr)
    finally:
        srv.close()


def test_http_client():
    """Test: WASM HTTP client fetches from a local server."""
    print("=" * 60)
    print("TEST 1: WASM HTTP Client -> Local HTTP Server")
    print("=" * 60)

    port = find_free_port()
    ready = threading.Event()

    server_thread = threading.Thread(target=run_test_http_server, args=(port, ready))
    server_thread.daemon = True
    server_thread.start()
    ready.wait(timeout=5)

    wasm_path = os.path.join(os.path.dirname(__file__), "http_client.wasm")
    exit_code = run_wasm_module(wasm_path, ["127.0.0.1", str(port), "/"])

    server_thread.join(timeout=5)

    print(f"\nExit code: {exit_code}")
    assert exit_code == 0, f"HTTP client exited with code {exit_code}"
    print("TEST 1 PASSED\n")


def test_echo_server():
    """Test: WASM echo server accepts a connection, echoes data back."""
    print("=" * 60)
    print("TEST 2: WASM Echo Server <- Local Client")
    print("=" * 60)

    port = find_free_port()

    wasm_path = os.path.join(os.path.dirname(__file__), "echo_server.wasm")

    # Run WASM echo server in a thread
    result = {"exit_code": None, "error": None}

    def run_server():
        try:
            result["exit_code"] = run_wasm_module(wasm_path, [str(port)])
        except Exception as e:
            result["error"] = e

    server_thread = threading.Thread(target=run_server)
    server_thread.daemon = True
    server_thread.start()

    # Give the WASM server time to start listening
    time.sleep(1)

    # Connect and send test data
    print(f"[test-client] Connecting to echo server on port {port}...", file=sys.stderr)
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.settimeout(5)
    client.connect(('127.0.0.1', port))

    test_messages = [b"Hello WASM!", b"Echo test 123", b"Final message"]
    for msg in test_messages:
        client.send(msg)
        echo = client.recv(4096)
        print(f"[test-client] Sent: {msg!r}, Got: {echo!r}", file=sys.stderr)
        assert echo == msg, f"Expected {msg!r}, got {echo!r}"

    client.close()
    server_thread.join(timeout=5)

    print(f"\nServer exit code: {result['exit_code']}")
    if result["error"]:
        print(f"Server error: {result['error']}")
    print("TEST 2 PASSED\n")


def test_curl_demo():
    """Test: WASM curl demo fetches from a local server."""
    print("=" * 60)
    print("TEST 3: WASM curl demo (libcurl-compatible API)")
    print("=" * 60)

    port = find_free_port()
    ready = threading.Event()

    server_thread = threading.Thread(target=run_test_http_server, args=(port, ready))
    server_thread.daemon = True
    server_thread.start()
    ready.wait(timeout=5)

    wasm_path = os.path.join(os.path.dirname(__file__), "libcurl-wasm", "curl_demo.wasm")
    exit_code = run_wasm_module(wasm_path, [f"http://127.0.0.1:{port}/test"])

    server_thread.join(timeout=5)

    print(f"\nExit code: {exit_code}")
    assert exit_code == 0, f"curl demo exited with code {exit_code}"
    print("TEST 3 PASSED\n")


def main():
    print("WASM POSIX Sockets Demo - End-to-End Tests")
    print()

    test_http_client()
    test_echo_server()
    test_curl_demo()

    print("=" * 60)
    print("ALL TESTS PASSED")
    print("=" * 60)


if __name__ == "__main__":
    main()
