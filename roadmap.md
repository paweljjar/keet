# Keet & Cattery Development Roadmap 🦜🐱🐾

An architectural master plan for **Keet**, **KeetScript**, and the **Cattery** native package ecosystem.

---

## 1. Executive Strategy & Core Architectural Priorities 🎯

### Repository Topology: Dual-Repository Architecture 🌐
Keet is partitioned cleanly across two dedicated repositories:
1. **`keet` (Language & Compiler Repository):** Houses the KeetScript language specifications, AST & intermediate representation (K-IR), the `keety` AOT compiler, the `cattery` CLI utility, and the ultra-lean `libkeet` baseline runtime.
2. **`cattery` (Standard Packages Monorepo Repository):** A dedicated, separate repository hosting all official standard packages (`packages/cattery-http-server`, `packages/cattery-uiengine`, `packages/cattery-sqlite`, `packages/cattery-router`, `packages/cattery-auth`, etc.). The `cattery` CLI downloads packages directly from this remote monorepo into the local project `.cattery/packages/` workspace cache.

---

### Top 1 Priority: KeetScript as an Independent Compiled Language & Base Runtime (`libkeet`)
- **Language Evolution:** Transition KeetScript from an embedded Single-File Component (SFC) DSL into a distinct, general-purpose standalone compiled systems language targeting high-performance, idiomatic C++20. Developers can write pure modules, algorithms, models, and CLI tools with zero web/HTML overhead.
- **AOT Compilation Pipeline:** Enhance the `keety` compiler with multi-pass syntactic analysis, compile-time static type checking, borrow checking, and clean C++20 header/source emission (`keety --lib`).
- **Minimal Core Runtime (`libkeet`):** `libkeet` is strictly the language baseline runtime (akin to `libc`/`libcore`), providing only the essential primitives: basic I/O streams (`iostream`), fundamental math & numeric operations, memory management/bump allocators, core strings, collections, and error handling. **It carries zero web, HTTP, VDOM, or database dependencies.**

### Top 2 Priority: Cattery Package CLI & Remote Package Downloader (`cattery`)
- **Cattery CLI (`cattery init`, `cattery add`):** Implement the native command-line utility to scaffold projects, resolve dependencies, and download adopted packages directly from the remote Cattery monorepo repository.
- **Remote Package Acquisition:** When running `cattery add cattery-sqlite`, the CLI downloads the package from the remote Cattery packages monorepo, validates integrity, caches it inside `.cattery/packages/cattery-sqlite`, and records the pinned version in `pedigree.json`.
- **Project Manifest (`pedigree.json`):** Declarative project manifest tracking project identity, compilation targets (CLI, server, desktop), compiler flags, and adopted Cattery packages.
- **Isolated Build Cache (`.cattery/`):** A toolchain-managed hidden directory housing intermediate transpiled C++ headers, implementation units, adopted package checkouts, and generated CMake configurations, keeping user repositories clean.

### Top 3 Priority: Framework Decoupling into the Cattery Packages Monorepo
- **Modular Framework Packages:** Because `libkeet` per se does not need HTTP or UI, all web, storage, and networking capabilities are decoupled out of core and published into the separate Cattery packages monorepo:
  - **`cattery-http-server`:** High-performance concurrent HTTP/1.1 and WebSocket networking engine.
  - **`cattery-uiengine`:** Server-driven Virtual DOM differential streaming, component lifecycle, and template rendering.
  - **`cattery-sqlite`:** Embedded SQLite 3 database engine with atomic ACID durability and `cattery evolve` schema auto-migrations.
  - **`cattery-router`:** Declarative multi-page routing and URL state synchronization.
  - **`cattery-auth`:** Cryptographic session management, OS CSPRNG entropy, and declarative `policy` edge security blocks.
- **Compose Only What You Need:** A CLI utility or embedded app links only `libkeet`; a full-stack web application adopts packages on demand:
  ```bash
  cattery add cattery-http-server cattery-uiengine cattery-sqlite
  ```

---

## 2. Ecosystem Nomenclature & World-Building 🐾

Keet combines bare-metal C++20 performance with expressive, modular ergonomics through a unified animal-themed tooling suite:

- **🦜 `libkeet` (Language Base Runtime):** The minimal, zero-web language runtime for KeetScript. Provides fundamental I/O streams, basic math, memory management, core strings, and collection primitives in a sub-500KB footprint.
- **🐱 `keety` ("little cat"):** The ahead-of-time (AOT) compiler transpiling `.keet` standalone language modules, library packages (`keety --lib`), and UI component Single-File Components into optimized C++20 header/source pairs.
- **📦 `cattery` (Packages Monorepo & CLI):** Both the dedicated packages monorepo repository hosting all official standard libraries, and the CLI utility where developers **"adopt"** modular packages (`cattery add <pkg>`) which are fetched and cached into `.cattery/packages/`.
- **📜 `pedigree.json`:** The declarative project manifest recording project identity, compilation targets, compiler flags, and adopted Cattery dependencies.
- **📁 `.cattery/`:** The toolchain-managed build cache workspace isolating generated C++ translation units, adopted packages, and CMake build artifacts.
- **🐈📦 `litterbox`:** A lightweight, dedicated developer workspace environment and LSP daemon host tailored for KeetScript projects.
- **🐈💨 `Zoomies` (`cattery dev`):** The lightning-fast local development watcher recompiling modified source units and pushing surgical VDOM patches over WebSockets in under 50ms without clearing session state.
- **🏠🐱 `dmstct-keety` ("domesticate kitty" / `cattery build --target desktop`):** The native desktop application packager linking lightweight OS system webviews (WebKit, WebView2, WebKitGTK) directly to Keet's native C++ core (<5MB binaries, <10MB RAM).

---

## 3. Completed Milestones ✅

### Feature 1: Scoped Component Styles & Automatic Style Harvesting [Completed ✅]
- **Description:** Automated tree traversal to harvest styles from rendered component hierarchies during render passes, alongside automatic CSS class scoping (`data-keet-scope="ComponentName#ID"`) to prevent style leakage across components.
- **Phases:**
    - **Phase 1: Automatic Tree-Wide Style Harvesting** [Completed ✅]
        - 1.1: Recursive VNode tree traversal in `UIEngine` collecting declared `StyleSheet` instances during render passes. [Completed ✅]
        - 1.2: Dynamic merging and caching of collected rules into `/styles.css` without manual parent `merge()` calls. [Completed ✅]
        - 1.3: Deprecation of manual style merging in parent nodes with backward compatibility for explicit overrides. [Completed ✅]
    - **Phase 2: CSS Scoping & Isolation Engine** [Completed ✅]
        - 2.1: Automatic scope attribute tagging (`data-keet-scope`) on component root VNodes. [Completed ✅]
        - 2.2: Rewriting and namespacing selector rules during CSS serialization strictly within component scope. [Completed ✅]
        - 2.3: Support for conditional class directives (`class:active="isActive"`) in VNode attribute maps. [Completed ✅]

---

### Feature 2: Built-in Internationalization & Localization (i18n) [Completed ✅]
- **Description:** Out-of-the-box localization subsystem managing thread-safe JSON translation catalogs, contextual parameter interpolation (`__("welcome", {{"name", "User"}})`), dynamic plural rules, and automatic locale detection via `Accept-Language` headers and cookies.
- **Phases:**
    - **Phase 1: Core Localization Engine (`keet/Localization.h`)** [Completed ✅]
        - 1.1: Singleton `Localization` engine parsing JSON translation catalogs (`locales/en.json`, `locales/pl.json`). [Completed ✅]
        - 1.2: Translation lookup with fallback language negotiation and parameter replacement. [Completed ✅]
        - 1.3: Ergonomic global translation helper (`__("key")`) accessible directly in component `render()` methods. [Completed ✅]
    - **Phase 2: Session Integration & Dynamic Language Switching** [Completed ✅]
        - 2.1: Locale detection from HTTP `Accept-Language` and session cookie. [Completed ✅]
        - 2.2: Active locale stored within `Session`, synchronized under session mutex. [Completed ✅]
        - 2.3: Immediate VNode tree re-rendering upon locale switch without full-page reloads. [Completed ✅]

---

### Feature 3: Dedicated KeetScript DSL & `keety` AOT Compiler 🐱 [Completed ✅]
- **Description:** Single-File Component (SFC) format combining reactive UI, Pythonic control flow, scoped `<style>`, and full-stack `<server>` database/routing declarations with the `keety` AOT compiler transpiling `.keet` into standard C++20.
- **Phases:**
    - **Phase 1: KeetScript Grammar & AST Definition** [Completed ✅]
        - 1.1: `.keet` format specification with `<script>`, `<template>`, `<style>`, and `<server>` blocks. [Completed ✅]
        - 1.2: Two-way data binding (`bind:value`, `bind:checked`) and event expressions (`onClick=() => count += 1`). [Completed ✅]
        - 1.3: Native C++ header bridge syntax (`import cpp from "Header.h" as Module`). [Completed ✅]
        - 1.4: `<server>` grammar: database tables (`primary_key`, `auto_increment`), backend REST routes, and frontend mounts. [Completed ✅]
    - **Phase 2: `keety` Compiler CLI Implementation & App Mode** [Completed ✅]
        - 2.1: Lexer, parser, and code generator for the standalone `keety` CLI. [Completed ✅]
        - 2.2: C++20 code generator emitting `UIEngine::Component` subclasses and pre-allocated child vectors. [Completed ✅]
        - 2.3: Application Mode (`keety --app`) generating standalone `main.cpp` with route dispatchers and worker threads. [Completed ✅]
        - 2.4: Seamless CMake integration via `keet_target_sources()` and `keet_add_application()`. [Completed ✅]
    - **Phase 3: IDE Tooling & Language Support** [Completed ✅]
        - 3.1: TextMate grammar bundle (`tooling/textmate`) for syntax highlighting. [Completed ✅]
        - 3.2: Visual Studio Code extension (`tooling/vscode-plugin`) with snippets, live `keety --lint`, and one-click build. [Completed ✅]

---

### Feature 4: WebSocket Transport & Real-Time Push Engine [Completed ✅]
- **Description:** Persistent, bidirectional WebSocket transport layer alongside HTTP POST polling. Eliminates connection handshake latency, reduces packet overhead, and enables proactive server push from C++ background worker threads.
- **Phases:**
    - **Phase 1: WebSocket Protocol Layer in `Server`** [Completed ✅]
        - 1.1: RFC 6455 WebSocket handshaking (`Upgrade: websocket`) and frame parsing inside `HTTPServer::Server`. [Completed ✅]
        - 1.2: Persistent socket connections attached to active worker threads and non-blocking event polling. [Completed ✅]
        - 1.3: Binary/text frame masking, fragmentation handling, and graceful connection closure. [Completed ✅]
    - **Phase 2: Client Runtime & Server-Push Integration** [Completed ✅]
        - 2.1: Embedded JavaScript runtime upgraded in `buildHtmlShell()` to connect via WebSocket with HTTP POST fallback. [Completed ✅]
        - 2.2: Client event routing over persistent WebSockets, streaming surgical VNode JSON patches back instantly. [Completed ✅]
        - 2.3: Server-push API (`Session::pushUpdate()`) allowing C++ worker threads to trigger UI patches proactively. [Completed ✅]

---

### Feature 5: Static Asset Serving & Middleware Pipeline [Completed ✅]
- **Description:** Production-ready HTTP middleware chain for request filtering, logging, CORS, and security hardening, combined with a high-throughput static asset file server (`/public/*`) featuring MIME detection and HTTP caching headers.
- **Phases:**
    - **Phase 1: HTTP Middleware Chain** [Completed ✅]
        - 1.1: Composable middleware signatures (`std::function<bool(ServerRequest&, ServerResponse&)>`). [Completed ✅]
        - 1.2: Sequential pipeline execution in `Server::_dispatch()` prior to route matching. [Completed ✅]
        - 1.3: Built-in middlewares for request logging, configurable CORS, and defensive security headers. [Completed ✅]
    - **Phase 2: Static File Server** [Completed ✅]
        - 2.1: Static directory routing for `/public/*` or user-defined asset folders. [Completed ✅]
        - 2.2: Automated MIME-type resolution across file extensions (`.png`, `.svg`, `.js`, `.css`, `.woff2`). [Completed ✅]
        - 2.3: Support for HTTP caching headers (`ETag`, `Cache-Control`, `If-None-Match`) to minimize bandwidth usage. [Completed ✅]

---

### Feature 6: Test Scaffolding & Headless Component Testing Harness (`cattery test`) [Completed ✅]
- **Description:** Automated testing foundation for Keet. Integrates CTest, compiles unit tests under AddressSanitizer (ASan), UndefinedBehaviorSanitizer (UBSan), and ThreadSanitizer (TSan), wires `tools/keety/test_ast.cpp`, and provides a headless native C++ VDOM test harness (`keet/Testing.h`) to verify component rendering and event dispatching without opening network sockets or browser instances.
- **Phases:**
    - **Phase 1: CMake & CTest Scaffolding + Sanitizers** [Completed ✅]
        - 1.1: Enable CTest in root `CMakeLists.txt` and wire `tools/keety/test_ast.cpp` into an executable test target. [Completed ✅]
        - 1.2: Add CMake options for ASan, UBSan, and TSan builds (`ENABLE_ASAN`, `ENABLE_TSAN`, `ENABLE_UBSAN`). [Completed ✅]
        - 1.3: Establish a lightweight, zero-dependency C++ unit assertion harness for engine regression testing. [Completed ✅]
    - **Phase 2: Headless VDOM Test Renderer & Simulator (`keet/Testing.h`)** [Completed ✅]
        - 2.1: Implement `TestHarness::mount<Component>(props)` to render components headlessly into an inspectable VNode hierarchy. [Completed ✅]
        - 2.2: Add CSS-selector query helpers (`find(".btn")`, `findAll("li")`, `text()`) and assertion macros (`KEET_ASSERT_TEXT`, `KEET_ASSERT_ATTR`). [Completed ✅]
        - 2.3: Provide synthetic event dispatchers (`triggerClick()`, `triggerInput("value")`) that execute bound C++ lambdas synchronously and return updated VNodes. [Completed ✅]
    - **Phase 3: Integration Runner & CLI (`cattery test`)** [Completed ✅]
        - 3.1: Support ephemeral `:memory:` database provisioning and mock `Session` contexts for end-to-end integration tests. [Completed ✅]
        - 3.2: Add `keet_add_test()` macro to CMake to compile and execute `.test.keet` components with formatted test reporting. [Completed ✅]
        - 3.3: Implement VNode snapshot testing (`assertMatchesSnapshot()`) to prevent unintended UI or scoped CSS regressions. [Completed ✅]

---

## 4. Master Engineering Sequence & Architecture Roadmap 🚀

The roadmap follows the natural dependency chain: **Language & Minimal Core First ➔ Package Manager & Remote Downloader Second ➔ Framework Decoupling into Separate Repo Third ➔ Advanced Capabilities Fourth**.

```
Phase I: KeetScript Language Evolution & Core Runtime (libkeet) [Top 1 Priority]
├── Feature 7: Standalone KeetScript Language & keety AOT Pipeline [Top 1 Priority]
├── Feature 8: libkeet Minimal Language Base Runtime (I/O streams, math, memory primitives)
└── Feature 9: Compile-Time Memory Safety (Borrow Checker) & K-IR Incremental Pipeline

Phase II: Cattery Package CLI & Remote Downloader (cattery) [Top 2 Priority]
├── Feature 10: cattery CLI (cattery init, cattery add, cattery build) & Workspace (.cattery/) [Top 2 Priority]
├── Feature 11: Project Manifest (pedigree.json) & Remote Cattery Monorepo Downloader
├── Feature 12: Zero-Config Hot-Reloading ("Zoomies" / cattery dev)
└── Feature 13: Interactive Component & Deterministic VDOM Snapshot Testing (cattery test)

Phase III: Framework Decoupling into Separate Cattery Monorepo [Top 3 Priority]
├── Feature 14: cattery-http-server & cattery-ws Decoupling (Standalone Networking Packages)
├── Feature 15: cattery-uiengine Decoupling (Server-Driven VDOM, Reactive Components, Slots)
├── Feature 16: cattery-sqlite Decoupling & Smart Auto-Migrations (cattery evolve)
├── Feature 17: cattery-auth (Session Security, CSPRNG & Declarative policy blocks)
└── Feature 18: cattery-router (Multi-Page Routing & URL State Synchronization)

Phase IV: Advanced Language Features & High-Performance Capabilities
├── Feature 19: Compile-Time Reactive SQL Binding (sql:query / sql:exec) & E2E Contracts (bind:value)
├── Feature 20: Integrated Client-Server Type Guarding & RPCs (def server:...)
├── Feature 21: Compile-Time Component Tree-Shaking & CSS Module Hashing (keety --css-modules)
├── Feature 22: AI-Driven Template Auto-Memoization (keety --smart-memo)
├── Feature 23: Ultra-Low Latency Arena Allocators (--arena-alloc) & Memory-Mapped State (state persist)
├── Feature 24: Smart Thread-Affinity Session Pinning (@pin-thread) & Autonomous Workers (cattery worker)
├── Feature 25: Asynchronous Coroutines, Modern Event Loops & Zero-Downtime State Hand-offs (Hot Binary Swapping)
├── Feature 26: Zero-Copy Server-Sent HTML Overlays (cattery swap) & Layout Morphing (<morph:view>)
├── Feature 27: Smart Declarative Caching Primitives (@cache) with Relational Invalidation
└── Feature 28: Edge P2P Cluster Sync ("Kitten Mesh") via CRDTs

Phase V: Developer Diagnostics, Observability, Packaging & Enterprise Ecosystem
├── Feature 29: Zero-Overhead Live-State Time Travel (cattery record) & Session Replay
├── Feature 30: Ghost-Mode Distributed Load Testing (cattery ghost)
├── Feature 31: Browser-Based C++ REPL ("Kitty Shell") & Visual State Inspector (cattery dashboard)
├── Feature 32: Native OpenAPI Specification (/docs) & LSP Daemon (litterbox)
├── Feature 33: Automated Dependency Injection (import service) & GraphQL Schema Mapping (schema)
├── Feature 34: Compile-Time Binary Asset Baking (cattery bake)
└── Feature 35: Autonomous Multi-Target Desktop Bundler (dmstct-keety / cattery build --target desktop)

Phase VI: The Cattery (cattery-fx) Packages Monorepo Repository
└── 63 Foundational Native Packages in Separate Monorepo
```

---

### Phase I: KeetScript Language Evolution & Core Runtime (`libkeet`) [Top 1 Priority 🎯]

#### Feature 7: Standalone KeetScript Language & `keety` AOT Pipeline [Top 1 Priority 🎯]
- **Status:** Strategic Language Foundation ⏳
- **Addresses:** KeetScript is currently constrained to Single-File Component (SFC) markup; lacks general-purpose language capabilities.
- **Description:** Elevates KeetScript into a **true, standalone general-purpose systems programming language** compiling directly into clean, idiomatic C++20 header (`.h`) and implementation (`.cpp`) files. Developers can author pure business logic, algorithms, models, and modular libraries—including future Cattery packages—entirely in KeetScript without requiring `<template>`, `<style>`, or `<server>` markup.
- **Key Syntax & Directives:**
  
  **1. Functions & Entry Point (`main`):**
  ```keet
  fn add(a: int, b: int): int {
      return a + b;
  }

  fn main(): void {
      let c: int = add(4, 5);
      print("Hello, World! 4 + 5 =", c);
  }
  ```
  *Output:*
  ```
  Hello, World! 4 + 5 = 9
  ```

  **2. Classes, Members, Visibility & Constructors:**
  ```keet
  export class Foo {
      public let c: int;
      private let d: int = 4;

      Foo(a: int, b: int) {
          this.c = a + b;
          print(subtract());
      }

      private fn subtract(): int {
          return this.c - this.d;
      }
  }
  ```

  **3. C++20 Transpilation Mapping (`keety`):**
  - `fn <name>(<args>): <type>` maps to idiomatic C++20 function declarations (`auto` or explicit return types).
  - `fn main(): void` generates the standard C++ `int main(int argc, char** argv)` runtime entry point.
  - `print(...)` maps to zero-overhead stream formatting in `libkeet` (`keet::io::print(...)` via `std::cout` / `std::format`).
  - `let <name>: <type> = <expr>;` maps to typed variable declarations.
  - `export class Foo` generates `#pragma once` header declarations with corresponding `public:` and `private:` blocks.
  - `Foo(...)` maps to standard C++ class constructors with initializer lists.
  - `this.property` maps directly to C++ `this->property` member access.
- **Phases:**
    - **Phase 1: Pure Module Grammar & Standalone Syntax**
        - 1.1: Support pure KeetScript source files without XML tags—enabling top-level `module`, `import`, `export`, `struct`, `class`, `enum`, `interface`, and `fn` / `def` declarations.
        - 1.2: Expand static type system: type inference, sum types (algebraic enums with values), generics (`List<T>`, `Result<T, E>`), and exhaustive pattern matching (`match`).
    - **Phase 2: Standalone C++ Library Code Generator (`keety --lib`)**
        - 2.1: Implement Library Compilation Mode (`keety --lib -i Module.keet -o Module.cpp -h Module.h`) emitting clean C++20 header/source pairs with `#pragma once`, namespaces, and typed exports.
        - 2.2: Dual Compilation Modes: `keety` automatically differentiates between pure language modules (`.keet` without `<template>`) producing C++20 libraries, and UI components (with `<template>`) producing UI component classes.
        - 2.3: CMake integration: provide `keet_add_library(target_name Sources.keet)` producing native CMake C++ targets linkable by any C++ application.

---

#### Feature 8: `libkeet` Minimal Language Base Runtime
- **Status:** High Priority ⏳
- **Description:** Establishes `libkeet` as the ultra-lean baseline runtime for the KeetScript language. It contains **no web, HTTP, WebSocket, VDOM, or database logic**. It provides strictly the foundational language runtime primitives:
  - Basic I/O streams (`iostream`, file handles, standard in/out/err).
  - Basic math operations (fixed-width numerics, matrix math, SIMD helpers).
  - Memory primitives (lightweight monotonic bump allocators, RAII wrappers, buffer views).
  - Fundamental strings and collections (`String`, `StringView`, `Vector`, `HashMap`).
- **Footprint Goal:** Sub-500KB compiled binary size; zero external runtime dependencies on macOS, Linux, and Windows.
- **Phases:**
    - **Phase 1: Minimal Base Runtime Extraction**
        - 1.1: Cleanly extract the base runtime into `libkeet` (`include/keet/runtime/`, `src/runtime/`).
        - 1.2: Implement foundational I/O, string formatting, and numeric primitives.
    - **Phase 2: Compiler Runtime Integration**
        - 2.1: Configure `keety` code generator to link generated C++ code against `libkeet` baseline headers.
        - 2.2: Verify non-web CLI programs compile into standalone binaries linking only `libkeet`.

---

#### Feature 9: Compile-Time Memory Safety (Borrow Checker) & K-IR Incremental Pipeline
- **Status:** Compiler Architecture Milestone ⏳
- **Description:** Introduces a compile-time borrow checker in `keety` to detect memory safety hazards before emitting C++, alongside a typed intermediate representation (K-IR) for sub-second incremental builds.
- **Phases:**
    - **Phase 1: Compile-Time Borrow Checker**
        - 1.1: Evolve KeetScript's type system with ownership semantics, move semantics, and reference lifetimes.
        - 1.2: Implement compile-time borrow checker in `keety` catching use-after-free, double-frees, and dangling references at build time with zero runtime garbage collection.
    - **Phase 2: K-IR Intermediate Representation**
        - 2.1: Design typed binary Intermediate Representation (K-IR) to decouple front-end syntax parsing from C++ codegen backends.
        - 2.2: Implement fine-grained AST dependency graph tracking for lightning-fast incremental rebuilds.

---

### Phase II: Cattery Package CLI & Ecosystem Infrastructure (`cattery`) [Top 2 Priority 🎯]

#### Feature 10: `cattery` CLI & Workspace Management (`.cattery/`) [Top 2 Priority 🎯]
- **Status:** Strategic Tooling Priority ⏳
- **Description:** Implements the native `cattery` command-line utility. Acts as the package manager, project orchestrator, and build toolchain manager.
- **Key Commands:**
  - `cattery init <name> [--lib | --app | --desktop]`: Scaffolds standard project layouts.
  - `cattery add <package>`: Downloads package from the separate Cattery packages monorepo repository and records it in `pedigree.json`.
  - `cattery build [--target server|desktop|lib]`: Compiles project via CMake and `keety`.
- **The `.cattery/` Workspace:** Isolates all generated C++ sources (`.cattery/codegen/`), downloaded package checkouts (`.cattery/packages/`), and CMake build artifacts (`.cattery/build/`), keeping user code directories clean.
- **Phases:**
    - **Phase 1: CLI Core & Project Scaffolding (`cattery init`)**
        - 1.1: Implement native C++ `cattery` executable using `cattery-cli`.
        - 1.2: Generate standard directory structures (`src/`, `pedigree.json`, `.gitignore`).
    - **Phase 2: `.cattery/` Cache Isolation**
        - 2.1: Redirect all intermediate transpilation outputs and CMake configurations into `.cattery/`.
        - 2.2: Implement package storage and cache management under `.cattery/packages/`.

---

#### Feature 11: Project Manifest (`pedigree.json`) & Remote Cattery Monorepo Downloader
- **Status:** Package Management Milestone ⏳
- **Description:** Defines and implements the declarative project manifest `pedigree.json` tracking dependencies, targets, and compilation flags, combined with a remote package acquisition engine that fetches packages directly from the separate Cattery packages monorepo repository.
- **Manifest Example:**
  ```json
  {
    "name": "my-app",
    "version": "1.0.0",
    "target": "server",
    "compiler": { "opt_level": "O3", "smart_memo": true },
    "dependencies": {
      "cattery-http-server": "^1.0.0",
      "cattery-uiengine": "^1.0.0",
      "cattery-sqlite": "^1.0.0"
    }
  }
  ```
- **Remote Acquisition Flow:**
  1. Developer executes `cattery add cattery-sqlite`.
  2. `cattery` CLI queries the remote Cattery packages monorepo repository (e.g. GitHub release tags or git tree).
  3. The package source directory (`packages/cattery-sqlite/`) is downloaded and extracted into `.cattery/packages/cattery-sqlite/`.
  4. Dependency version lock is written into `pedigree.json`.
  5. CMake targets are dynamically injected into `.cattery/CMakeLists.txt`.
- **Phases:**
    - **Phase 1: Manifest Schema & Parser**
        - 1.1: Strict JSON schema validator for `pedigree.json`.
        - 1.2: Implement `cattery add <pkg>` and `cattery remove <pkg>` manifest mutators.
    - **Phase 2: Remote Monorepo Fetcher & Caching Engine**
        - 2.1: Implement HTTP/git downloader in `cattery` CLI fetching package subtrees from the official Cattery packages repository.
        - 2.2: Integrity verification (SHA-256 checksums) and local caching under `.cattery/packages/`.
        - 2.3: Automatic CMake configuration synthesis linking adopted package targets.

---

#### Feature 12: Zero-Config Hot-Reloading ("Zoomies" / `cattery dev`)
- **Status:** High DX Priority ⏳
- **Description:** Instant (<50ms) local development loop. Cross-platform file watcher detects changes in `.keet`, `.cpp`, and `.h` files, compiles the delta unit, hot-swaps UI component VNode patches in connected browser tabs over WebSockets, and preserves component states (`useState`), active form input values, and scroll positions without full-page refreshes.
- **Phases:**
    - **Phase 1: Cross-Platform File Watcher (`kqueue` / `inotify`)**
        - 1.1: Incremental watcher inside `cattery dev` with debounce handling.
        - 1.2: In-browser compile diagnostic overlay for syntax errors.
    - **Phase 2: Dynamic VNode Hot-Swapping & State Preservation**
        - 2.1: Compile delta units into dynamic shared objects or VNode patch frames.
        - 2.2: Stream surgical patch over active WebSocket connection; preserve session state.

---

#### Feature 13: Interactive Component & Deterministic VDOM Snapshot Testing (`cattery test`)
- **Status:** Quality Gate Milestone ⏳
- **Description:** Headless UI testing harness and CLI utility generating unit test skeletons for `.keet` files, asserting VNode structures and server-rendered HTML payloads natively as fast C++ unit tests.
- **New Capabilities:**
  - **Component Unit Testing:** Mount components headlessly and assert rendered VNodes without opening sockets or browsers.
  - **Deterministic Virtual DOM Snapshot Testing:** Compares deterministic JSON VNode hierarchies instantly during builds or CI pipelines.
- **Phases:**
    - **Phase 1: Test Generator & Runner (`cattery test`)**
        - 1.1: Scaffolding tool generating `.test.keet` skeletons with native assertion macros.
        - 1.2: Headless mounting harness executing component render cycles in memory.
    - **Phase 2: Deterministic Snapshot Serialization**
        - 2.1: Normalized JSON VNode serializer producing deterministic output.
        - 2.2: Automated snapshot diffing and interactive update flags (`cattery test --update-snapshots`).

---

### Phase III: Framework Decoupling into Separate Cattery Monorepo [Top 3 Priority 🎯]

#### Feature 14: `cattery-http-server` & `cattery-ws` Decoupling (Standalone Networking Packages)
- **Status:** Critical Framework Decoupling ⏳
- **Description:** Extracts the HTTP/1.1 and RFC 6455 WebSocket server implementation out of the core `keet` repository into the separate Cattery packages monorepo (`packages/cattery-http-server/` and `packages/cattery-ws/`).
- **Phases:**
    - **Phase 1: `cattery-http-server` Package Extraction**
        - 1.1: Relocate `Server`, `ServerRequest`, `ServerResponse`, and HTTP middleware chain into the separate Cattery repository.
        - 1.2: Decouple from UI engine; expose clean request dispatching interfaces.
    - **Phase 2: `cattery-ws` Protocol Extraction**
        - 1.1: Extract RFC 6455 frame parsing, masking, and client/server session tracking into `packages/cattery-ws/`.
        - 1.2: Support standalone WebSocket servers, CLI clients, and background push queues.

---

#### Feature 15: `cattery-uiengine` Decoupling (Server-Driven VDOM, Reactive Components, Slots)
- **Status:** Critical Framework Decoupling ⏳
- **Description:** Extracts the reactive UI engine out of the core repository into `packages/cattery-uiengine/` inside the Cattery packages monorepo. Encompasses `UIEngine`, component lifecycle, VNode diffing, slot content projection (`<slot />`), and client JS runtime generation.
- **Phases:**
    - **Phase 1: `cattery-uiengine` Extraction**
        - 1.1: Decouple `UIEngine`, `Component`, `VNode`, and `Props` into `packages/cattery-uiengine/`.
        - 1.2: Ensure clean interoperability with `cattery-http-server` via abstract event streams.
    - **Phase 2: Server-Driven Content Projection (`<slot />`)**
        - 2.1: Support default `<slot />` and named slots (`<slot name="header" />`) with fallback default content.
        - 2.2: Project caller child VNodes into child component slot locations at compile time with zero runtime allocation overhead.

---

#### Feature 16: `cattery-sqlite` Decoupling & Smart Auto-Migrations (`cattery evolve`)
- **Status:** Critical Framework Decoupling ⏳
- **Description:** Decouples database storage into `packages/cattery-sqlite/` in the separate Cattery repository, bundling an in-tree SQLite 3 amalgamation (`sqlite3.c`), WAL-mode concurrency, ACID durability, and the `cattery evolve` schema auto-migration engine.
- **New Capabilities:**
  - **Smart Database Schema Auto-Migration (`cattery evolve`):** Automatically introspects existing SQLite database files, compares them against current model definitions, computes the structural delta, and applies safe in-place `ALTER TABLE` statements (adding columns, creating indices) without data loss.
- **Phases:**
    - **Phase 1: SQLite Amalgamation & C++ Engine**
        - 1.1: Package SQLite 3 amalgamation (`sqlite3.c` / `sqlite3.h`) inside `packages/cattery-sqlite/`.
        - 1.2: Implement `SqliteDatabase` wrapper featuring RAII connection management, WAL mode, prepared statement caching, and parameterized value binding.
    - **Phase 2: Smart Schema Auto-Migration CLI (`cattery evolve`)**
        - 2.1: Implement schema diffing engine reading table pragma information (`PRAGMA table_info`) from target SQLite files and comparing against AST declarations.
        - 2.2: Expose `cattery evolve --dry-run` and `cattery evolve --apply` with automatic rollback snapshots.

---

#### Feature 17: `cattery-auth` (Session Security, CSPRNG & Declarative `policy` blocks)
- **Status:** High Security Milestone ⏳
- **Description:** Decoupled authentication package in `packages/cattery-auth/` providing cryptographic session storage, OS CSPRNG entropy (`getrandom`), CSRF protection, and declarative edge security policy blocks.
- **New Capabilities:**
  - **Declarative Edge Security Policies (`policy` blocks):** Declare component-level security, role-based access control (RBAC), and attribute-based access control (ABAC) rules directly within `.keet` files:
    ```keet
    policy {
        allow read: if role in ["admin", "member"];
        allow write: if role == "admin" && session.is_mfa_verified;
    }
    ```
    Compiled simultaneously into HTTP route middleware guards and VDOM rendering barriers.
- **Phases:**
    - **Phase 1: Hardened Cryptographic Session Store (`cattery-auth`)**
        - 1.1: Cryptographically secure 128-bit session IDs using OS CSPRNG (`getrandom`), rejecting forged client headers.
        - 1.2: Persistent SQLite session store (`SqliteSessionStore`) with sliding TTLs.
    - **Phase 2: Declarative Edge Security Policies (`policy` blocks)**
        - 2.1: Add `policy` syntax grammar to KeetScript.
        - 2.2: Transpile `policy` blocks into dual guards: route-level request interceptors in HTTP handlers, and rendering filters in the VDOM generator.

---

#### Feature 18: `cattery-router` (Multi-Page Routing & URL State Synchronization)
- **Status:** Medium Priority ⏳
- **Description:** Decouples routing into `packages/cattery-router/`. Introduces dynamic route parameters (`:id`), wildcard catch-alls (`*`), nested layout routes, reactive query parameter binding (`bind:query`), and programmatic navigation synchronized with the browser History API (`pushState` / `popstate`).
- **Phases:**
    - **Phase 1: Dynamic Parameter Matching & Route Context**
        - 1.1: Support dynamic parameterized segments (`/users/:id`), wildcards (`/docs/*`), and query string extraction.
        - 1.2: Expose matched route parameters and query dictionaries directly to components via `useRoute()`.
    - **Phase 2: Client History Integration & Reactive Query Sync**
        - 2.1: Intercept `<Link>` navigation and `popstate` events over WebSocket while updating the browser URL bar.
        - 2.2: Support two-way reactive query parameter synchronization (`bind:query="searchQuery"`).

---

### Phase IV: Advanced Language Features & High-Performance Capabilities

#### Feature 19: Compile-Time Reactive SQL Binding (`sql:query` / `sql:exec`) & E2E Contracts (`bind:value`)
- **Status:** High Priority ⏳
- **Description:** Embeds native SQL statements directly into `.keet` components with strict compile-time verification against database schemas, paired with end-to-end type safety contracts bridging HTML input controls directly to database columns.
- **New Capabilities:**
  - **Compile-Time Reactive SQL Binding (`sql:query` / `sql:exec`):** Embed raw SQL statements directly in component script blocks. The compiler validates SQL syntax and column types against the target database schema at build time, generating zero-overhead C++ prepared statement bindings with zero runtime ORM overhead.
    ```keet
    let activeUsers = sql:query<User>("SELECT id, name, email FROM users WHERE status = ?", "active");
    sql:exec("UPDATE counters SET views = views + 1 WHERE id = ?", pageId);
    ```
  - **End-to-End Type Safety Contracts (`bind:value` to SQL Schema):** Connect `<input>` fields directly to schema-defined database columns. The compiler verifies field lengths, numeric ranges, and regex formats at build time, synthesizing both client-side validation attributes and server-side safety checks automatically.
- **Phases:**
    - **Phase 1: Compile-Time SQL Parsing & Schema Introspection**
        - 1.1: Add `sql:query<T>(...)` and `sql:exec(...)` expressions to KeetScript grammar.
        - 1.2: Connect `keety` semantic analyzer to SQLite schema metadata at compile time to validate queries and types.
        - 1.3: Generate direct C++ prepared statement parameter binding (`sqlite3_bind_*`) indexed at compile time.
    - **Phase 2: End-to-End Input Contracts**
        - 2.1: Implement type-contract bridging from `<input bind:value="user.age" />` to schema column `INTEGER CHECK(age >= 0)`.
        - 2.2: Emit HTML5 constraints (`min="0"`, `type="number"`) and server-side type-coercion guards during code generation.

---

#### Feature 20: Integrated Client-Server Type Guarding & RPCs (`def server:...`)
- **Status:** High Priority ⏳
- **Description:** Declares server-side Remote Procedure Call (RPC) functions directly inside component script blocks without writing separate REST controller routes or client fetch wrappers.
- **New Capabilities:**
  - **Declarative Server RPCs (`def server:...`):** Write functions that execute strictly on the server while callable transparently from frontend event handlers:
    ```keet
    <script>
    def server:createUser(name: string, email: string): Result<User, string> {
        return db.insert("users", { name, email });
    }

    fn handleClick() {
        let res = await createUser("Alice", "alice@example.com");
    }
    </script>
    ```
  - **Automated Serialization & Type Guarding:** Bi-directional JSON-RPC over WebSocket frames generated automatically with compile-time argument type validation and cryptographic session authentication checks.
- **Phases:**
    - **Phase 1: Grammar & AST Support**
        - 1.1: Add `def server:<name>(<args>): <returnType> { ... }` syntax to KeetScript parser.
        - 1.2: Isolate server RPC AST bodies so they are never emitted into client-side JS bundles or desktop view scripts.
    - **Phase 2: RPC Dispatcher & Code Generation**
        - 2.1: Generate typed C++ server route dispatchers in the component's generated translation unit.
        - 2.2: Synthesize client-side async stub invoking the RPC over the active WebSocket connection with automatic promise resolution.

---

#### Feature 21: Compile-Time Component Tree-Shaking & CSS Module Hashing (`keety --css-modules`)
- **Status:** Performance Optimization ⏳
- **Description:** Eliminates binary bloat and CSS selector collisions by pruning unreferenced templates and styles at compile time, while generating cryptographically deterministic, scoped CSS class hashes.
- **New Capabilities:**
  - **Compile-Time Component Tree-Shaking:** Static dependency analysis traces the component mount tree from the root application entry point. Unused component definitions, dead `<template>` branches, unreferenced helper functions, and orphaned `<style>` rules are completely stripped before C++ code emission.
  - **Compile-Time CSS Module Hashing (`keety --css-modules`):** Generates deterministic, compact class name hashes (e.g., `.btn_a8f3`) during compilation, eliminating runtime CSS selector scoping overhead and ensuring zero class name collisions across large component libraries.
- **Phases:**
    - **Phase 1: Component Dependency Graph Analysis**
        - 1.1: Build static reachability graph across all declared components, templates, and server RPCs.
        - 1.2: Prune unreferenced nodes from the AST prior to code generation.
    - **Phase 2: Deterministic CSS Module Hashing**
        - 2.1: Implement hash generation (`XXH3` / `MurmurHash3`) based on file path and class name.
        - 2.2: Rewrite class selectors in template markup and CSS stylesheets at compile time.

---

#### Feature 22: AI-Driven Template Auto-Memoization (`keety --smart-memo`)
- **Status:** Strategic Performance Innovation ⏳
- **Description:** An intelligent AOT compilation mode that analyzes component reactivity graphs to pre-render and memoize completely static UI subtrees into raw HTML strings at compile time.
- **New Capabilities:**
  - **AOT Reactive Boundary Analysis:** `keety --smart-memo` traverses `<template>` AST nodes, identifying branches that depend solely on compile-time constants or immutable props.
  - **Bypassing Runtime VDOM Diffing:** Static subtrees are pre-compiled into static HTML byte buffers. During runtime re-renders, the Virtual DOM diffing engine skips diffing these subtrees entirely, streaming pre-rendered HTML fragments directly to the network socket.
- **Phases:**
    - **Phase 1: Reactivity Graph Analysis in `keety`**
        - 1.1: Identify dynamic reactive dependencies (`state.*`, `props.*`, `bind:*`) for each VNode element.
        - 1.2: Partition template trees into mutable reactive islands and immutable static continents.
    - **Phase 2: Static Byte Buffer Emission**
        - 2.1: Transpile static continents into `std::string_view` literals in the emitted C++ component class.
        - 2.2: Hook `UIEngine::diff()` to fast-path memoized subtrees with a single pointer comparison.

---

#### Feature 23: Ultra-Low Latency Arena Allocators (`--arena-alloc`) & Memory-Mapped State Persistence (`state persist`)
- **Status:** High Performance Milestone ⏳
- **Description:** Delivers bare-metal memory performance and instant crash-resilient state recovery through thread-local bump allocation pools and POSIX memory-mapped file persistence.
- **New Capabilities:**
  - **Ultra-Low Latency Arena Allocators (`--arena-alloc`):** Introduces thread-local monotonic arena memory pools (`ArenaAllocator`) for each request/frame render cycle. Eliminates heap fragmentation, mutex contention, and `malloc`/`free` overhead by freeing entire request trees in a single pointer reset.
  - **Memory-Mapped State Persistence (`state persist`):** Links component state variables directly to zero-copy memory-mapped disk files via `mmap`. State modifications are flushed to disk asynchronously by the kernel, enabling instant application recovery across process crashes or system reboots with zero database query overhead.
    ```keet
    <script>
    state persist counter: int = 0;
    state persist sessionData: UserSession;
    </script>
    ```
- **Phases:**
    - **Phase 1: Monotonic Arena Allocator (`keet::Arena`)**
        - 1.1: Implement high-performance thread-local bump allocator with configurable page sizes (e.g., 64KB chunks).
        - 1.2: Overload VNode and `Json::Value` allocation operators to allocate directly out of the active thread's frame arena.
        - 1.3: Reset the arena pointer at the completion of each HTTP request or WebSocket event frame.
    - **Phase 2: POSIX `mmap` State Persistence Engine**
        - 2.1: Implement `MmapStore` managing memory-mapped files under `.cattery/state/`.
        - 2.2: Add `state persist` keyword in KeetScript parser to map variables to fixed-offset memory regions.
        - 2.3: Support atomic swap and sync policies (`MS_ASYNC` vs `MS_SYNC`).

---

#### Feature 24: Smart Thread-Affinity Session Pinning (`@pin-thread`) & Autonomous Background Workers (`cattery worker`)
- **Status:** Concurrency & Throughput Milestone ⏳
- **Description:** Maximizes CPU cache locality and worker parallelism by pinning active client sessions to specific OS worker threads and managing asynchronous background tasks natively within the runtime.
- **New Capabilities:**
  - **Smart Thread-Affinity Session Pinning (`@pin-thread`):** Routes incoming WebSocket frames for a given session consistently to the same worker thread. Enables lock-free, thread-local session caching, eliminating lock contention on session state across high-concurrency workloads.
  - **Autonomous Background Worker Threads (`cattery worker`):** Declare long-running background tasks, queue consumers, or periodic workers managed natively inside the C++ runtime using C++20 coroutine syntax:
    ```keet
    worker def cleanExpiredSessions(interval: Duration = 1.hours) {
        while (running) {
            db.exec("DELETE FROM sessions WHERE expires_at < unixepoch()");
            await sleep(interval);
        }
    }
    ```
- **Phases:**
    - **Phase 1: Thread-Affinity Dispatcher**
        - 1.1: Implement consistent hashing dispatcher on session ID in `HTTPServer::Server`.
        - 1.2: Pin WebSocket client file descriptors to dedicated thread-local event loops (`epoll`/`kqueue`).
        - 1.3: Enable thread-local component state caching with zero synchronization locks.
    - **Phase 2: Runtime Worker Manager (`cattery worker`)**
        - 2.1: Add `worker` declaration syntax to KeetScript.
        - 2.2: Integrate C++20 coroutine task scheduler spawning lightweight fiber workers inside thread pools.

---

#### Feature 25: Asynchronous Coroutines, Modern Event Loops & Zero-Downtime State Hand-offs (Hot Binary Swapping)
- **Status:** Scalability & High Availability Milestone ⏳
- **Description:** Transforms Keet's network architecture into an asynchronous, non-blocking coroutine engine with kernel I/O multiplexing (`kqueue` / `io_uring`), coupled with zero-downtime hot binary swapping for production deployments.
- **New Capabilities:**
  - **Asynchronous Coroutine Event Loops:** Stackless C++20 coroutines (`co_await`) paired with `kqueue` (macOS/BSD) and `io_uring` / `epoll` (Linux) allowing single Keet processes to sustain 100,000+ concurrent WebSocket connections.
  - **Zero-Downtime Hot Binary Swapping:** Upgrade production binaries without dropping active WebSocket connections or losing session states. Spawns the new binary in parallel, transfers active client socket file descriptors over a Unix domain socket (`SCM_RIGHTS`), serializes active sessions via POSIX shared memory (`shm_open`), and cleanly terminates the old process.
- **Phases:**
    - **Phase 1: Asynchronous Event Loop Core**
        - 1.1: Abstract non-blocking I/O multiplexer supporting `kqueue` and `io_uring`.
        - 1.2: Implement awaitable socket read/write primitives (`co_await socket.read()`).
    - **Phase 2: Hot Binary Swapping & FD Hand-off**
        - 2.1: Implement IPC handshake protocol between parent and child processes over a local Unix domain socket.
        - 2.2: Pass listening sockets and active client WebSocket file descriptors via `sendmsg()` with `SCM_RIGHTS`.
        - 2.3: Transfer in-flight session state via shared memory segments (`shm_open`) and drain old worker threads gracefully.

---

#### Feature 26: Zero-Copy Server-Sent HTML Overlays (`cattery swap`) & Layout Morphing (`<morph:view>`)
- **Status:** Reactive UI Innovation ⏳
- **Description:** Eliminates client-side JSON parsing and Virtual DOM reconciliation overhead by streaming raw HTML delta micro-fragments directly over WebSockets, complemented by server-driven layout morphing transitions.
- **New Capabilities:**
  - **Zero-Copy Server-Sent HTML Overlays (`cattery swap`):** Instead of serializing VDOM patches as JSON structures for client JavaScript to diff, `cattery swap` performs the diff on the server and emits minimal raw HTML morph fragments (`<div id="target" keet-swap="morph">...</div>`). The client runtime applies swaps with native `Element.replaceWith` or DOM morphing with zero deserialization overhead.
  - **Zero-Overhead Layout Morphing (`<morph:view>`):** Coordinates fluid page transitions and shared-element view animations driven by server-side geometric delta computations.
- **Phases:**
    - **Phase 1: HTML Morph Fragment Streamer**
        - 1.1: Extend `UIEngine::diff()` to generate raw HTML swap payloads with target selector headers.
        - 1.2: Upgrade client runtime to parse and apply server-sent HTML fragments into the DOM with zero client-side JSON overhead.
    - **Phase 2: Layout Morphing Protocol (`<morph:view>`)**
        - 2.1: Add `<morph:view name="header">` directive to KeetScript template grammar.
        - 2.2: Track component layout identifiers and emit CSS view-transition headers during route transitions.

---

#### Feature 27: Smart Declarative Caching Primitives (`@cache`) with Relational Invalidation
- **Status:** Performance Optimization ⏳
- **Description:** Eliminates repetitive computations and database queries through function-level caching annotations with automatic relation-aware invalidation.
- **New Capabilities:**
  - **Declarative Function Caching (`@cache`):** Annotate expensive data fetching or rendering functions with TTL and memory limits:
    ```keet
    @cache(ttl = 5.minutes, max_entries = 1000)
    fn getLeaderboard(): List<PlayerScore> {
        return db.query<PlayerScore>("SELECT * FROM scores ORDER BY points DESC LIMIT 10");
    }
    ```
  - **Relational-Aware Invalidation:** The caching engine tracks table dependencies referenced inside cached queries. Whenever a state mutation or SQL execution modifies the underlying table (`scores`), affected cache entries are automatically evicted across all worker threads.
- **Phases:**
    - **Phase 1: LRU Caching Store with TTL Support**
        - 1.1: Build high-concurrency sharded LRU/LFU cache store (`cattery-cache`).
        - 1.2: Add `@cache` function decorator syntax to KeetScript.
    - **Phase 2: Table Mutation Dependency Tracking**
        - 2.1: Automatically register table dependencies during `sql:query` execution inside cached functions.
        - 2.2: Hook `TableProxy` and `SqliteDatabase::exec` to trigger targeted cache invalidations on insert/update/delete operations.

---

#### Feature 28: Edge P2P Cluster Sync ("Kitten Mesh") via CRDTs
- **Status:** Distributed Systems Milestone ⏳
- **Description:** Enables multiple lightweight Keet instances (such as edge servers, Raspberry Pis, or branch offices) to form a decentralized peer-to-peer mesh network without a centralized master database.
- **New Capabilities:**
  - **CRDT State Synchronization:** Leverages state-based and operation-based Conflict-free Replicated Data Types (CRDTs—LWW-Element-Set, PN-Counter, Observed-Remove Set) to synchronize distributed state over encrypted WebSockets.
  - **Autonomous Mesh Peer Discovery:** Nodes discover peers on local networks via mDNS / ZeroConf and establish resilient peer-to-peer mesh connections with automatic partition recovery and convergent eventual consistency.
- **Phases:**
    - **Phase 1: CRDT Data Types Engine**
        - 1.1: Implement core CRDT primitives (State Vector, Lamport Clocks, LWW-Set, Map) in C++20.
        - 1.2: Expose `mesh:state` reactive directives in KeetScript.
    - **Phase 2: Peer-to-Peer WebSocket Gossip Protocol**
        - 2.1: Build peer discovery engine (mDNS / direct peer URI lists).
        - 2.2: Implement differential state sync protocol over WebSocket channels with automatic reconnect and state convergence.

---

### Phase V: Developer Diagnostics, Observability, Packaging & Enterprise Ecosystem

#### Feature 29: Zero-Overhead Live-State Time Travel (`cattery record`) & Session Replay
- **Status:** Advanced Diagnostics Milestone ⏳
- **Description:** A deterministic recording and replay engine enabling frame-by-frame state inspection to debug complex production issues and race conditions.
- **New Capabilities:**
  - **Session Recording (`.keet-session`):** Efficiently logs incoming WebSocket frames, user input mutations, database transactions, and component lifecycle events into a compact binary session file.
  - **Deterministic Time Travel Replay:** Replay recorded sessions in the developer environment frame-by-frame to inspect VNode trees, variable states, and SQL queries at any instant in the past.
- **Phases:**
    - **Phase 1: Binary Session Event Logger**
        - 1.1: Implement zero-allocation ring-buffer logger recording event inputs and timestamps.
        - 1.2: Compress recorded sessions using LZ4 into `.keet-session` files.
    - **Phase 2: Session Replay Engine (`cattery replay`)**
        - 2.1: Build headless replay harness feeding recorded frames into component instances deterministically.
        - 2.2: Provide step-forward, step-backward, and breakpoint inspection in the developer console.

---

#### Feature 30: Ghost-Mode Distributed Load Testing (`cattery ghost`)
- **Status:** Performance & Scale Tooling ⏳
- **Description:** Native CLI load testing utility capable of simulating massive real-world traffic against Keet servers.
- **New Capabilities:**
  - **Headless Client Fibers:** Spawns tens of thousands of lightweight headless WebSocket client fibers from a single machine to stress-test HTTP endpoints, WebSocket frame dispatchers, thread pool contention, and database lock concurrency.
  - **Real-Time Latency Metrics:** Emits live terminal percentiles (p50, p95, p99) for handshake latency, VNode patch delivery times, and memory consumption.
- **Phases:**
    - **Phase 1: Fiber-Based Client Simulator**
        - 1.1: Implement lightweight async WebSocket client harness powered by C++20 coroutines.
        - 1.2: Support scriptable user journeys (connect, authenticate, click, submit form, assert patch).
    - **Phase 2: Metrics Aggregator & Terminal Dashboard**
        - 2.1: Collect lock-free high-resolution latency histograms.
        - 2.2: Render interactive ANSI terminal dashboards displaying throughput, active fibers, error rates, and latency curves.

---

#### Feature 31: Browser-Based C++ REPL ("Kitty Shell") & Visual State Inspector (`cattery dashboard`)
- **Status:** Developer Experience & Observability ⏳
- **Description:** Live operational introspection suite giving developers complete visibility into running Keet server processes.
- **New Capabilities:**
  - **Browser-Based C++ REPL ("Kitty Shell"):** A password-protected live web terminal enabling developers to evaluate C++ expressions, inspect active sessions, and execute database queries directly against running process memory.
  - **Interactive Visual State Inspector (`cattery dashboard`):** A real-time local control center visualizing active WebSocket connections, VNode patch rates, memory arenas, thread pool utilization, and SQLite query latency telemetry.
- **Phases:**
    - **Phase 1: Kitty Shell REPL Web Interface**
        - 1.1: Implement secure WebSocket REPL channel guarded by administrative session tokens.
        - 1.2: Connect dynamic expression evaluator / reflection inspector to running application state.
    - **Phase 2: Visual State Dashboard (`cattery dashboard`)**
        - 1.1: Embed a dedicated monitoring endpoint streaming real-time JSON telemetry metrics.
        - 1.2: Provide interactive UI visualizing connection pools, memory arena allocations, and database query timings.

---

#### Feature 32: Native OpenAPI Specification (`/docs`) & LSP Daemon (`litterbox`)
- **Status:** Enterprise Tooling Milestone ⏳
- **Description:** Self-documenting API infrastructure and language intelligence suite.
- **New Capabilities:**
  - **Native OpenAPI Spec & Interactive Tester (`/docs`):** Compiles route handlers, request schemas, authentication requirements, and return types into an automated OpenAPI 3.1 specification, served with an interactive test playground at `/docs`.
  - **LSP Daemon (`litterbox` / `keety --lsp`):** Embedded Language Server Protocol daemon delivering autocomplete, type diagnostics, jump-to-definition, and refactoring across VS Code, Neovim, and CLion.
- **Phases:**
    - **Phase 1: Compile-Time OpenAPI Generation**
        - 1.1: Introspect server routes and parameter schemas during AOT compilation.
        - 1.2: Generate OpenAPI 3.1 JSON definitions and embed interactive documentation UI at `/docs`.
    - **Phase 2: Language Server Protocol Engine**
        - 2.1: Implement JSON-RPC 2.0 communication over standard I/O in `keety --lsp`.
        - 2.2: Provide real-time semantic diagnostics, prop completion, and cross-file navigation.

---

#### Feature 33: Automated Dependency Injection (`import service`) & GraphQL Schema Mapping (`schema`)
- **Status:** Enterprise Architecture Milestone ⏳
- **Description:** Advanced architectural building blocks for large-scale enterprise Keet applications.
- **New Capabilities:**
  - **Automated Dependency Injection (`import service`):** Declare singleton, scoped, or transient services in `.keet` components with compiler-managed wiring and constructor injection:
    ```keet
    import service PaymentGateway from "services/PaymentGateway.keet";
    ```
  - **Compile-Time GraphQL Schema Mapping (`schema`):** Declarative data model contracts that auto-generate SQLite migrations, C++ resolvers, type-safe query executors, and schema validation during AOT compilation.
- **Phases:**
    - **Phase 1: Dependency Injection Container**
        - 1.1: Add `service` declaration and import syntax to KeetScript.
        - 1.2: Synthesize compile-time dependency injection resolution graph in `Codegen.cpp`.
    - **Phase 2: GraphQL Schema Engine**
        - 2.1: Add `schema` block parsing in KeetScript supporting GraphQL SDL syntax.
        - 2.2: Generate type-safe C++ resolver skeletons and query execution dispatchers.

---

#### Feature 34: Compile-Time Binary Asset Baking (`cattery bake`)
- **Status:** Production Optimization Milestone ⏳
- **Description:** Eliminates disk I/O bottlenecks and file path resolution vulnerabilities by compressing and embedding static assets directly into the compiled executable binary.
- **New Capabilities:**
  - **LZ4 Compression & Binary Segment Embedding:** Assets (images, SVG icons, fonts, CSS, JavaScript client runtimes) are compressed using LZ4 and converted into static `const unsigned char[]` arrays placed in read-only binary data segments (`.rodata`).
  - **Zero-Copy Memory Serving:** The HTTP server serves baked assets directly from application memory using zero-copy socket transfers, bypassing the file system and disk read system calls entirely.
- **Phases:**
    - **Phase 1: Asset Baking CLI (`cattery bake`)**
        - 1.1: Scan static asset directories (e.g. `public/`, `assets/`).
        - 1.2: Compress files with LZ4 and generate C++ header/source compilation units embedding byte arrays and MIME lookup tables.
    - **Phase 2: Zero-Copy Server Integration**
        - 2.1: Integrate baked asset table into static file routing pipeline.
        - 2.2: Serve in-memory assets with pre-calculated ETags and immutable caching headers.

---

#### Feature 35: Autonomous Multi-Target Desktop Bundler (`dmstct-keety` / `cattery build --target desktop`)
- **Status:** Production Distribution Milestone ⏳
- **Description:** Compiles the exact same `.keet` codebase into either a standalone high-performance server binary or a lightweight native desktop application (macOS `.app`, Linux AppImage, Windows `.exe`) with zero Electron bloat.
- **New Capabilities:**
  - **Zero-Bloat Desktop Runtime:** Links native OS system webviews (WebKit on macOS, WebKitGTK on Linux, Edge WebView2 on Windows) directly to Keet's native C++ core (<5MB binary footprint, <10MB idle RAM, <100ms cold launch).
  - **Multi-Target Compilation Flag (`cattery build --target desktop`):** Seamless switch between server daemon and GUI desktop application from a single command.
- **Phases:**
    - **Phase 1: Native System Webview Window Harness**
        - 1.1: Integrate zero-dependency C++ system webview windowing wrapper for macOS, Linux, and Windows.
        - 1.2: In-memory IPC via custom WebView URI schemes, eliminating loopback TCP ports and firewall alerts.
    - **Phase 2: Automated Desktop Packaging**
        - 2.1: Build `dmstct-keety` packaging pipeline emitting signed macOS `.app` bundles, Linux AppImages, and Windows PE executables.
        - 2.2: Native OS integration: system tray, native file pickers, and OS desktop notifications.

---

## 5. Phase VI: The Cattery (`cattery-fx`) Packages Monorepo Repository 📦

All standard library extensions, network protocols, database connectors, and domain utilities reside in the **dedicated `cattery` packages monorepo repository** (`packages/<package-name>`). Developers adopt packages into their projects using the `cattery` CLI, which fetches them into `.cattery/packages/`.

### 5.1. Decoupled Core Architecture (`libkeet` vs Standard Packages) 🛠️

`libkeet` in the main `keet` repository is strictly the language baseline runtime (I/O, math, strings, memory). All higher-level capabilities live as separate packages in the `cattery` packages monorepo:

1. **`cattery-http-server`** — High-performance concurrent HTTP/1.1 server, connection multiplexing, and middleware pipeline.
2. **`cattery-uiengine`** — Server-driven Virtual DOM differential streaming, component lifecycle, and reactive state reconciliation.
3. **`cattery-sqlite`** — Embedded SQLite 3 amalgamation engine, query builders, WAL concurrency, and `cattery evolve` auto-migrations.
4. **`cattery-ws`** — Bidirectional RFC 6455 WebSocket framing, connection pooling, and real-time push streaming.
5. **`cattery-auth`** — Cryptographic primitives, OS CSPRNG entropy (`getrandom`), session stores, and declarative `policy` edge security blocks.
6. **`cattery-router`** — URL path routing, dynamic parameters (`:id`), wildcards, and history synchronizer.
7. **`cattery-json`** — High-speed JSON serialization/deserialization with `std::string_view` zero-copy tokenization.
8. **`cattery-async`** — Task schedulers, thread pools, futures/promises, channels, and C++20 coroutines (`co_await`).
9. **`cattery-log`** — Non-blocking asynchronous structured JSON logging with file rotation and correlation IDs.
10. **`cattery-cli`** — Declarative CLI option and subcommand parser powering `cattery`, `keety`, and user tools.
11. **`cattery-test`** — Lightweight unit-testing assertion library, headless VDOM snapshot testing, and test runner CLI (`cattery test`).
12. **`cattery-cache`** — Generic thread-safe LRU/LFU cache with sliding TTL expirations and `@cache` invalidation hooks.
13. **`cattery-rate-limit`** — Token-bucket, leaky-bucket, and sliding-window rate limiters for routes and sockets.
14. **`cattery-i18n`** — Multilingual translation catalogs, CLDR plural conformance, and dynamic locale switching.

---

### 5.2. Foundational Cattery Package Master Registry (63 Packages in Monorepo) 📋

| # | Package Name | Category | Primary Purpose | Distribution & Location |
|---|---|---|---|---|
| 1 | **`cattery-http-server`** | Core Networking | High-performance concurrent HTTP/1.1 server and middleware pipeline. | 📦 `cattery` repo: `packages/cattery-http-server/` |
| 2 | **`cattery-uiengine`** | Reactive UI | Server-driven Virtual DOM differential streaming, component lifecycle, and slots. | 📦 `cattery` repo: `packages/cattery-uiengine/` |
| 3 | **`cattery-sqlite`** | Data & Storage | Embedded SQLite 3 engine, query builders, WAL concurrency, and auto-migrations (`cattery evolve`). | 📦 `cattery` repo: `packages/cattery-sqlite/` |
| 4 | **`cattery-ws`** | Core Networking | Bidirectional RFC 6455 WebSocket frame manager for real-time streaming. | 📦 `cattery` repo: `packages/cattery-ws/` |
| 5 | **`cattery-auth`** | Core Security | Cryptographic primitives, CSPRNG, session management, and `policy` security blocks. | 📦 `cattery` repo: `packages/cattery-auth/` |
| 6 | **`cattery-router`** | Core Networking | URL path routing, dynamic parameters (`:id`), wildcards, and middleware stacking. | 📦 `cattery` repo: `packages/cattery-router/` |
| 7 | **`cattery-http`** | Core Networking | High-performance asynchronous HTTP/HTTPS client with TLS & chunked decoding. | 📦 `cattery` repo: `packages/cattery-http/` |
| 8 | **`cattery-json`** | Data & Storage | High-speed JSON serialization/deserialization wrapping `keet::Json`. | 📦 `cattery` repo: `packages/cattery-json/` |
| 9 | **`cattery-async`** | Concurrency & CLI | Task schedulers, thread pools, futures/promises, and coroutines (`co_await`). | 📦 `cattery` repo: `packages/cattery-async/` |
| 10 | **`cattery-log`** | Concurrency & CLI | Structured JSON logging with high-throughput asynchronous file/console sinks. | 📦 `cattery` repo: `packages/cattery-log/` |
| 11 | **`cattery-cli`** | Concurrency & CLI | Declarative CLI argument/flag parser with auto-generated help menus. | 📦 `cattery` repo: `packages/cattery-cli/` |
| 12 | **`cattery-test`** | Security & Testing | Lightweight unit-testing assertion library, mocking utilities, and test runner. | 📦 `cattery` repo: `packages/cattery-test/` |
| 13 | **`cattery-cache`** | Storage Extensions | In-memory key-value cache with LRU eviction, sliding TTLs, and `@cache` hooks. | 📦 `cattery` repo: `packages/cattery-cache/` |
| 14 | **`cattery-rate-limit`** | Telemetry & Limits | Token-bucket, leaky-bucket, and sliding-window rate limiters for routes and sockets. | 📦 `cattery` repo: `packages/cattery-rate-limit/` |
| 15 | **`cattery-i18n`** | Domain & Locale | Multilingual translation catalogs, CLDR plural rules, and dynamic locale switching. | 📦 `cattery` repo: `packages/cattery-i18n/` |
| 16 | **`cattery-validate`** | Data & Storage | Runtime data validation and parsing using static type schemas and model rules. | 🌱 `cattery` repo: `packages/cattery-validate/` |
| 17 | **`cattery-env`** | Data & Storage | Safe configuration and `.env` environment variable loading with type coercion. | 🌱 `cattery` repo: `packages/cattery-env/` |
| 18 | **`cattery-dash`** | Data Structures | Functional utility toolkits (filtering, mapping, grouping, zip, debounce, throttle). | 🌱 `cattery` repo: `packages/cattery-dash/` |
| 19 | **`cattery-uuid`** | Data Structures | Fast, cryptographically secure unique identifier generation (UUID v4, UUID v7). | 🌱 `cattery` repo: `packages/cattery-uuid/` |
| 20 | **`cattery-time`** | Data Structures | Timezone-aware date parsing, formatting, duration arithmetic, and cron scheduling. | 🌱 `cattery` repo: `packages/cattery-time/` |
| 21 | **`cattery-regex`** | Data Structures | Optimized regular expression pattern matching and lexical tokenizing. | 🌱 `cattery` repo: `packages/cattery-regex/` |
| 22 | **`cattery-path`** | Concurrency & CLI | Cross-platform file path manipulation, normalization, and directory traversal. | 🌱 `cattery` repo: `packages/cattery-path/` |
| 23 | **`cattery-jwt`** | Security & Testing | JSON Web Token issuance, HMAC/RSA signature verification, and stateless auth. | 🌱 `cattery` repo: `packages/cattery-jwt/` |
| 24 | **`cattery-markdown`** | Security & Testing | Ultra-fast Markdown parser emitting native Keet Virtual DOM node hierarchies. | 🌱 `cattery` repo: `packages/cattery-markdown/` |
| 25 | **`cattery-ui-icons`** | Security & Testing | Curated vector icon collection pre-packaged as native Keet UI components. | 🌱 `cattery` repo: `packages/cattery-ui-icons/` |
| 26 | **`cattery-queue`** | Storage Extensions | In-process and distributed background job queue with concurrency limits & retries. | 🌱 `cattery` repo: `packages/cattery-queue/` |
| 27 | **`cattery-storage`** | Storage Extensions | Unified cloud object storage client (AWS S3, Google Cloud Storage, Cloudflare R2). | 🌱 `cattery` repo: `packages/cattery-storage/` |
| 28 | **`cattery-events`** | Storage Extensions | In-memory pub/sub event bus with asynchronous dispatch and topic filters. | 🌱 `cattery` repo: `packages/cattery-events/` |
| 29 | **`cattery-graphql`** | Advanced Protocols | Schema-first GraphQL parser, query validation, and async resolver execution. | 🌱 `cattery` repo: `packages/cattery-graphql/` |
| 30 | **`cattery-grpc`** | Advanced Protocols | Binary Protobuf serialization and gRPC streaming client/server transport. | 🌱 `cattery` repo: `packages/cattery-grpc/` |
| 31 | **`cattery-oauth`** | Advanced Protocols | Turnkey OAuth2 and OpenID Connect (OIDC) authentication flows (GitHub, Google, Apple). | 🌱 `cattery` repo: `packages/cattery-oauth/` |
| 32 | **`cattery-ws-client`** | Advanced Protocols | Resilient client-side WebSocket manager with automatic exponential backoff reconnect. | 🌱 `cattery` repo: `packages/cattery-ws-client/` |
| 33 | **`cattery-image`** | Processing & Media | Native image resizing, cropping, compression, and WebP/PNG/JPEG conversion. | 🌱 `cattery` repo: `packages/cattery-image/` |
| 34 | **`cattery-pdf`** | Processing & Media | Fast programmatic generation of PDF reports, invoices, and documents. | 🌱 `cattery` repo: `packages/cattery-pdf/` |
| 35 | **`cattery-csv`** | Processing & Media | Streaming tabular data parser for high-throughput bulk imports and exports. | 🌱 `cattery` repo: `packages/cattery-csv/` |
| 36 | **`cattery-yaml`** | Processing & Media | Fast YAML configuration file parser and serializer. | 🌱 `cattery` repo: `packages/cattery-yaml/` |
| 37 | **`cattery-html`** | Processing & Media | Lightweight HTML DOM traversal, CSS-selector querying, and web-scraping engine. | 🌱 `cattery` repo: `packages/cattery-html/` |
| 38 | **`cattery-compression`** | Telemetry & Limits | High-performance gzip, zlib, and Brotli streaming compression/decompression. | 🌱 `cattery` repo: `packages/cattery-compression/` |
| 39 | **`cattery-metrics`** | Telemetry & Limits | Telemetry exporter collecting latency, RSS, and Prometheus-compatible metrics. | 🌱 `cattery` repo: `packages/cattery-metrics/` |
| 40 | **`cattery-audio`** | Domain (Lorikeet) | Native PCM audio buffer playback, recording, resampling, and DSP voice filters. | 🌱 `cattery` repo: `packages/cattery-audio/` |
| 41 | **`cattery-ipa`** | Domain (Lorikeet) | International Phonetic Alphabet (IPA) parsing, syllabification, and phoneme mapping. | 🌱 `cattery` repo: `packages/cattery-ipa/` |
| 42 | **`cattery-asr`** | Domain (Lorikeet) | Local speech-to-text bindings (e.g., `whisper.cpp`) for zero-latency transcription. | 🌱 `cattery` repo: `packages/cattery-asr/` |
| 43 | **`cattery-stream`** | Domain (Lorikeet) | IPTV and HLS stream ingestion with MPEG-TS packet demuxing for media immersion. | 🌱 `cattery` repo: `packages/cattery-stream/` |
| 44 | **`cattery-srs`** | Domain (Lorikeet) | Spaced Repetition System algorithm (SuperMemo SM-2 & FSRS-5) for memory decay modeling. | 🌱 `cattery` repo: `packages/cattery-srs/` |
| 45 | **`cattery-subtitles`**| Domain (Lorikeet) | Subtitle parser and synchronizer supporting SRT, VTT, ASS/SSA with phoneme timing alignment. | 🌱 `cattery` repo: `packages/cattery-subtitles/` |
| 46 | **`cattery-llm`** | AI & Local ML | Embedded local LLM inference bindings (via `llama.cpp`) for on-device reasoning and grammar assistance. | 🌱 `cattery` repo: `packages/cattery-llm/` |
| 47 | **`cattery-tts`** | AI & Local ML | Offline neural Text-to-Speech synthesis (Piper / Sherpa-ONNX) for realistic pronunciation audio. | 🌱 `cattery` repo: `packages/cattery-tts/` |
| 48 | **`cattery-vector`** | AI & Local ML | Vector embeddings, cosine similarity search, and approximate nearest neighbor (ANN/HNSW) indexing. | 🌱 `cattery` repo: `packages/cattery-vector/` |
| 49 | **`cattery-ocr`** | AI & Local ML | Embedded Optical Character Recognition (Tesseract / ONNX vision) for document and image scanning. | 🌱 `cattery` repo: `packages/cattery-ocr/` |
| 50 | **`cattery-sse`** | Core Networking | Server-Sent Events (SSE) streaming protocol manager for low-overhead server push. | 🌱 `cattery` repo: `packages/cattery-sse/` |
| 51 | **`cattery-dns`** | Core Networking | Asynchronous non-blocking DNS resolution and DNS-over-HTTPS (DoH) client. | 🌱 `cattery` repo: `packages/cattery-dns/` |
| 52 | **`cattery-mqtt`** | Advanced Protocols | Lightweight MQTT 3.1.1 / 5.0 client for IoT messaging and real-time event brokering. | 🌱 `cattery` repo: `packages/cattery-mqtt/` |
| 53 | **`cattery-smtp`** | Advanced Protocols | Non-blocking SMTP email client with MIME multi-part attachments and TLS encryption. | 🌱 `cattery` repo: `packages/cattery-smtp/` |
| 54 | **`cattery-totp`** | Security & Testing | Time-based and HMAC-based One-Time Password (TOTP / HOTP) two-factor authentication engine. | 🌱 `cattery` repo: `packages/cattery-totp/` |
| 55 | **`cattery-crypto-keys`** | Security & Testing | Key derivation (Argon2id, PBKDF2) and native OS credential vault / keychain access. | 🌱 `cattery` repo: `packages/cattery-crypto-keys/` |
| 56 | **`cattery-zip`** | Processing & Archive | High-throughput ZIP and TAR archive decompression and streaming archive creation. | 🌱 `cattery` repo: `packages/cattery-zip/` |
| 57 | **`cattery-toml`** | Processing & Media | Fast TOML configuration file parser and serializer for package and build manifests. | 🌱 `cattery` repo: `packages/cattery-toml/` |
| 58 | **`cattery-xml`** | Processing & Media | Streaming XML/RSS/Atom feed parser and DOM serializer for media metadata and EPG. | 🌱 `cattery` repo: `packages/cattery-xml/` |
| 59 | **`cattery-gpio`** | Hardware & IoT | Memory-mapped GPIO, I2C, SPI, and PWM pin control for Raspberry Pi and embedded devices. | 🌱 `cattery` repo: `packages/cattery-gpio/` |
| 60 | **`cattery-serial`** | Hardware & IoT | Cross-platform RS-232 / USB serial port communication for microcontrollers and peripherals. | 🌱 `cattery` repo: `packages/cattery-serial/` |
| 61 | **`cattery-bluetooth`**| Hardware & IoT | Native Bluetooth Low Energy (BLE) scanning, GATT client connection, and characteristic streaming. | 🌱 `cattery` repo: `packages/cattery-bluetooth/` |
| 62 | **`cattery-dialog`** | Desktop UI | Native operating system file pickers, alert dialogs, and folder selection for `dmstct-keety`. | 🌱 `cattery` repo: `packages/cattery-dialog/` |
| 63 | **`cattery-jsondb`** | Data & Storage | Ultra-lightweight schema-validated flat-file & in-memory JSON document database. | 🌱 `cattery` repo: `packages/cattery-jsondb/` |
