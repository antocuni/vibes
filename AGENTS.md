# Guidelines for AI Coding Assistants

This repository is a workspace for vibe coding - quick experiments, explorations, and AI-assisted projects.

## Repository structure

- Each subdirectory is an independent project
- Each project must have its own README.md
- The main README.md is auto-generated from project directories
- Projects are sorted by date (most recent first)

## When working on a new task

When asked to work on a new project or exploration:

1. **Create a dedicated directory** for the project with a descriptive name
   - Use lowercase-with-hyphens naming (e.g., `async-file-processor`, `sqlite-experiments`, `web-scraper`)

2. **Write a comprehensive README.md** in the project directory explaining:
   - What the project does or explores
   - What problem it solves or question it answers
   - Key findings, learnings, or outcomes
   - How to use or run the code (if applicable)
   - Any relevant links to documentation, libraries, or related projects

3. **Write the code and any other files** needed for the project in that directory

4. **Keep the project self-contained** - all code, data files (within reason), and documentation for a project should live in its directory

## Project README requirements

Each project's README.md should include:

- **Clear title and description** (h1) - what does this project do?
- **Motivation** - what problem does it solve or question does it answer?
- **Implementation details** - what technologies, libraries, or approaches were used?
- **Key findings or results** - what was learned or discovered?
- **Usage instructions** - how to run or use the code (if applicable)
- **Links** - to relevant documentation, libraries, or related projects

Write READMEs as if explaining to someone (including your future self) who wants to understand what was done and why.

### Example README structure

```markdown
# Project Name

Brief description of what this project does or explores.

## What was explored

Description of the problem, question, or idea that motivated this project.

## Implementation

Key details about how it works, what technologies were used, interesting discoveries.

## Key findings

- Finding or learning 1
- Finding or learning 2
- Finding or learning 3

## Usage

How to run or use the code (if applicable).
```

## What to include in projects

- Source code and scripts
- Documentation and notes
- Small data files or examples (< 2MB)
- Configuration files
- Test files

## What to avoid

- Don't copy entire external repositories - link to them instead
- Don't commit large binary files
- Don't include build artifacts or dependency directories (virtual environments, node_modules, etc.)
- Don't update the main README.md manually (it's auto-generated)

## Summary generation

A GitHub Actions workflow automatically:
- Scans all project directories
- Generates or uses cached summaries using the `llm` CLI tool
- Updates the main README.md with a sorted project list (most recent first)

Each project can optionally include a `_summary.md` file to cache its summary and avoid regeneration.

You don't need to manually update the main README.md - just ensure each project has a good README.md in its directory.

## Best practices

- **Be specific** - vague descriptions aren't helpful
- **Document decisions** - explain why certain approaches were chosen
- **Include examples** - show how to use the code
- **Link to resources** - cite documentation, tutorials, or related work
- **Keep it focused** - each project should explore one main idea or solve one problem
