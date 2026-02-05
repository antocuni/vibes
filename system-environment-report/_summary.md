Anthropic's Claude Code Remote environment operates as a secure, containerized Linux (Ubuntu 24.04 LTS) sandbox using Google's [gVisor](https://gvisor.dev/) for kernel-level isolation. It provides strong compute resources (16-core CPU with modern SIMD/AI extensions, 21 GB RAM), a network-limited proxy for package/code hosting, and a broad suite of pre-installed development tools and languages (Python, Node.js, Go, Rust, Java, and more). The system mounts a network-rooted 9p filesystem, enforces limits on open files and stack size, disables direct internet and Docker/POD access, and runs as root within the sandbox, balancing usability for coding with robust security boundaries. This setup enables advanced, reproducible code execution while safeguarding against risky operations and network exposure.

Key features/highlights:
- Linux (Ubuntu 24.04 LTS), run via gVisor (`runsc`) user-space kernel, root filesystem over [9p protocol](https://en.wikipedia.org/wiki/9P_(protocol)).
- 16 physical CPU cores, AVX-512/AMX acceleration, 21 GB RAM, and 30 GB ephemeral disk.
- Pre-installed: Python 3.11, Node.js 22, Go 1.24, Rust 1.93, Java 21, Ruby, PHP, Perl, Bun, plus major build tools/package managers.
- No direct internet access; network requests routed through an authenticated proxy (whitelist includes popular package/code hosts).
- Cgroups for resource isolation, root user/context, and enforced ulimit (20,000 open files, 8 MB stack).

For more details or full documentation, see [Claude Code](https://claude.ai/code) and [gVisor](https://gvisor.dev/).
