# KeetScript VS Code Extension 🐱

Visual Studio Code extension providing dedicated language support, syntax highlighting, autocompletion snippets, and live compiler diagnostics for **KeetScript** (`.keet`) Single-File Components (SFCs) and full-stack applications.

---

## Features

- **🎨 Syntax Highlighting:**
  - Full-stack `<server>` section: SQLite `database`, `table`, schema constraints (`primary_key`, `auto_increment`), REST `backend`, HTTP route handlers (`get`, `post`, `put`, `delete`), and `frontend` servers.
  - Component `<script>` section: reactive `state`, constants `let`, functions `def`, and native C++ `import cpp` bridges.
  - Declarative `<template>` section: HTML tags, `{interpolations}`, two-way bindings (`bind:value`, `bind:checked`), inline event lambdas (`onClick`), and Pythonic control flow (`if:`, `elif:`, `else:`, `for item in items:`).
  - Scoped `<style>` section: CSS properties and selectors.

- **⚡ Autocompletion & Snippets:**
  - `keetapp`: Scaffold a complete Full-Stack single-file application with `<server>`, `<script>`, `<template>`, and `<style>`.
  - `server`, `database`, `table`, `backend`, `frontend`, `get`, `post`.
  - `state`, `let`, `def`, `import cpp`.
  - `bind:value`, `bind:checked`, `onClick`.

- **🔍 Live Compiler Diagnostics & Linting:**
  - Automatically runs the `keety --lint` compiler on save and document open.
  - Highlights lexical and syntactic errors directly in the VS Code editor problem panel.

- **🚀 Ahead-of-Time (AOT) C++20 Compilation Commands:**
  - **Keet: Compile Component to C++**: Transpiles `.keet` into standard C++20 `Component.h` and `Component.cpp`.
  - **Keet: Compile Full-Stack Application (--app)**: Generates a self-contained C++ application entry point (`main.cpp`) initializing the database, backend, and frontend worker threads.

---

## Extension Settings

This extension contributes the following settings:

* `keetscript.compilerPath`: Absolute path to the `keety` compiler executable. If omitted, the extension automatically discovers `keety` inside standard workspace build directories (e.g. `cmake-build-debug/keety` or `build/keety`).

---

## Installation

1. Download the `.vsix` package:
   ```bash
   cd tooling/vscode-plugin
   npm run compile
   npx @vscode/vsce package --no-dependencies
   ```
2. Install in VS Code:
   ```bash
   code --install-extension vscode-keetscript-1.0.0.vsix
   ```
