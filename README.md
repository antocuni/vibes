# vibes

**WARNING: this is almost 100% AI SLOP**

Most of the text and code in this repository has been written by AI assistants. This sentence has been written by a human :).

---

A repository for vibe coding experiments and AI-assisted explorations.

Each directory in this repo is a separate project or exploration carried out with the help of AI coding assistants like [Claude Code](https://www.claude.com/product/claude-code). The work may include experiments, prototypes, research, or learning exercises.

When working on a new project, create a directory for it and include a README.md explaining what the project does, what was explored, or what was learned.

See [CLAUDE.md](CLAUDE.md) for instructions on how Claude should work with this repository, and [AGENTS.md](AGENTS.md) for general guidelines for AI coding assistants.

Inspired by [simonw/research](https://github.com/simonw/research).

<!--[[[cog
import os
import re
import subprocess
import pathlib
from datetime import datetime, timezone

# Model to use for generating summaries
MODEL = "github/gpt-4.1"

# Get all subdirectories with their first commit dates
repo_dir = pathlib.Path.cwd()
subdirs_with_dates = []

for d in repo_dir.iterdir():
    if d.is_dir() and not d.name.startswith('.'):
        # Get the date of the first commit that touched this directory
        try:
            result = subprocess.run(
                ['git', 'log', '--diff-filter=A', '--follow', '--format=%aI', '--reverse', '--', d.name],
                capture_output=True,
                text=True,
                timeout=5
            )
            if result.returncode == 0 and result.stdout.strip():
                # Parse first line (oldest commit)
                date_str = result.stdout.strip().split('\n')[0]
                commit_date = datetime.fromisoformat(date_str.replace('Z', '+00:00'))
                subdirs_with_dates.append((d.name, commit_date))
            else:
                # No git history, use directory modification time
                subdirs_with_dates.append((d.name, datetime.fromtimestamp(d.stat().st_mtime, tz=timezone.utc)))
        except Exception:
            # Fallback to directory modification time
            subdirs_with_dates.append((d.name, datetime.fromtimestamp(d.stat().st_mtime, tz=timezone.utc)))

# Print the heading with count
if subdirs_with_dates:
    n = len(subdirs_with_dates)
    label = "project" if n == 1 else "projects"
    print(f"## {n} {label}\n")

    # Sort by date, most recent first
    subdirs_with_dates.sort(key=lambda x: x[1], reverse=True)

    for dirname, commit_date in subdirs_with_dates:
        folder_path = repo_dir / dirname
        readme_path = folder_path / "README.md"
        summary_path = folder_path / "_summary.md"

        date_formatted = commit_date.strftime('%Y-%m-%d')

        # Get GitHub repo URL
        github_url = None
        try:
            result = subprocess.run(
                ['git', 'remote', 'get-url', 'origin'],
                capture_output=True,
                text=True,
                timeout=2
            )
            if result.returncode == 0 and result.stdout.strip():
                origin = result.stdout.strip()
                # Convert SSH URL to HTTPS URL for GitHub
                if origin.startswith('git@github.com:'):
                    origin = origin.replace('git@github.com:', 'https://github.com/')
                if origin.endswith('.git'):
                    origin = origin[:-4]
                github_url = f"{origin}/tree/main/{dirname}"
        except Exception:
            pass

        if github_url:
            print(f"### [{dirname}]({github_url}) ({date_formatted})\n")
        else:
            print(f"### {dirname} ({date_formatted})\n")

        # Check if summary already exists
        if summary_path.exists():
            # Use cached summary
            with open(summary_path, 'r') as f:
                description = f.read().strip()
                if description:
                    print(description)
                else:
                    print("*No description available.*")
        elif readme_path.exists():
            # Generate new summary using llm command
            prompt = """Summarize this project in 2-3 lines maximum. Be specific and concise - no bullet lists, no links, no emoji. Just a brief plain-text description of what the project is and does."""
            result = subprocess.run(
                ['llm', '-m', MODEL, '-s', prompt],
                stdin=open(readme_path),
                capture_output=True,
                text=True,
                timeout=60
            )
            if result.returncode != 0:
                error_msg = f"LLM command failed for {dirname} with return code {result.returncode}"
                if result.stderr:
                    error_msg += f"\nStderr: {result.stderr}"
                raise RuntimeError(error_msg)
            if result.stdout.strip():
                description = result.stdout.strip()
                print(description)
                # Save to cache file
                with open(summary_path, 'w') as f:
                    f.write(description + '\n')
            else:
                raise RuntimeError(f"LLM command returned no output for {dirname}")
        else:
            print("*No description available.*")

        print()  # Add blank line between entries
else:
    print("No projects yet. Create a directory with a README.md to get started!\n")
]]]-->
## 6 projects

### [minimal-reminders-app](https://github.com/antocuni/vibes/tree/main/minimal-reminders-app) (2026-02-18)

A minimal Android reminders app (21KB APK) built with pure Java and zero dependencies. Supports recurring reminders with day-of-week selection, notifications with dismiss/snooze actions, and survives device reboots.

### [android-reminders](https://github.com/antocuni/vibes/tree/main/android-reminders) (2026-02-18)

Minimal Android reminders app built with zero frameworks — pure Java, bare SDK tools, ~29KB APK. Supports recurring alarms with day-of-week selection, notifications with dismiss/snooze, and persists across reboots.

### [wasi-posix-sockets](https://github.com/antocuni/vibes/tree/main/wasi-posix-sockets) (2026-02-07)

POSIX socket functions exposed as WASM imports, with a Python/wasmtime host providing real socket implementations. Includes HTTP client, echo server, and a libcurl-compatible API demo, all running as WASM modules with networking delegated to the host.

### [wasi-http-requests](https://github.com/antocuni/vibes/tree/main/wasi-http-requests) (2026-02-07)

Working examples of making HTTP GET and POST requests from C compiled to WASI, run with wasmtime. Uses wasi:http component model with wit-bindgen C bindings, since libcurl and POSIX sockets cannot yet be compiled to WASI.

### [system-environment-report](https://github.com/antocuni/vibes/tree/main/system-environment-report) (2026-02-05)

An exploration of Anthropic's Claude Code Remote environment: a gVisor-sandboxed Ubuntu 24.04 container with 16 CPU cores, 21 GB RAM, and pre-installed dev tools (Python, Node.js, Go, Rust, Java), with network access restricted through an authenticated proxy.

### [snake-game](https://github.com/antocuni/vibes/tree/main/snake-game) (2026-02-02)

A browser-based Snake game built with vanilla HTML, CSS, and JavaScript using canvas rendering. Features localStorage high scores, smooth controls, and no dependencies.

<!--[[[end]]]-->

---

## Updating this README

This README uses [cogapp](https://nedbatchelder.com/code/cog/) to automatically generate project descriptions.

### Automatic updates

A GitHub Action automatically runs `cog -r -P README.md` on every push to main and commits any changes to the README or new `_summary.md` files.

### Manual updates

To update locally:

```bash
# Run cogapp to regenerate the project list
cog -r -P README.md
```

The script automatically:
- Discovers all subdirectories in this folder
- Gets the first commit date for each folder and sorts by most recent first
- For each folder, checks if a `_summary.md` file exists
- If the summary exists, it uses the cached version
- If not, it generates a new summary using `llm -m github/gpt-4.1` with a prompt that creates short 2-3 line descriptions
- Creates markdown links to each project folder on GitHub
- New summaries are saved to `_summary.md` to avoid regenerating them on every run

To regenerate a specific project's description, delete its `_summary.md` file and run `cog -r -P README.md` again.
