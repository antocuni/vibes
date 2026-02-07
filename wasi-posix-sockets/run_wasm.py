#!/usr/bin/env python3
"""
run_wasm.py - WASM runner with POSIX socket shims

Loads a WASM module compiled with wasi-libc and provides POSIX socket
functions as host imports. The WASM module can call socket(), connect(),
send(), recv(), close(), bind(), listen(), accept(), and shutdown() -
all delegated to real Python socket operations.

Usage:
    python3 run_wasm.py <module.wasm> [args...]

Examples:
    python3 run_wasm.py http_client.wasm 93.184.215.14 80 /
    python3 run_wasm.py echo_server.wasm 8080
"""

import sys
import struct
import socket
import ctypes
import wasmtime


class SocketShim:
    """
    Manages a table of file descriptors mapping to Python socket objects.

    WASM code calls socket functions by fd number; this class translates
    those calls into real Python socket operations and handles reading/writing
    WASM linear memory for buffer arguments.
    """

    def __init__(self):
        self.sockets: dict[int, socket.socket] = {}
        self.next_fd = 100  # start high to avoid collision with WASI fds

    def _alloc_fd(self, sock: socket.socket) -> int:
        fd = self.next_fd
        self.next_fd += 1
        self.sockets[fd] = sock
        return fd

    def _get_sock(self, fd: int) -> socket.socket | None:
        return self.sockets.get(fd)

    def _read_memory(self, caller: wasmtime.Caller, ptr: int, length: int) -> bytes:
        """Read `length` bytes from WASM linear memory at `ptr`."""
        mem = caller["memory"]
        return bytes(mem.read(caller, ptr, ptr + length))

    def _write_memory(self, caller: wasmtime.Caller, ptr: int, data: bytes) -> None:
        """Write `data` into WASM linear memory at `ptr`."""
        mem = caller["memory"]
        mem.write(caller, data, ptr)

    # -- Socket API implementations --

    def impl_socket(self, caller: wasmtime.Caller, domain: int, type_: int, protocol: int) -> int:
        """Create a socket. Returns fd on success, -1 on failure."""
        try:
            # Map WASM constants to Python constants
            py_family = {2: socket.AF_INET}.get(domain, domain)
            py_type = {1: socket.SOCK_STREAM, 2: socket.SOCK_DGRAM}.get(type_, type_)
            py_proto = {6: socket.IPPROTO_TCP}.get(protocol, protocol)

            sock = socket.socket(py_family, py_type, py_proto)
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            fd = self._alloc_fd(sock)
            print(f"  [host] socket({domain}, {type_}, {protocol}) -> fd={fd}", file=sys.stderr)
            return fd
        except OSError as e:
            print(f"  [host] socket() failed: {e}", file=sys.stderr)
            return -1

    def impl_connect(self, caller: wasmtime.Caller, sockfd: int, addr_ptr: int, addrlen: int) -> int:
        """Connect a socket. Reads sockaddr_in from WASM memory."""
        try:
            sock = self._get_sock(sockfd)
            if sock is None:
                return -1

            # Read the sockaddr_in struct from WASM memory
            # Layout: sin_family (u16 LE), sin_port (u16 big-endian/network order),
            #         sin_addr (4 bytes network order), sin_zero (8 bytes)
            addr_bytes = self._read_memory(caller, addr_ptr, addrlen)
            family = struct.unpack_from("<H", addr_bytes, 0)[0]
            port = struct.unpack_from("!H", addr_bytes, 2)[0]  # network byte order
            addr_raw = addr_bytes[4:8]
            ip = socket.inet_ntoa(addr_raw)

            print(f"  [host] connect(fd={sockfd}, {ip}:{port})", file=sys.stderr)
            sock.connect((ip, port))
            return 0
        except OSError as e:
            print(f"  [host] connect() failed: {e}", file=sys.stderr)
            return -1

    def impl_send(self, caller: wasmtime.Caller, sockfd: int, buf_ptr: int, length: int, flags: int) -> int:
        """Send data through a socket."""
        try:
            sock = self._get_sock(sockfd)
            if sock is None:
                return -1
            data = self._read_memory(caller, buf_ptr, length)
            sent = sock.send(data)
            print(f"  [host] send(fd={sockfd}, {sent} bytes)", file=sys.stderr)
            return sent
        except OSError as e:
            print(f"  [host] send() failed: {e}", file=sys.stderr)
            return -1

    def impl_recv(self, caller: wasmtime.Caller, sockfd: int, buf_ptr: int, length: int, flags: int) -> int:
        """Receive data from a socket into WASM memory."""
        try:
            sock = self._get_sock(sockfd)
            if sock is None:
                return -1
            data = sock.recv(length)
            if len(data) == 0:
                return 0  # connection closed
            self._write_memory(caller, buf_ptr, data)
            print(f"  [host] recv(fd={sockfd}, {len(data)} bytes)", file=sys.stderr)
            return len(data)
        except OSError as e:
            print(f"  [host] recv() failed: {e}", file=sys.stderr)
            return -1

    def impl_close(self, caller: wasmtime.Caller, sockfd: int) -> int:
        """Close a socket."""
        try:
            sock = self._get_sock(sockfd)
            if sock is None:
                return -1
            sock.close()
            del self.sockets[sockfd]
            print(f"  [host] close(fd={sockfd})", file=sys.stderr)
            return 0
        except OSError as e:
            print(f"  [host] close() failed: {e}", file=sys.stderr)
            return -1

    def impl_bind(self, caller: wasmtime.Caller, sockfd: int, addr_ptr: int, addrlen: int) -> int:
        """Bind a socket to an address."""
        try:
            sock = self._get_sock(sockfd)
            if sock is None:
                return -1

            addr_bytes = self._read_memory(caller, addr_ptr, addrlen)
            family = struct.unpack_from("<H", addr_bytes, 0)[0]
            port = struct.unpack_from("!H", addr_bytes, 2)[0]  # network byte order
            addr_raw = addr_bytes[4:8]
            ip = socket.inet_ntoa(addr_raw)

            print(f"  [host] bind(fd={sockfd}, {ip}:{port})", file=sys.stderr)
            sock.bind((ip, port))
            return 0
        except OSError as e:
            print(f"  [host] bind() failed: {e}", file=sys.stderr)
            return -1

    def impl_listen(self, caller: wasmtime.Caller, sockfd: int, backlog: int) -> int:
        """Start listening on a socket."""
        try:
            sock = self._get_sock(sockfd)
            if sock is None:
                return -1
            print(f"  [host] listen(fd={sockfd}, backlog={backlog})", file=sys.stderr)
            sock.listen(backlog)
            return 0
        except OSError as e:
            print(f"  [host] listen() failed: {e}", file=sys.stderr)
            return -1

    def impl_accept(self, caller: wasmtime.Caller, sockfd: int, addr_ptr: int, addrlen_ptr: int) -> int:
        """Accept a connection. Writes client sockaddr_in back to WASM memory."""
        try:
            sock = self._get_sock(sockfd)
            if sock is None:
                return -1
            conn, (ip, port) = sock.accept()
            new_fd = self._alloc_fd(conn)
            print(f"  [host] accept(fd={sockfd}) -> fd={new_fd} from {ip}:{port}", file=sys.stderr)

            # Write the client address back into WASM memory if requested
            if addr_ptr != 0:
                addr_bytes = struct.pack("<H", 2)  # AF_INET (LE)
                addr_bytes += struct.pack("!H", port)  # port in network byte order
                addr_bytes += socket.inet_aton(ip)  # addr in network byte order
                addr_bytes += b'\x00' * 8  # sin_zero
                self._write_memory(caller, addr_ptr, addr_bytes)
            if addrlen_ptr != 0:
                self._write_memory(caller, addrlen_ptr, struct.pack("<I", 16))

            return new_fd
        except OSError as e:
            print(f"  [host] accept() failed: {e}", file=sys.stderr)
            return -1

    def impl_shutdown(self, caller: wasmtime.Caller, sockfd: int, how: int) -> int:
        """Shutdown a socket."""
        try:
            sock = self._get_sock(sockfd)
            if sock is None:
                return -1
            py_how = {0: socket.SHUT_RD, 1: socket.SHUT_WR, 2: socket.SHUT_RDWR}.get(how, how)
            sock.shutdown(py_how)
            print(f"  [host] shutdown(fd={sockfd}, how={how})", file=sys.stderr)
            return 0
        except OSError as e:
            print(f"  [host] shutdown() failed: {e}", file=sys.stderr)
            return -1


def run_wasm_module(wasm_path: str, args: list[str]) -> int:
    """Load and run a WASM module with POSIX socket shims."""

    shim = SocketShim()

    # Configure engine and store
    engine = wasmtime.Engine()
    module = wasmtime.Module.from_file(engine, wasm_path)

    # Set up WASI
    wasi_config = wasmtime.WasiConfig()
    wasi_config.argv = [wasm_path] + args
    wasi_config.inherit_stdout()
    wasi_config.inherit_stderr()
    wasi_config.inherit_stdin()

    store = wasmtime.Store(engine)
    store.set_wasi(wasi_config)

    # Create linker and define WASI
    linker = wasmtime.Linker(engine)
    linker.define_wasi()

    # Define POSIX socket imports
    # All functions use i32 params/returns. The "buf" pointers are i32 (WASM pointers).
    i32 = wasmtime.ValType.i32()

    # socket(domain, type, protocol) -> fd
    linker.define_func("posix_sockets", "socket",
        wasmtime.FuncType([i32, i32, i32], [i32]),
        shim.impl_socket, access_caller=True)

    # connect(sockfd, addr_ptr, addrlen) -> result
    linker.define_func("posix_sockets", "connect",
        wasmtime.FuncType([i32, i32, i32], [i32]),
        shim.impl_connect, access_caller=True)

    # send(sockfd, buf_ptr, len, flags) -> bytes_sent
    linker.define_func("posix_sockets", "send",
        wasmtime.FuncType([i32, i32, i32, i32], [i32]),
        shim.impl_send, access_caller=True)

    # recv(sockfd, buf_ptr, len, flags) -> bytes_received
    linker.define_func("posix_sockets", "recv",
        wasmtime.FuncType([i32, i32, i32, i32], [i32]),
        shim.impl_recv, access_caller=True)

    # close(sockfd) -> result
    linker.define_func("posix_sockets", "close",
        wasmtime.FuncType([i32], [i32]),
        shim.impl_close, access_caller=True)

    # bind(sockfd, addr_ptr, addrlen) -> result
    linker.define_func("posix_sockets", "bind",
        wasmtime.FuncType([i32, i32, i32], [i32]),
        shim.impl_bind, access_caller=True)

    # listen(sockfd, backlog) -> result
    linker.define_func("posix_sockets", "listen",
        wasmtime.FuncType([i32, i32], [i32]),
        shim.impl_listen, access_caller=True)

    # accept(sockfd, addr_ptr, addrlen_ptr) -> new_fd
    linker.define_func("posix_sockets", "accept",
        wasmtime.FuncType([i32, i32, i32], [i32]),
        shim.impl_accept, access_caller=True)

    # shutdown(sockfd, how) -> result
    linker.define_func("posix_sockets", "shutdown",
        wasmtime.FuncType([i32, i32], [i32]),
        shim.impl_shutdown, access_caller=True)

    # Instantiate and run
    instance = linker.instantiate(store, module)

    # WASI modules export _start
    start = instance.exports(store).get("_start")
    if start is None:
        print("Error: module has no _start export", file=sys.stderr)
        return 1

    try:
        start(store)
        return 0
    except wasmtime.ExitTrap as e:
        return e.code
    except wasmtime.WasmtimeError as e:
        print(f"WASM trap: {e}", file=sys.stderr)
        return 1


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 run_wasm.py <module.wasm> [args...]")
        print()
        print("Examples:")
        print("  python3 run_wasm.py http_client.wasm 93.184.215.14 80 /")
        print("  python3 run_wasm.py echo_server.wasm 8080")
        sys.exit(1)

    wasm_path = sys.argv[1]
    wasm_args = sys.argv[2:]

    exit_code = run_wasm_module(wasm_path, wasm_args)
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
