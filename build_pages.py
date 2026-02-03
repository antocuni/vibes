#!/usr/bin/env python3
"""Build GitHub Pages site from README files.

Converts the main README.md and each project's README.md to HTML pages.
For projects that contain HTML apps (index.html), copies the app files
and adds a link to the live app on the project page.
"""

import os
import re
import shutil
import pathlib
import subprocess

import markdown

SITE_DIR = pathlib.Path("_site")
REPO_DIR = pathlib.Path(".")

# Files/dirs to skip when copying app files
SKIP_FILES = {"README.md", "_summary.md", ".git", "__pycache__", "node_modules"}

# Extensions that indicate an HTML app
APP_INDICATOR = "index.html"

CSS = """\
:root {
    --color-bg: #ffffff;
    --color-text: #24292f;
    --color-link: #0969da;
    --color-border: #d0d7de;
    --color-code-bg: #f6f8fa;
    --color-header-bg: #f6f8fa;
    --color-app-btn: #2da44e;
    --color-app-btn-hover: #218838;
}

* { box-sizing: border-box; }

body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Noto, Helvetica, Arial, sans-serif;
    line-height: 1.6;
    color: var(--color-text);
    background: var(--color-bg);
    margin: 0;
    padding: 0;
}

.container {
    max-width: 800px;
    margin: 0 auto;
    padding: 2rem 1.5rem;
}

nav {
    margin-bottom: 2rem;
    padding-bottom: 1rem;
    border-bottom: 1px solid var(--color-border);
    font-size: 0.9rem;
}

nav a {
    color: var(--color-link);
    text-decoration: none;
}

nav a:hover {
    text-decoration: underline;
}

article h1 { font-size: 2rem; border-bottom: 1px solid var(--color-border); padding-bottom: 0.3em; }
article h2 { font-size: 1.5rem; border-bottom: 1px solid var(--color-border); padding-bottom: 0.3em; margin-top: 1.5em; }
article h3 { font-size: 1.25rem; margin-top: 1.5em; }

a { color: var(--color-link); text-decoration: none; }
a:hover { text-decoration: underline; }

code {
    background: var(--color-code-bg);
    padding: 0.2em 0.4em;
    border-radius: 6px;
    font-size: 0.85em;
    font-family: ui-monospace, SFMono-Regular, "SF Mono", Menlo, Consolas, "Liberation Mono", monospace;
}

pre {
    background: var(--color-code-bg);
    padding: 1rem;
    border-radius: 6px;
    overflow-x: auto;
    line-height: 1.45;
}

pre code {
    background: none;
    padding: 0;
    border-radius: 0;
    font-size: 0.85em;
}

ul, ol { padding-left: 2em; }
li { margin: 0.25em 0; }

hr {
    border: none;
    border-top: 1px solid var(--color-border);
    margin: 2rem 0;
}

table {
    border-collapse: collapse;
    width: 100%;
}

th, td {
    border: 1px solid var(--color-border);
    padding: 0.5em 1em;
    text-align: left;
}

th {
    background: var(--color-header-bg);
}

.app-link {
    display: inline-block;
    margin: 1rem 0;
    padding: 0.6rem 1.2rem;
    background: var(--color-app-btn);
    color: #fff;
    border-radius: 6px;
    font-weight: 600;
    text-decoration: none;
    font-size: 1rem;
}

.app-link:hover {
    background: var(--color-app-btn-hover);
    text-decoration: none;
}

.project-list a {
    font-weight: 600;
}

footer {
    margin-top: 3rem;
    padding-top: 1rem;
    border-top: 1px solid var(--color-border);
    font-size: 0.8rem;
    color: #656d76;
}
"""


def render_html(title, body_html, nav_html="", extra_head=""):
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title}</title>
    <style>{CSS}</style>
    {extra_head}
</head>
<body>
    <div class="container">
        {nav_html}
        <article>
{body_html}
        </article>
        <footer>
            Built automatically from project README files.
        </footer>
    </div>
</body>
</html>"""


def md_to_html(md_text):
    """Convert markdown text to HTML."""
    extensions = [
        "fenced_code",
        "tables",
        "toc",
        "attr_list",
        "codehilite",
    ]
    extension_configs = {
        "codehilite": {"css_class": "highlight", "guess_lang": False},
    }
    md = markdown.Markdown(extensions=extensions, extension_configs=extension_configs)
    return md.convert(md_text)


def strip_cog_directives(text):
    """Remove cogapp directives from markdown text.

    Keeps the generated content between ]]]-->  and <!--[[[end]]]-->.
    """
    # Remove cog start blocks: <!--[[[cog ... ]]]-->
    text = re.sub(r"<!--\[\[\[cog.*?\]\]\]-->", "", text, flags=re.DOTALL)
    # Remove cog end markers
    text = text.replace("<!--[[[end]]]-->", "")
    return text


def get_github_repo_url():
    """Get the GitHub HTTPS URL for this repo."""
    try:
        result = subprocess.run(
            ["git", "remote", "get-url", "origin"],
            capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            origin = result.stdout.strip()
            if "github.com" in origin or "git/" in origin:
                # Handle SSH URLs
                if origin.startswith("git@github.com:"):
                    origin = origin.replace("git@github.com:", "https://github.com/")
                if origin.endswith(".git"):
                    origin = origin[:-4]
                # Extract owner/repo from proxy URLs too
                m = re.search(r"([^/]+/[^/]+)$", origin)
                if m:
                    return f"https://github.com/{m.group(1)}"
    except Exception:
        pass
    return None


def get_pages_base_url():
    """Determine the base URL for GitHub Pages."""
    repo_url = get_github_repo_url()
    if repo_url:
        # e.g., https://github.com/antocuni/vibes -> https://antocuni.github.io/vibes
        parts = repo_url.rstrip("/").split("/")
        if len(parts) >= 2:
            owner = parts[-2]
            repo = parts[-1]
            return f"https://{owner}.github.io/{repo}"
    return ""


def get_project_dirs():
    """Get list of project directories sorted by most recent commit first."""
    dirs = []
    for d in REPO_DIR.iterdir():
        if d.is_dir() and not d.name.startswith(".") and not d.name.startswith("_"):
            readme = d / "README.md"
            if readme.exists():
                dirs.append(d.name)
    return sorted(dirs)


def has_html_app(project_dir):
    """Check if a project directory contains an HTML app."""
    return (REPO_DIR / project_dir / APP_INDICATOR).exists()


def copy_app_files(project_dir, dest_dir):
    """Copy app files from a project directory to the site, excluding docs."""
    src = REPO_DIR / project_dir
    dst = dest_dir / "app"
    dst.mkdir(parents=True, exist_ok=True)

    for item in src.iterdir():
        if item.name in SKIP_FILES:
            continue
        if item.is_file():
            shutil.copy2(item, dst / item.name)
        elif item.is_dir() and not item.name.startswith("."):
            shutil.copytree(item, dst / item.name, dirs_exist_ok=True)


def rewrite_project_links(html_content, projects):
    """Rewrite links to project directories to point to project pages."""
    for project in projects:
        # Replace GitHub tree/main links with relative page links
        html_content = re.sub(
            rf'href="https://github\.com/[^"]+/tree/main/{re.escape(project)}"',
            f'href="{project}/"',
            html_content,
        )
    return html_content


def build():
    """Build the GitHub Pages site."""
    print("Building GitHub Pages site...")

    # Clean and create site directory
    if SITE_DIR.exists():
        shutil.rmtree(SITE_DIR)
    SITE_DIR.mkdir()

    projects = get_project_dirs()
    print(f"Found {len(projects)} project(s): {', '.join(projects)}")

    # Build main index.html
    readme_path = REPO_DIR / "README.md"
    if readme_path.exists():
        readme_text = readme_path.read_text()
        readme_text = strip_cog_directives(readme_text)
        html_content = md_to_html(readme_text)
        html_content = rewrite_project_links(html_content, projects)

        index_html = render_html("vibes", html_content)
        (SITE_DIR / "index.html").write_text(index_html)
        print("Built index.html")

    # Build project pages
    for project in projects:
        project_dir = SITE_DIR / project
        project_dir.mkdir(parents=True, exist_ok=True)

        project_readme = REPO_DIR / project / "README.md"
        if project_readme.exists():
            readme_text = project_readme.read_text()
            html_content = md_to_html(readme_text)

            # If this project has an HTML app, add a prominent link
            app_banner = ""
            if has_html_app(project):
                app_banner = '<p><a class="app-link" href="app/">Launch App</a></p>\n'
                copy_app_files(project, project_dir)
                print(f"  Copied app files for {project}")

            nav = '<nav><a href="../">← Back to all projects</a></nav>'
            body = app_banner + html_content

            page_html = render_html(project, body, nav_html=nav)
            (project_dir / "index.html").write_text(page_html)
            print(f"Built {project}/index.html")

    # Copy .nojekyll to prevent Jekyll processing
    (SITE_DIR / ".nojekyll").write_text("")

    print(f"\nSite built in {SITE_DIR}/")


if __name__ == "__main__":
    build()
