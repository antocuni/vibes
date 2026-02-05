# System Environment Report

A detailed exploration of the Claude Code Remote computing environment, documenting the sandbox infrastructure, available resources, and development tools.

## What was explored

This project documents the computing environment that Claude Code runs in when operating in "remote" mode on Anthropic's infrastructure. The goal was to understand:

- What operating system and kernel powers the environment
- Available CPU, memory, and storage resources
- Network access patterns and restrictions
- Pre-installed programming languages and development tools
- Security and isolation mechanisms

---

## Executive Summary

This environment is a **containerized sandbox** running on Anthropic's Claude Code Remote infrastructure. It's a Linux-based virtual environment with controlled network egress, designed for secure code execution.

---

## Operating System

| Property | Value |
|----------|-------|
| **Distribution** | Ubuntu 24.04.3 LTS (Noble Numbat) |
| **Kernel** | Linux 4.4.0 x86_64 |
| **Runtime** | gVisor (`runsc`) sandbox |

### Full OS Release Information

```
PRETTY_NAME="Ubuntu 24.04.3 LTS"
NAME="Ubuntu"
VERSION_ID="24.04"
VERSION="24.04.3 LTS (Noble Numbat)"
VERSION_CODENAME=noble
ID=ubuntu
ID_LIKE=debian
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
UBUNTU_CODENAME=noble
```

### About gVisor

The kernel is named `runsc`, indicating this runs inside **gVisor** - Google's secure container runtime that provides kernel-level isolation by intercepting and emulating system calls. gVisor implements a user-space kernel that intercepts application system calls and acts as the guest kernel, providing strong isolation without the overhead of full virtualization.

---

## CPU Information

| Property | Value |
|----------|-------|
| **Vendor** | GenuineIntel |
| **Architecture** | x86_64 (64-bit) |
| **CPU Family** | 6, Model 207 (likely Emerald Rapids or similar server CPU) |
| **Cores** | 16 physical cores |
| **Threads per core** | 1 (no hyperthreading exposed) |
| **Clock Speed** | 2.1 GHz |
| **Cache** | 8 MB L2/L3 per core |
| **Address Space** | 46-bit physical, 48-bit virtual |
| **BogoMIPS** | 2100.00 |

### CPU Layout

```
Architecture:        x86_64
CPU op-mode(s):      32-bit, 64-bit
Address sizes:       46 bits physical, 48 bits virtual
Byte Order:          Little Endian
CPU(s):              16
On-line CPU(s) list: 0-15
Vendor ID:           GenuineIntel
Model name:          unknown
CPU family:          6
Model:               207
Thread(s) per core:  1
Core(s) per socket:  16
Socket(s):           1
```

### CPU Feature Flags

The CPU supports an extensive set of modern features:

**SIMD & Vector Extensions:**
- SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2
- AVX, AVX2, F16C, FMA
- **AVX-512**: F, DQ, BW, VL, CD, VBMI, VBMI2, VNNI, BITALG, VPOPCNTDQ, FP16

**AI/ML Acceleration:**
- **AMX** (Advanced Matrix Extensions): BF16, INT8, Tile

**Cryptography:**
- AES-NI (hardware AES)
- SHA-NI (hardware SHA)
- PCLMULQDQ, VAES, VPCLMULQDQ

**Memory & Virtualization:**
- VMX (hardware virtualization)
- RDRAND, RDSEED (hardware random)
- TSX (Transactional Synchronization Extensions): HLE, RTM

**Full Flag List:**
```
fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36
clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm pni pclmulqdq
vmx ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt aes xsave avx
f16c rdrand hypervisor lahf_lm abm 3dnowprefetch fsgsbase tsc_adjust bmi1
hle avx2 smep bmi2 erms invpcid rtm avx512f avx512dq rdseed adx smap clwb
avx512cd sha_ni avx512bw avx512vl xsaveopt xsavec xgetbv1 xsaves avx512vbmi
umip avx512_vbmi2 gfni vaes vpclmulqdq avx512_vnni avx512_bitalg
avx512_vpopcntdq la57 rdpid cldemote movdiri movdir64b fsrm md_clear
serialize tsxldtrk amx_bf16 avx512_fp16 amx_tile amx_int8 arch_capabilities
```

---

## Memory

| Type | Total | Used | Available |
|------|-------|------|-----------|
| **RAM** | 21 GB | ~400 MB | ~20 GB |
| **Swap** | 0 | 0 | 0 |

```
               total        used        free      shared  buff/cache   available
Mem:            21Gi       398Mi        20Gi          0B       129Mi        20Gi
Swap:             0B          0B          0B
```

The environment has substantial memory available with no swap configured - all memory operations are in RAM.

---

## Storage & Filesystems

### Disk Space

| Filesystem | Size | Used | Available | Use% | Mount Point |
|------------|------|------|-----------|------|-------------|
| Root (9p) | 30 GB | 9.1 MB | ~30 GB | 1% | `/` |
| tmpfs | 315 GB | 0 | 315 GB | 0% | `/dev` |
| tmpfs | 315 GB | 0 | 315 GB | 0% | `/dev/shm` |
| tmpfs | 315 GB | 0 | 315 GB | 0% | `/sys/fs/cgroup` |

### Mount Details

```
none / 9p rw,trans=fd,rfdno=4,wfdno=4,aname=/,dfltuid=4294967294,dfltgid=4294967294,dcache=1000,cache=remote_revalidating,disable_fifo_open,overlayfs_stale_read,directfs 0 0
none /dev tmpfs rw,mode=0755 0 0
none /sys sysfs ro,noexec,nosuid,dentry_cache_limit=1000 0 0
none /proc proc rw,dentry_cache_limit=1000 0 0
none /dev/pts devpts rw 0 0
none /dev/shm tmpfs rw,noexec,nosuid,mode=1777 0 0
none /sys/fs/cgroup tmpfs rw,noexec,nosuid 0 0
none /sys/fs/cgroup/cpu cgroup rw,cpu 0 0
none /sys/fs/cgroup/cpuacct cgroup rw,cpuacct 0 0
none /sys/fs/cgroup/cpuset cgroup rw,cpuset 0 0
none /sys/fs/cgroup/devices cgroup rw,devices 0 0
none /sys/fs/cgroup/job cgroup rw,job 0 0
none /sys/fs/cgroup/memory cgroup rw,memory 0 0
none /sys/fs/cgroup/pids cgroup rw,pids 0 0
```

### About the 9p Filesystem

The root filesystem uses **9p**, a network filesystem protocol originally developed for Plan 9. It's commonly used for passing through host directories to virtual machines and containers. Key characteristics:

- `trans=fd` - Uses file descriptors for transport
- `cache=remote_revalidating` - Caches with revalidation
- `directfs` - Direct filesystem access mode
- `overlayfs_stale_read` - Handles overlay filesystem scenarios

This enables efficient file sharing with the host while maintaining isolation.

### cgroups Support

Full cgroup v1 support is available for resource control:
- CPU accounting and limits
- CPU sets (core pinning)
- Memory limits
- PID limits
- Device access control

---

## Network Access

### Configuration

| Property | Value |
|----------|-------|
| **Direct network access** | Blocked |
| **Egress method** | HTTP/HTTPS proxy with JWT authentication |
| **Proxy endpoint** | `21.0.0.31:15004` |
| **DNS** | Handled by proxy (no `/etc/resolv.conf`) |

### Network Interface Status

Standard network tools (`ip`, `ifconfig`) are not functional in this environment - network access is abstracted through the proxy layer.

### Allowed Hosts (Whitelist)

Network access is permitted to an extensive list of development-related domains:

**Package Registries:**
- npm: `registry.npmjs.org`, `npmjs.com`, `npmjs.org`
- PyPI: `pypi.org`, `pypi.python.org`, `files.pythonhosted.org`
- RubyGems: `rubygems.org`, `index.rubygems.org`
- Crates.io: `crates.io`, `static.crates.io`, `index.crates.io`
- Maven: `repo.maven.apache.org`, `central.maven.org`, `repo1.maven.org`
- NuGet: `nuget.org`, `api.nuget.org`
- Packagist: `packagist.org`, `repo.packagist.org`
- Hex.pm: `hex.pm`
- CPAN: `cpan.org`, `metacpan.org`
- Go: `proxy.golang.org`, `sum.golang.org`, `pkg.go.dev`
- Conda: `anaconda.com`, `conda.anaconda.org`, `repo.anaconda.com`

**Code Hosting:**
- GitHub: `github.com`, `api.github.com`, `raw.githubusercontent.com`, `gist.github.com`
- GitLab: `gitlab.com`, `registry.gitlab.com`
- Bitbucket: `bitbucket.org`, `api.bitbucket.org`

**Container Registries:**
- Docker Hub: `hub.docker.com`, `registry-1.docker.io`, `auth.docker.io`
- GitHub Container Registry: `ghcr.io`
- Google Container Registry: `gcr.io`, `*.gcr.io`
- AWS ECR: `public.ecr.aws`

**Cloud Providers:**
- AWS: `*.amazonaws.com`, `*.api.aws`
- GCP: `*.googleapis.com`, `cloud.google.com`, `gcloud.google.com`
- Azure: `azure.com`, `portal.azure.com`, `dev.azure.com`

**Documentation & Language Sites:**
- `golang.org`, `rust-lang.org`, `python.org`
- `nodejs.org`, `ruby-lang.org`, `php.net`
- `docs.claude.com`, `claude.ai`

---

## Available Software

### Programming Languages & Runtimes

| Language | Version | Path |
|----------|---------|------|
| **Python** | 3.11.14 | `/usr/local/bin/python3` |
| **Node.js** | 22.22.0 | `/opt/node22/bin/node` |
| **Go** | 1.24.7 | `/usr/local/go/bin/go` |
| **Rust** | 1.93.0 (254b59607 2026-01-19) | `/root/.cargo/bin/rustc` |
| **Java** | OpenJDK 21.0.9 | `/usr/lib/jvm/java-21-openjdk-amd64` |
| **Ruby** | 3.3.6 (2024-11-05) | `/usr/local/bin/ruby` |
| **PHP** | 8.4.17 (cli) | `/usr/bin/php` |
| **Perl** | 5.38.2 | `/usr/bin/perl` |
| **Bun** | 1.3.6 | `/root/.bun/bin/bun` |

### Java Details

```
openjdk version "21.0.9" 2025-10-21
OpenJDK Runtime Environment (build 21.0.9+10-Ubuntu-124.04)
OpenJDK 64-Bit Server VM (build 21.0.9+10-Ubuntu-124.04, mixed mode, sharing)
```

### Build Tools & Compilers

| Tool | Version | Path |
|------|---------|------|
| **GCC** | 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04) | `/usr/bin/gcc` |
| **G++** | 13.3.0 | `/usr/bin/g++` |
| **CMake** | Available | `/usr/bin/cmake` |
| **Make** | Available | `/usr/bin/make` |
| **Cargo** | (bundled with Rust) | `/root/.cargo/bin/cargo` |

### Package Managers

| Manager | Type | Path |
|---------|------|------|
| **npm** | Node.js | `/opt/node22/bin/npm` |
| **pnpm** | Node.js | Global |
| **yarn** | Node.js | Global |
| **pip3** | Python | `/usr/bin/pip3` |
| **cargo** | Rust | `/root/.cargo/bin/cargo` |
| **apt/apt-get** | System | `/usr/bin/apt` |
| **dpkg** | System (Debian 1.22.6) | `/usr/bin/dpkg` |

### Global Node.js Packages

```
/opt/node22/lib
├── @anthropic-ai/claude-code@2.1.19
├── chromedriver@144.0.0
├── corepack@0.34.0
├── eslint@9.39.2
├── http-server@14.1.1
├── nodemon@3.1.11
├── npm@10.9.4
├── playwright@1.56.1
├── pnpm@10.28.1
├── prettier@3.8.1
├── serve@14.2.5
├── ts-node@10.9.2
├── typescript@5.9.3
└── yarn@1.22.22
```

### Pre-installed Python Packages

| Package | Version | Description |
|---------|---------|-------------|
| requests | 2.32.5 | HTTP library |
| PyYAML | 6.0.1 | YAML parser |
| Jinja2 | 3.1.6 | Template engine |
| cryptography | 41.0.7 | Cryptographic recipes |
| conan | 2.24.0 | C/C++ package manager |
| certifi | 2026.1.4 | CA certificates |
| packaging | 24.0 | Python packaging utilities |
| python-dateutil | 2.9.0.post0 | Date utilities |

### Version Control & CLI Tools

| Tool | Version |
|------|---------|
| **Git** | 2.43.0 |
| **curl** | Available |
| **wget** | Available |

---

## System Limits (ulimit)

| Resource | Limit |
|----------|-------|
| Real-time non-blocking time | Unlimited |
| Core file size | Unlimited |
| Data segment size | Unlimited |
| Scheduling priority | 0 |
| File size | Unlimited |
| Pending signals | 0 |
| Max locked memory | 64 KB |
| Max memory size | Unlimited |
| **Open files** | **20,000** |
| Pipe size | 8 × 512 bytes |
| POSIX message queues | 819,200 bytes |
| Real-time priority | 0 |
| **Stack size** | **8 MB** |
| CPU time | Unlimited |
| Max user processes | Unlimited |
| Virtual memory | Unlimited |
| File locks | Unlimited |

---

## Environment Characteristics

### Sandbox Properties

Key environment variables indicating sandbox status:

```bash
IS_SANDBOX=yes
CLAUDECODE=1
CLAUDE_CODE_REMOTE=true
CLAUDE_CODE_ENTRYPOINT=remote
CLAUDE_CODE_REMOTE_ENVIRONMENT_TYPE=cloud_default
CLAUDE_CODE_VERSION=2.1.19
```

### User Context

- Running as **root** user (UID 0) - common in container environments
- Home directory: `/root`
- Working directory: `/home/user/vibes`

### Notable Constraints

1. **No Docker/Podman** - Container tools are not available inside the sandbox
2. **No direct internet access** - All traffic must go through the authenticated proxy
3. **No network interface tools** - `ip`/`ifconfig` are not functional
4. **gVisor syscall limitations** - Some low-level operations may behave differently than on bare metal Linux
5. **No interactive terminal features** - `GIT_EDITOR=true` disables interactive editors

### Environment Paths

```bash
JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
MAVEN_HOME=/opt/maven
GRADLE_HOME=/opt/gradle
BUN_INSTALL=/root/.bun
```

---

## Implementation

Data was gathered using standard Linux commands:

| Command | Purpose |
|---------|---------|
| `uname -a` | Kernel and architecture info |
| `cat /etc/os-release` | OS distribution details |
| `cat /proc/cpuinfo` | CPU specifications and flags |
| `lscpu` | CPU topology |
| `free -h` | Memory information |
| `df -h` | Disk space usage |
| `cat /proc/mounts` | Mounted filesystems |
| `env` | Environment variables |
| `ulimit -a` | System resource limits |
| `which <cmd>` | Software availability |
| `<cmd> --version` | Software versions |
| `pip3 list` | Python packages |
| `npm list -g` | Global Node.js packages |

---

## Use Cases

This information is useful for:

1. **Understanding capabilities** - Knowing what languages and tools are pre-installed
2. **Debugging** - Understanding why certain operations might fail (e.g., no Docker, restricted network)
3. **Performance optimization** - Leveraging available CPU features (AVX-512, AMX for ML workloads)
4. **Security awareness** - Understanding the isolation model and trust boundaries
5. **Development planning** - Knowing resource limits (20k open files, 8MB stack, 21GB RAM)

---

## Links

- [gVisor Documentation](https://gvisor.dev/) - The secure container runtime
- [9p Protocol](https://en.wikipedia.org/wiki/9P_(protocol)) - The filesystem protocol used
- [Claude Code](https://claude.ai/code) - The AI coding assistant
- [AVX-512 Documentation](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/) - Intel intrinsics guide
- [AMX Overview](https://www.intel.com/content/www/us/en/products/docs/accelerator-engines/advanced-matrix-extensions/overview.html) - Advanced Matrix Extensions
