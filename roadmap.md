# Keet & Cattery Development Roadmap 🦜🐱🐾

An architectural master plan for **Keet**, **KeetScript**, and the **Cattery** native package ecosystem.

---

## 1. Ecosystem Nomenclature & World-Building 🐾

Keet combines bare-metal C++20 performance with expressive, reactive ergonomics through a unified animal-themed tooling suite:

- **🦜 Keet:** The core framework engine. A zero-runtime-overhead C++20 full-stack micro-framework bundling a concurrent multi-threaded HTTP/WebSocket server, embedded SQLite database, and server-driven Virtual DOM renderer in a sub-2MB binary footprint.
- **🐱 `keety` ("little cat"):** The ahead-of-time (AOT) compiler transpiling `.keet` Single-File Components (SFCs) and `<server>` orchestration blocks into optimized C++20 header/source pairs during CMake builds.
- **📦 `cattery` (`cattery-fx`):** The native package ecosystem and monorepo standard library where developers **"adopt"** zero-dependency C++ and KeetScript libraries.
- **🐈📦 `litterbox`:** A lightweight, dedicated developer workspace environment tailored for KeetScript full-stack projects.
- **🐈💨 `Zoomies`:** The lightning-fast local development watcher (`keet dev`) recompiling modified SFC components and pushing surgical VDOM patches over WebSockets in under 50ms without clearing session state or refreshing browsers.
- **🏠🐱 `dmstct-keety` ("domesticate kitty"):** The native desktop application packager linking lightweight OS system webviews (WebKit, WebView2, WebKitGTK) directly to Keet's native C++ core (<5MB binaries, <10MB RAM).

---

## 2. Completed Milestones ✅

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

## 3. Master Engineering Sequence & Architecture Roadmap 🚀

Following the code review and landscape analysis, the roadmap orders features by strict architectural prerequisites:
**Testing and Data Safety must land before higher-level language features and ecosystem packages.**

```
Phase I: Core Hardening & Quality Gate
├── Feature 6: Test Scaffolding & Headless Testing Harness (keet test)
├── Feature 7: Embedded SQLite Database Engine (True SQL in <server>)
└── Feature 8: Session Security, Persistent Storage & Auth Guards

Phase II: Language, Compiler & Reactive UI
├── Feature 9: Declarative Multi-Page Router Extensions (keet/Router.h)
├── Feature 10: Multi-Component Composition & .keet Imports
├── Feature 11: Server-Driven Slots & Content Projection (<slot />)
├── Feature 12: Schema-Driven Form Validation & Model Binding
└── Feature 13: Instant Live Reload — "Zoomies" 🐈💨

Phase III: Next-Gen Compiler & Developer Environment
├── Feature 14: Standalone KeetScript Language & keety-next (Pure Modules, .h/.cpp Libs, Borrow Checker)
└── Feature 15: Native Language Server Protocol (LSP) Daemon & litterbox 🐈📦

Phase IV: Desktop Packaging & Advanced Ecosystem
├── Feature 16: Native Desktop Packaging (dmstct-keety) 🏠🐱
├── Feature 17: Network Client Hardening & Automated Missing Key Translation Engine
├── Feature 18: Asynchronous Coroutines & Event Loops (kqueue / io_uring)
└── Feature 19: Universal Data Connectors & Embedded Document Database (cattery-jsondb, cattery-sql-client, cattery-mongo)

Phase V: The Cattery (cattery-fx) Monorepo Standard Library
└── 63 Foundational Native Packages
```

---

### Phase I: Core Hardening & Quality Gate

#### Feature 6: Test Scaffolding & Headless Component Testing Harness (`keet test` / `cattery-test`) [Completed ✅]
- **Status:** Completed ✅
- **Addresses:** Landscape report step 1; lack of automated regression tests and sanitizers.
- **Description:** Establishes the automated testing foundation for the Keet framework. Integrates CTest, compiles unit tests under AddressSanitizer (ASan), UndefinedBehaviorSanitizer (UBSan), and ThreadSanitizer (TSan), wires `tools/keety/test_ast.cpp`, and provides a headless native C++ VDOM test harness (`keet/Testing.h`) to verify component rendering and event dispatching without opening network sockets or browser instances.
- **Phases:**
    - **Phase 1: CMake & CTest Scaffolding + Sanitizers** [Completed ✅]
        - 1.1: Enable CTest in root `CMakeLists.txt` and wire `tools/keety/test_ast.cpp` into an executable test target. [Completed ✅]
        - 1.2: Add CMake options for ASan, UBSan, and TSan builds (`ENABLE_ASAN`, `ENABLE_TSAN`, `ENABLE_UBSAN`). [Completed ✅]
        - 1.3: Establish a lightweight, zero-dependency C++ unit assertion harness for engine regression testing. [Completed ✅]
    - **Phase 2: Headless VDOM Test Renderer & Simulator (`keet/Testing.h`)** [Completed ✅]
        - 2.1: Implement `TestHarness::mount<Component>(props)` to render components headlessly into an inspectable VNode hierarchy. [Completed ✅]
        - 2.2: Add CSS-selector query helpers (`find(".btn")`, `findAll("li")`, `text()`) and assertion macros (`KEET_ASSERT_TEXT`, `KEET_ASSERT_ATTR`). [Completed ✅]
        - 2.3: Provide synthetic event dispatchers (`triggerClick()`, `triggerInput("value")`) that execute bound C++ lambdas synchronously and return updated VNodes. [Completed ✅]
    - **Phase 3: Integration Runner & CLI (`keet test`)** [Completed ✅]
        - 3.1: Support ephemeral `:memory:` database provisioning and mock `Session` contexts for end-to-end integration tests. [Completed ✅]
        - 3.2: Add `keet_add_test()` macro to CMake to compile and execute `.test.keet` components with formatted test reporting. [Completed ✅]
        - 3.3: Implement VNode snapshot testing (`assertMatchesSnapshot()`) to prevent unintended UI or scoped CSS regressions. [Completed ✅]

---

#### Feature 7: Embedded SQLite Database Engine (True SQL in `<server>` / `cattery-db`)
- **Status:** High Priority ⏳ (Addresses Core Concurrency & Data Safety: F-01, F-02, F-12)
- **Description:** Replaces the prototype file-backed JSON database with SQLite bundled directly as a single `sqlite3.c` amalgamation with zero external runtime dependencies. Eliminates table-copy concurrency races across worker threads (`std::shared_mutex` / WAL mode), guarantees atomic writes with ACID durability, applies column `default` modifiers, and empowers developers to declare relational schemas directly in `<server>` with raw SQL query execution and fluent query helpers.
- **Phases:**
    - **Phase 1: SQLite Amalgamation & Thread-Safe C++ Engine**
        - 1.1: Integrate SQLite 3 amalgamation (`sqlite3.c` / `sqlite3.h`) into `src/` and `include/keet/` with zero third-party dependencies.
        - 1.2: Implement `SqliteDatabase` wrapper featuring RAII connection management, WAL mode concurrency, prepared statement caching, and parameterized value binding.
        - 1.3: Expose high-performance query execution APIs returning `Json::Array` result sets while maintaining backward compatibility with the fluent `TableProxy` interface.
    - **Phase 2: KeetScript `<server>` SQL Grammar & Schema Codegen**
        - 2.1: Extend `<server>` table grammar to support standard SQL column types (`text`, `integer`, `real`, `blob`) and constraints (`primary_key`, `auto_increment`, `not_null`, `unique`, `default`, `foreign_key`, `check`).
        - 2.2: Ensure column `default` modifiers are correctly parsed and emitted into table definitions and insert statements.
        - 2.3: Add raw SQL query execution syntax in backend route handlers (`db.query("SELECT * FROM tasks WHERE completed = false")`).
        - 2.4: Generate automated DDL migration statements (`CREATE TABLE IF NOT EXISTS`, index creation) during startup in `Codegen::_generateAppMain()`.

---

#### Feature 8: Built-in Authentication, Session Persistence & Security Guards (`cattery-jwt`)
- **Status:** High Priority ⏳ (Addresses Session Vulnerabilities: F-06, F-05, F-10)
- **Description:** Elevates Keet's in-memory session subsystem into a production-ready authentication and security layer. Replaces non-cryptographic session generation with OS CSPRNG (`getrandom` / `/dev/urandom`), enforces `HttpOnly` and `Secure` cookie attributes, guards against header injection (`\r\n` validation), implements persistent SQLite-backed session storage, CSRF protection, and declarative route/component authorization guards (`@authenticated`, `@role("admin")`).
- **Phases:**
    - **Phase 1: Secure Session Store & Cryptographic Utilities (`keet/Auth.h`)**
        - 1.1: Issue cryptographically secure 128-bit session IDs using OS CSPRNG (`getrandom`), rejecting untrusted client-forged `X-Session-Id` headers.
        - 1.2: Enforce strict cookie security flags (`HttpOnly`, `SameSite=Lax`, `Secure` when TLS is active) and sanitize response headers against CR/LF injection.
        - 1.3: Abstract session storage behind `ISessionStore` with a built-in SQLite-backed implementation (`SqliteSessionStore`) supporting sliding TTLs and server restarts without user session loss.
        - 1.4: Implement automatic CSRF token generation and validation for state-mutating requests (POST, PUT, DELETE).
    - **Phase 2: Declarative Auth Guards in `<server>` & `<script>`**
        - 2.1: Extend `<server>` route grammar with declarative security annotations (`@authenticated`, `@role("admin")`, `@rate_limit(60/m)`) compiled directly into middleware pipeline checks.
        - 2.2: Expose `Auth::currentUser()`, `Auth::login()`, and `Auth::logout()` primitives inside component `<script>` blocks and backend handlers.
        - 2.3: Support conditional UI rendering based on authentication state and automated login redirect handling.

---

### Phase II: Language, Compiler & Reactive UI

#### Feature 9: Declarative Multi-Page Router Extensions & URL State Synchronization (`keet/Router.h` / `cattery-router`)
- **Status:** Medium Priority ⏳
- **Description:** Builds upon Keet's existing foundational exact-path router (`Router`, `Link`) by extending it with full multi-page navigation ergonomics. Introduces dynamic route parameters (`:id`), wildcard catch-alls (`*`), nested layout routes, reactive query parameter binding (`bind:query`), and programmatic navigation synchronized with the browser History API (`pushState` / `popstate`).
- **Phases:**
    - **Phase 1: Dynamic Parameter Matching & Route Context**
        - 1.1: Extend `Router` and `RouteMatch` in `keet/Router.h` to support dynamic parameterized segments (`/users/:id`), wildcard segments (`/docs/*`), and query string extraction.
        - 1.2: Expose matched route parameters and query dictionaries directly to components via `useRoute()` and programmatic `navigate("/path")` helpers.
        - 1.3: Ensure deep-linking via HTTP GET renders the targeted nested route on initial page load with correct server-rendered HTML.
    - **Phase 2: Client History Integration & KeetScript Directives**
        - 2.1: Enhance the client JS runtime to intercept `<Link>` navigation and `popstate` events, dispatching route changes over WebSocket/HTTP while updating the browser URL bar via `history.pushState`.
        - 2.2: Add compile-time route validation and `<Router>`, `<Route>`, and `<Link>` syntax sugar to `keety`.
        - 2.3: Support two-way reactive query parameter synchronization (`bind:query="searchQuery"`) between component state and URL parameters.

---

#### Feature 10: Multi-Component Composition & `.keet` Imports
- **Status:** Medium Priority ⏳
- **Description:** Enables modular, reusable UI hierarchies by allowing `.keet` files to import and nest other `.keet` Single-File Components directly inside `<template>` markup (e.g., `<Navbar title="Dashboard" />`, `<Card item={user} />`). Extends `keety` to parse component import directives, distinguish between native HTML elements and custom component tags via PascalCase naming conventions, map dynamic expressions to component props, and automate multi-file dependency resolution in CMake.
- **Phases:**
    - **Phase 1: Component Import Grammar & Prop Declarations**
        - 1.1: Extend `<script>` grammar to support component imports (`import ComponentName from "./path/to/Component.keet"`).
        - 1.2: Add explicit prop declarations in `<script>` (`prop title: string;`, `prop count: int = 0;`) mapping to `Component(UIEngine::Props props)` and typed member variables.
        - 1.3: Update AST nodes and parser routines in `tools/keety` (`ImportKeetDecl`, `PropDecl`).
    - **Phase 2: Tag Resolution & AOT Code Generation**
        - 2.1: Enhance template parser and code generator to identify custom component tags via uppercase naming conventions (PascalCase vs lowercase HTML tags).
        - 2.2: Implement AOT code generation in `Codegen.cpp` to emit `#include "Component.h"` and instantiate child component instances (`parentVec.push_back(Generated::ComponentName{...})`).
        - 2.3: Support passing static attributes, dynamic expressions (`title={pageTitle}`), and callback delegates into child component props.
    - **Phase 3: Build System & Tooling Integration**
        - 3.1: Update `KeetCompiler.cmake` (`keet_add_application`) to recursively discover imported `.keet` component dependencies and compile them in topological order.
        - 3.2: Update TextMate grammar and VS Code extension to provide auto-completion, import resolution, and diagnostics for nested component tags and props.

---

#### Feature 11: Server-Driven Slots & Content Projection (`<slot />`)
- **Status:** Medium Priority ⏳ (Depends on Feature 10)
- **Description:** Enables flexible component composition and layout wrapping by allowing components to declare `<slot />` placeholders that receive arbitrary child elements passed from caller components (e.g., `<Modal><h2>Confirm</h2><p>Are you sure?</p></Modal>`). Transpiles slot projection directly into C++ VNode tree construction with zero runtime overhead.
- **Phases:**
    - **Phase 1: Slot Grammar, AST & Fallback Content**
        - 1.1: Add default `<slot />` and named slots (`<slot name="..." />`) to KeetScript template grammar and AST.
        - 1.2: Support fallback default content inside slot elements (`<slot><p>Default content</p></slot>`) rendered when no caller children are passed.
        - 1.3: Update `tools/keety` parser to validate slot placements and child elements in component invocation tags.
    - **Phase 2: AOT Codegen & Child Projection**
        - 2.1: Update `Codegen.cpp` to pass caller children vectors into child component constructors or slot rendering delegates.
        - 2.2: Implement named slot distribution (e.g., `<template slot="header">`, `<template slot="footer">`) projecting matching child trees into corresponding designated slots.
        - 2.3: Ensure scoped CSS rules declared on projected slot children and parent containers are harvested and scoped correctly.

---

#### Feature 12: Schema-Driven Form Validation & Reactive Model Binding (`cattery-validate`)
- **Status:** Medium Priority ⏳ (Depends on Features 7 & 10)
- **Addresses:** F-11 form submit reset loss; ties UI forms to SQLite database schemas.
- **Description:** Bridges `<server>` database table schemas and `<template>` form inputs to eliminate boilerplate validation logic. Derives both client-side HTML5 validation attributes and strict server-side validation rules directly from SQLite table constraints or explicit `<script>` validation schemas, providing instant field-level error feedback and type-safe form submission handlers without erasing user input on errors.
- **Phases:**
    - **Phase 1: Validation Rules Engine & Schema Derivation (`keet/Validation.h`)**
        - 1.1: Define `Validator` primitives supporting constraints such as `required`, `min_length`, `max_length`, `pattern`, `email`, and custom predicates.
        - 1.2: Extend `keety` to automatically synthesize `Validator` schemas from `<server>` table declarations and explicit `<script>` model definitions.
        - 1.3: Automatically emit corresponding HTML5 input attributes (`required`, `minlength`, `maxlength`, `pattern`, `type`) on bound `<input>` elements during AOT compilation.
    - **Phase 2: Reactive Error State & Form Submission Lifecycle**
        - 2.1: Add structured form binding directives (`onSubmit`, `bind:form`, `errors.fieldName`) to `<template>` and `<script>`.
        - 2.2: Automatically validate incoming form payloads before executing component or backend mutation logic, populating field-level localized error messages on failure.
        - 2.3: Fix client runtime form reset behavior: reset inputs only upon confirmed server success (fixing F-11).

---

#### Feature 13: Instant Live Reload — "Zoomies" 🐈💨
- **Status:** High DX Priority ⏳
- **Description:** Delivers an instantaneous (<50ms) development feedback loop during local development (`keet dev`). When a developer saves any `.keet`, `.cpp`, or `.h` file:
  1. The cross-platform file system watcher detects changes in milliseconds.
  2. `keety` compiles the translation unit delta.
  3. Surgical Virtual DOM JSON patches are streamed over persistent WebSockets to connected browser sessions.
  4. Component state (`useState`), active form input values, and scroll positions are fully preserved without page refreshes.
- **Phases:**
    - **Phase 1: File Watcher & Dev Server Mode**
        - 1.1: Implement cross-platform file watcher in `keety` (`--watch` / `keet dev`) utilizing `kqueue` on macOS/BSD and `inotify` on Linux.
        - 1.2: Establish a lightweight local reload channel between `keety` and the running frontend server.
        - 1.3: Display in-browser compile diagnostics and syntax error toast overlays when `.keet` parsing fails.
    - **Phase 2: Dynamic Template Reloading & State-Preserving VDOM Patching**
        - 2.1: Implement dynamic component reloading via shared library hot-swapping (`dlopen`) or fast template evaluation to update component rendering trees on the fly.
        - 2.2: Broadcast re-render notifications across active browser sessions and stream surgical VNode patches over the persistent WebSocket connection.
        - 2.3: Preserve component state values (`useState`) and active session scopes across hot reloads.

---

### Phase III: Next-Gen Compiler & Developer Environment

#### Feature 14: Standalone KeetScript Language Evolution & `keety-next` Compiler
- **Status:** Strategic Language & Compiler Milestone ⏳
- **Description:** Elevates KeetScript from an SFC (Single-File Component) web DSL into a **true, standalone general-purpose systems programming language** that compiles directly into standard, idiomatic C++20 header (`.h`) and implementation (`.cpp`) files. Developers can write pure business logic, data models, algorithms, and modular libraries—including the 60 Cattery standard library packages—entirely in KeetScript without requiring `<template>`, `<style>`, or `<server>` markup. Introduces a compile-time borrow checker in `keety` to guarantee memory safety at build time with zero runtime garbage collection overhead.
- **Phases:**
    - **Phase 1: Pure Module Grammar & Standalone Language Syntax (Beyond SFCs)**
        - 1.1: Support pure KeetScript source files without XML section tags—enabling top-level `module`, `import`, `export`, `struct`, `class`, `enum`, `interface`, and `fn` / `def` declarations.
        - 1.2: Expand the type system: strong static typing, type inference, algebraic data types (enums with values / sum types), generics/templates (`List<T>`, `Result<T, E>`), and pattern matching.
        - 1.3: Enable writing non-UI business logic, standard libraries, CLI tools, and `cattery-*` packages entirely in KeetScript.
    - **Phase 2: Standalone C++ Library Code Generator (`keety --lib`)**
        - 2.1: Implement Library Compilation Mode (`keety --lib -i Module.keet -o Module.cpp -h Module.h`) emitting clean, idiomatic C++20 header (`.h`) and implementation (`.cpp`) pairs with `#pragma once`, clean namespaces, and typed public API exports.
        - 2.2: Dual Compilation Modes: `keety` intelligently differentiates between pure language modules (`.keet` without `<template>`) producing C++20 libraries, and UI components (with `<template>`) producing `UIEngine::Component` classes.
        - 2.3: CMake integration: add `keet_add_library(target_name Sources.keet)` producing native CMake C++ library targets linkable by any pure C++ application or Keet project.
    - **Phase 3: Compile-Time Memory Safety (Borrow Checker)**
        - 3.1: Evolve KeetScript's type system with ownership, move semantics, and reference lifetime annotations.
        - 3.2: Implement compile-time borrow checker in `keety` detecting use-after-free, double frees, and dangling references before emitting C++ code.
        - 3.3: Guarantee zero runtime performance penalty or garbage collection pauses in emitted C++ binaries.
    - **Phase 4: Binary AST Representation & Next-Gen Incremental Pipeline (`keety-next`)**
        - 4.1: Design compact binary serialization format for KeetScript ASTs to avoid repetitive text lexing and parsing.
        - 4.2: Decouple front-end parser from code generator via structured intermediate representation (IR).
        - 4.3: Enable fine-grained dependency graph tracking to achieve sub-second incremental builds on large projects.
    - **Phase 5: Zero-Cost FFI & Native C/C++ Interoperability**
        - 5.1: Automated header introspection engine parsing C/C++ headers and synthesizing typed KeetScript declarations.
        - 5.2: Direct symbol linking with zero marshaling overhead for native libraries (`import cpp from "Header.h" as Module`).

---

#### Feature 15: Native Language Server Protocol (LSP) Daemon & `litterbox` 🐈📦
- **Status:** Strategic Tooling Enhancement ⏳
- **Description:** Embeds a high-performance Language Server Protocol (LSP) daemon directly into `keety` to deliver enterprise-grade IDE intelligence across VS Code, Neovim, and JetBrains CLion. Integrates into `litterbox`, a lightweight dedicated developer environment tailored for KeetScript projects.
- **Phases:**
    - **Phase 1: Embedded `keety` LSP Server**
        - 1.1: Implement JSON-RPC 2.0 communication over standard I/O in `keety --lsp`.
        - 1.2: Real-time incremental document synchronization and semantic diagnostic reporting.
        - 1.3: Context-aware code completion for KeetScript tags, directives (`bind:*`), props, and `<server>` SQL tables.
    - **Phase 2: Semantic Analysis & Navigation**
        - 2.1: Implement Go-to-Definition and Find References across `.keet` component imports and native C++ headers.
        - 2.2: Hover documentation displaying inferred variable types and component prop contracts.
        - 2.3: Safe symbol renaming across multiple `.keet` files.
    - **Phase 3: `litterbox` Developer Workspace**
        - 3.1: Standalone developer workspace environment managing local compiler toolchains, SDKs, and adopted Cattery packages.
        - 3.2: Project scaffolding wizard (`litterbox init`, `litterbox dev`, `litterbox build`).

---

### Phase IV: Desktop Packaging & Advanced Ecosystem

#### Feature 16: Native Desktop Packaging (`dmstct-keety`) 🏠🐱
- **Status:** Production Distribution Milestone ⏳
- **Description:** A dedicated standalone packaging and distribution tool that "domesticates" web-based Keet full-stack applications into standalone native desktop executables (macOS `.app`, Linux AppImage/deb, Windows `.exe`). Instead of bundling heavy Chromium and Node.js runtimes (Electron: 150MB+ disk, 80MB+ RAM), `dmstct-keety` links lightweight OS system webviews (WebKit on macOS, WebKitGTK on Linux, Edge WebView2 on Windows) directly to Keet's C++ core (<5MB binary, <10MB idle RAM, <100ms launch).
- **Phases:**
    - **Phase 1: Native System Webview Window Harness & Platform Sockets**
        - 1.1: Integrate zero-dependency C++ system webview windowing wrapper (Cocoa/WebKit on macOS, WebKitGTK on Linux, Edge WebView2 on Windows).
        - 1.2: Cross-platform socket layer: implement Winsock abstraction for Windows while preserving POSIX sockets on macOS and Linux.
        - 1.3: Direct in-memory IPC and custom WebView URI schemes (`WKScriptMessageHandler` / `WebMessageReceived`) to eliminate loopback TCP overhead and local firewall prompts.
        - 1.4: Graceful shutdown handling on window close or `SIGINT`/`SIGTERM` to ensure SQLite transactions commit and thread pools drain cleanly.
    - **Phase 2: `dmstct-keety` CLI Bundler & Build Tooling**
        - 2.1: Standalone `dmstct-keety` CLI supporting `package`, `build`, and `run` commands.
        - 2.2: Automated packaging pipeline emitting signed macOS `.app` bundles (with `Info.plist` and `.icns`), Linux AppImages, and Windows PE executables.
        - 2.3: CMake targets (`keet_add_desktop_app(target_name App.keet)`) for single-command builds.
    - **Phase 3: Native Desktop Hardware & OS Integrations**
        - 3.1: Expose native system file picker dialogs, message boxes, and folder selection directly into `<script>`.
        - 3.2: Implement system tray and menu bar integration with minimize-to-tray and custom menus.
        - 3.3: Native desktop push notification support (`NSUserNotification` on macOS, `libnotify` on Linux, Action Center on Windows).

---

#### Feature 17: Network Client Hardening & Automated Missing Key Translation Engine (`cattery-http`)
- **Status:** Advanced Localization & Transport ⏳ (Addresses F-13, F-14)
- **Description:** Upgrades `UIEngine::fetch()` with optional OpenSSL TLS/HTTPS and chunked transfer decoding (F-13) and fixes French/Slavic plural evaluation corner cases (F-14). On this hardened transport, establishes an extensible background localization service that intercepts unlocalized or missing catalog keys at runtime. Returns a non-blocking fallback frame, dispatches an asynchronous background translation request to cloud providers (DeepL/LibreTranslate) or local neural engines, auto-persists translations to `locales/<target>.json`, and reconciles active sessions dynamically.
- **Phases:**
    - **Phase 1: HTTP Client Hardening (`UIEngine::fetch` / `cattery-http`)**
        - 1.1: Decode chunked HTTP bodies (`Transfer-Encoding: chunked`) and correctly honor `Content-Length`.
        - 1.2: Add optional OpenSSL TLS support for `https://` requests in `UIEngine::fetch()`.
        - 1.3: Fix language-specific plural rule evaluation order for zero counts matching CLDR (F-14).
    - **Phase 2: Translation Provider Abstraction & Asynchronous Queue**
        - 2.1: Define abstract `ITranslationProvider` interface in `keet/Localization.h` supporting cloud endpoints and local neural models.
        - 2.2: Implement lock-free background translation queue in `Localization.cpp` that batches requests without stalling render threads.
        - 2.3: Return active language-tag fallback strings on frame evaluation while enqueueing the background translation task.
    - **Phase 3: Disk Auto-Persistence & Session Reconciliation**
        - 3.1: Implement atomic disk file serialization (`std::filesystem::rename` write-swap) for updated catalogs in `locales/<locale>.json`.
        - 3.2: Broadcast session-level invalidations to re-render active sessions subscribed to the updated locale once a translation completes.

---

#### Feature 18: Asynchronous Coroutines & Modern Event Loops (`cattery-async`)
- **Status:** Concurrency & Scale Architecture ⏳
- **Description:** Evolves `keet::Server` from thread-per-connection / blocking worker pools into a high-throughput, non-blocking asynchronous event loop powered by C++20 stackless coroutines (`co_await`) paired with modern kernel I/O multiplexing (`kqueue` on macOS/BSD, `io_uring` / `epoll` on Linux). Enables tens of thousands of concurrent WebSocket connections with minimal thread overhead.
- **Phases:**
    - **Phase 1: Event Loop Core Abstraction**
        - 1.1: Abstract non-blocking I/O multiplexer with `kqueue` (macOS) and `epoll` / `io_uring` (Linux) backends.
        - 1.2: Replace blocking worker accept queues with asynchronous socket readiness notifications.
    - **Phase 2: C++20 Coroutine Integration**
        - 2.1: Implement awaitable socket read/write primitives (`co_await socket.read()`, `co_await socket.write()`).
        - 2.2: Support coroutine route handlers in `<server>` for seamless non-blocking database queries and external HTTP calls.

---

#### Feature 19: Universal Data Connectors & Embedded Document Database (`cattery-jsondb`, `cattery-sql-client`, `cattery-mongo`)
- **Status:** Strategic Data Milestone ⏳
- **Description:** Expands Keet's database capabilities beyond local embedded SQLite by introducing enterprise external database connectivity (PostgreSQL, MySQL/MariaDB, MongoDB) and revisiting the native Keet JSON-based database concept as an ultra-lightweight, dedicated document store package (`cattery-jsondb`) tailored for flat-file CMS, embedded devices, and rapid prototyping without SQL overhead.
- **Phases:**
    - **Phase 1: `cattery-jsondb` — Native Schema-Validated JSON Document Database (Revisited)**
        - 1.1: Re-architect the original flat-file JSON database concept into a thread-safe, append-only log / memory-mapped document store (`cattery-jsondb`).
        - 1.2: Implement flexible runtime schema validation (via `cattery-validate`), BSON/JSON binary indexing, nested document queries (`doc.get("profile.address.city")`), and atomic ACID commit durability without requiring SQL schemas.
        - 1.3: Expose reactive change streams allowing UI components to subscribe to document mutations.
    - **Phase 2: `cattery-sql-client` — External SQL Database Connectors (PostgreSQL & MySQL)**
        - 2.1: Native wire-protocol clients and connection pool management for PostgreSQL (wire format / `libpq`) and MySQL/MariaDB.
        - 2.2: Parameterized query execution, streaming cursor result sets, and async connection leasing across worker threads.
        - 2.3: Cross-database transaction abstraction and unified connection string parsing (`postgres://user:pass@host:5432/dbname`).
    - **Phase 3: `cattery-mongo` — Native MongoDB Wire Client**
        - 3.1: Lightweight native BSON encoder/decoder and MongoDB wire protocol implementation (OP_MSG).
        - 3.2: Collection-based CRUD operations, aggregation pipelines, document projections, and replica-set reconnect handling.
        - 3.3: First-class KeetScript `<server>` integration: `mongo = Mongo.connect("mongodb://localhost:27017")`.

---

## 4. Phase V: The Cattery (`cattery-fx`) Package Roadmap 📦

To solve ecosystem dependencies without relying on heavy, dynamic runtimes like npm or pip, Keet utilizes **Cattery (`cattery-fx`)**—a native package manager and monorepo standard library. Developers **adopt** packages into their projects with zero external build tools.

### 4.1. Packages Currently Integrated in Keet (Separation & Upgrade Plan) 🛠️

Thirteen foundational modules currently reside directly inside the monolithic `keet` core repository (`src/`, `include/keet/`, and `tools/keety/`). These must be decoupled into independent, reusable `cattery-*` packages and upgraded with production-grade capabilities:

1. **`cattery-db` (from `include/keet/Database.h` & `src/Database.cpp`)**
   - **Current State:** Monolithic file-backed JSON document database using `TableProxy` with in-memory tables.
   - **Why Separate:** Concurrency bugs, unindexed O(N) operations, and lack of transaction safety block multi-threaded scaling.
   - **Upgrade Required:** Replace JSON storage with an in-tree SQLite 3 amalgamation (`sqlite3.c`), WAL-mode multi-threading, ACID transactions, column default constraints, and a fluent type-safe SQL query builder (resolves **F-01, F-02, F-12**).

2. **`cattery-http` (from `include/keet/UIEngine.h` & `src/UIEngine.cpp`)**
   - **Current State:** Embedded synchronous POSIX socket HTTP/1.1 client in `UIEngine::fetch()`.
   - **Why Separate:** Tightly coupled to the UI engine, blocking render threads during network operations.
   - **Upgrade Required:** Decouple into a standalone non-blocking HTTP/HTTPS client, add optional OpenSSL TLS support, decode `Transfer-Encoding: chunked` payloads, and honor `Content-Length` headers properly (resolves **F-13**).

3. **`cattery-router` (from `include/keet/UIEngine.h`, `src/UIEngine.cpp` & `src/Server.cpp`)**
   - **Current State:** Exact-string-matching `Router` and `Link` components in `UIEngine`, and manual route registration in `Server`.
   - **Why Separate:** Routing logic is duplicated between client VDOM history navigation and backend HTTP dispatching.
   - **Upgrade Required:** Unify into a single high-performance router supporting dynamic parameterized paths (`/users/:id`), wildcard segments (`/docs/*`), query string parsing, regex constraints, and nested route layout middleware.

4. **`cattery-ws` (from `include/keet/WebSocket.h` & `src/WebSocket.cpp`)**
   - **Current State:** Server-side RFC 6455 WebSocket framing and connection management wired directly into `HTTPServer::Server`.
   - **Why Separate:** Prevents non-server CLI tools or external clients from utilizing Keet's WebSocket framing engine.
   - **Upgrade Required:** Separate frame encoding/decoding into a standalone protocol library, add automated ping/pong keep-alive heartbeats, and support client-side connection management (`cattery-ws-client`).

5. **`cattery-tls` (from `include/keet/Crypto.h` & `src/Crypto.cpp`)**
   - **Current State:** Minimal helper routines limited to SHA-1 and Base64 encoding used exclusively for the WebSocket handshake.
   - **Why Separate:** Security functions should not be locked inside internal server handshake logic.
   - **Upgrade Required:** Expand into a full native cryptographic toolkit: SHA-256, HMAC token signing, OS CSPRNG entropy (`getrandom` / `/dev/urandom`), and zero-overhead OpenSSL/mbedTLS stream wrappers (resolves **F-06**).

6. **`cattery-json` (from `include/keet/Json.h` & `src/Json.cpp`)**
   - **Current State:** DOM-style JSON parser/serializer using `std::variant`.
   - **Why Separate:** Used across every layer (database, server, UI VDOM) but lacks schema mapping and zero-copy slicing.
   - **Upgrade Required:** Extract into an independent high-speed parsing library with `std::string_view` zero-copy tokenization, strict 400 error handling on malformed syntax (resolving **F-09**), and compile-time struct reflection mapping.

7. **`cattery-async` (from `include/keet/Server.h` & `src/Server.cpp`)**
   - **Current State:** Fixed worker thread pool with condition variables embedded inside `HTTPServer::Server`.
   - **Why Separate:** Thread pool and task scheduling cannot be reused by database jobs, translation queues, or background tasks.
   - **Upgrade Required:** Extract into a general-purpose concurrency package providing thread pools, task schedulers, future/promise chains, channels, and C++20 coroutine primitives (`co_await`).

8. **`cattery-log` (from `include/keet/Middleware.h` & `src/Middleware.cpp`)**
   - **Current State:** Synchronous console output (`std::cout`, `std::cerr`) with basic middleware request logging.
   - **Why Separate:** Unstructured terminal logging impacts server throughput under heavy request loads.
   - **Upgrade Required:** Provide non-blocking asynchronous structured JSON logging, configurable log levels (DEBUG, INFO, WARN, ERROR), file rotation, and correlation IDs for error tracking (resolves **F-08**).

9. **`cattery-cli` (from `tools/keety/main.cpp`)**
   - **Current State:** Ad-hoc manual `argv` string checking in the `keety` compiler entry point.
   - **Why Separate:** Every future CLI tool (`dmstct-keety`, `keet dev`, `litterbox`) will re-implement argument parsing.
   - **Upgrade Required:** Create a declarative CLI option and flag parser with automatic `--help` generation, type validation, subcommands, and shell auto-completion.

10. **`cattery-test` (from `tools/keety/test_ast.cpp`)**
    - **Current State:** Single untracked AST test file without a test runner or framework integration.
    - **Why Separate:** Testing is essential across all 60 Cattery packages and user applications.
    - **Upgrade Required:** Implement lightweight native test assertion macros (`KEET_ASSERT_*`), test discovery, fixture lifecycle, mocking utilities, and a clean CTest runner CLI (`keet test`).

11. **`cattery-cache` (from `include/keet/StaticFiles.h`, `src/StaticFiles.cpp` & `src/UIEngine.cpp`)**
    - **Current State:** Isolated, hardcoded in-memory caches for static file ETags and UI session tracking.
    - **Why Separate:** In-memory caching logic is fragmented and lacks standardized eviction policies.
    - **Upgrade Required:** Extract a generic thread-safe LRU/LFU cache with sliding TTL expirations, atomic invalidation, and memory consumption caps.

12. **`cattery-rate-limit` (from `src/Middleware.cpp`)**
    - **Current State:** Naive per-connection request counter in the HTTP middleware chain.
    - **Why Separate:** Does not support distributed backends, WebSocket frame rate limiting, or sophisticated burst algorithms.
    - **Upgrade Required:** Build a dedicated rate-limiting library supporting token-bucket, leaky-bucket, and sliding-window counters with route-level and IP-level granularity.

13. **`cattery-i18n` (from `include/keet/Localization.h` & `src/Localization.cpp`)**
    - **Current State:** Monolithic singleton localization catalog parsing JSON files, evaluating basic plural formulas, and negotiating session locales.
    - **Why Separate:** Localization should be a modular dependency usable across backend microservices, CLI utilities, and desktop apps without pulling in the entire UI engine.
    - **Upgrade Required:** Decouple from UI engine state, conform plural category rules strictly to CLDR specifications (resolving **F-14**), integrate non-blocking missing key resolution queues, and add cloud/neural automated translation provider backends.

---

### 4.2. Foundational Cattery Package Master Registry (60 Packages) 📋

| # | Package Name | Category | Primary Purpose | Current Status & Keet Integration |
|---|---|---|---|---|
| 1 | **`cattery-db`** | Data & Storage | Type-safe query builders, migrations, and ORM abstractions for SQLite. | 🛠️ **Integrated** *(in `Database.h/cpp` — needs SQLite separation & upgrade)* |
| 2 | **`cattery-http`** | Core Networking | High-performance asynchronous HTTP/HTTPS client with TLS & chunked decoding. | 🛠️ **Integrated** *(in `UIEngine::fetch` — needs non-blocking & TLS separation)* |
| 3 | **`cattery-router`** | Core Networking | URL path routing, dynamic parameters (`:id`), wildcards, and middleware stacking. | 🛠️ **Integrated** *(in `Router`/`Link` & `Server` — needs unified separation)* |
| 4 | **`cattery-ws`** | Core Networking | Bidirectional RFC 6455 WebSocket frame manager for real-time streaming. | 🛠️ **Integrated** *(in `WebSocket.h/cpp` — needs standalone protocol separation)* |
| 5 | **`cattery-tls`** | Core Networking | Cryptographic primitives, hashing, CSPRNG, and zero-overhead TLS wrappers. | 🛠️ **Integrated** *(in `Crypto.h/cpp` — needs full crypto & TLS upgrade)* |
| 6 | **`cattery-json`** | Data & Storage | High-speed JSON serialization/deserialization wrapping `keet::Json`. | 🛠️ **Integrated** *(in `Json.h/cpp` — needs schema mapping & zero-copy upgrade)* |
| 7 | **`cattery-async`** | Concurrency & CLI | Task schedulers, thread pools, futures/promises, and coroutines (`co_await`). | 🛠️ **Integrated** *(in `Server.cpp` pool — needs generic scheduler separation)* |
| 8 | **`cattery-log`** | Concurrency & CLI | Structured JSON logging with high-throughput asynchronous file/console sinks. | 🛠️ **Integrated** *(in `Middleware.cpp` — needs async structured logging upgrade)* |
| 9 | **`cattery-cli`** | Concurrency & CLI | Declarative CLI argument/flag parser with auto-generated help menus. | 🛠️ **Integrated** *(in `keety/main.cpp` — needs generic CLI separation)* |
| 10 | **`cattery-test`** | Security & Testing | Lightweight unit-testing assertion library, mocking utilities, and test runner. | 🛠️ **Integrated** *(in `test_ast.cpp` — needs full test harness separation)* |
| 11 | **`cattery-cache`** | Storage Extensions | In-memory key-value cache with LRU eviction, sliding TTLs, and sharded locks. | 🛠️ **Integrated** *(in `StaticFiles`/`Session` — needs unified LRU separation)* |
| 12 | **`cattery-rate-limit`** | Telemetry & Limits | Token-bucket, leaky-bucket, and sliding-window rate limiters for routes and sockets. | 🛠️ **Integrated** *(in `Middleware.cpp` — needs advanced algorithm separation)* |
| 13 | **`cattery-i18n`** | Domain & Locale | Multilingual translation catalogs, CLDR plural rules, and dynamic locale switching. | 🛠️ **Integrated** *(in `Localization.h/cpp` — needs modular separation & upgrade)* |
| 14 | **`cattery-validate`** | Data & Storage | Runtime data validation and parsing using static type schemas and model rules. | 🌱 **New Package** *(Planned)* |
| 15 | **`cattery-env`** | Data & Storage | Safe configuration and `.env` environment variable loading with type coercion. | 🌱 **New Package** *(Planned)* |
| 16 | **`cattery-dash`** | Data Structures | Functional utility toolkits (filtering, mapping, grouping, zip, debounce, throttle). | 🌱 **New Package** *(Planned)* |
| 17 | **`cattery-uuid`** | Data Structures | Fast, cryptographically secure unique identifier generation (UUID v4, UUID v7). | 🌱 **New Package** *(Planned)* |
| 18 | **`cattery-time`** | Data Structures | Timezone-aware date parsing, formatting, duration arithmetic, and cron scheduling. | 🌱 **New Package** *(Planned)* |
| 19 | **`cattery-regex`** | Data Structures | Optimized regular expression pattern matching and lexical tokenizing. | 🌱 **New Package** *(Planned)* |
| 20 | **`cattery-path`** | Concurrency & CLI | Cross-platform file path manipulation, normalization, and directory traversal. | 🌱 **New Package** *(Planned)* |
| 21 | **`cattery-jwt`** | Security & Testing | JSON Web Token issuance, HMAC/RSA signature verification, and stateless auth. | 🌱 **New Package** *(Planned)* |
| 22 | **`cattery-markdown`** | Security & Testing | Ultra-fast Markdown parser emitting native Keet Virtual DOM node hierarchies. | 🌱 **New Package** *(Planned)* |
| 23 | **`cattery-ui-icons`** | Security & Testing | Curated vector icon collection pre-packaged as native Keet UI components. | 🌱 **New Package** *(Planned)* |
| 24 | **`cattery-queue`** | Storage Extensions | In-process and distributed background job queue with concurrency limits & retries. | 🌱 **New Package** *(Planned)* |
| 25 | **`cattery-storage`** | Storage Extensions | Unified cloud object storage client (AWS S3, Google Cloud Storage, Cloudflare R2). | 🌱 **New Package** *(Planned)* |
| 26 | **`cattery-events`** | Storage Extensions | In-memory pub/sub event bus with asynchronous dispatch and topic filters. | 🌱 **New Package** *(Planned)* |
| 27 | **`cattery-graphql`** | Advanced Protocols | Schema-first GraphQL parser, query validation, and async resolver execution. | 🌱 **New Package** *(Planned)* |
| 28 | **`cattery-grpc`** | Advanced Protocols | Binary Protobuf serialization and gRPC streaming client/server transport. | 🌱 **New Package** *(Planned)* |
| 29 | **`cattery-oauth`** | Advanced Protocols | Turnkey OAuth2 and OpenID Connect (OIDC) authentication flows (GitHub, Google, Apple). | 🌱 **New Package** *(Planned)* |
| 30 | **`cattery-ws-client`** | Advanced Protocols | Resilient client-side WebSocket manager with automatic exponential backoff reconnect. | 🌱 **New Package** *(Planned)* |
| 31 | **`cattery-image`** | Processing & Media | Native image resizing, cropping, compression, and WebP/PNG/JPEG conversion. | 🌱 **New Package** *(Planned)* |
| 32 | **`cattery-pdf`** | Processing & Media | Fast programmatic generation of PDF reports, invoices, and documents. | 🌱 **New Package** *(Planned)* |
| 33 | **`cattery-csv`** | Processing & Media | Streaming tabular data parser for high-throughput bulk imports and exports. | 🌱 **New Package** *(Planned)* |
| 34 | **`cattery-yaml`** | Processing & Media | Fast YAML configuration file parser and serializer. | 🌱 **New Package** *(Planned)* |
| 35 | **`cattery-html`** | Processing & Media | Lightweight HTML DOM traversal, CSS-selector querying, and web-scraping engine. | 🌱 **New Package** *(Planned)* |
| 36 | **`cattery-compression`** | Telemetry & Limits | High-performance gzip, zlib, and Brotli streaming compression/decompression. | 🌱 **New Package** *(Planned)* |
| 37 | **`cattery-metrics`** | Telemetry & Limits | Telemetry exporter collecting latency, RSS, and Prometheus-compatible metrics. | 🌱 **New Package** *(Planned)* |
| 38 | **`cattery-audio`** | Domain (Lorikeet) | Native PCM audio buffer playback, recording, resampling, and DSP voice filters. | 🌱 **New Package** *(Planned)* |
| 39 | **`cattery-ipa`** | Domain (Lorikeet) | International Phonetic Alphabet (IPA) parsing, syllabification, and phoneme mapping. | 🌱 **New Package** *(Planned)* |
| 40 | **`cattery-asr`** | Domain (Lorikeet) | Local speech-to-text bindings (e.g., `whisper.cpp`) for zero-latency transcription. | 🌱 **New Package** *(Planned)* |
| 41 | **`cattery-stream`** | Domain (Lorikeet) | IPTV and HLS stream ingestion with MPEG-TS packet demuxing for media immersion. | 🌱 **New Package** *(Planned)* |
| 42 | **`cattery-srs`** | Domain (Lorikeet) | Spaced Repetition System algorithm (SuperMemo SM-2 & FSRS-5) for memory decay modeling. | 🌱 **New Package** *(Planned)* |
| 43 | **`cattery-subtitles`**| Domain (Lorikeet) | Subtitle parser and synchronizer supporting SRT, VTT, ASS/SSA with phoneme timing alignment. | 🌱 **New Package** *(Planned)* |
| 44 | **`cattery-llm`** | AI & Local ML | Embedded local LLM inference bindings (via `llama.cpp`) for on-device reasoning and grammar assistance. | 🌱 **New Package** *(Planned)* |
| 45 | **`cattery-tts`** | AI & Local ML | Offline neural Text-to-Speech synthesis (Piper / Sherpa-ONNX) for realistic pronunciation audio. | 🌱 **New Package** *(Planned)* |
| 46 | **`cattery-vector`** | AI & Local ML | Vector embeddings, cosine similarity search, and approximate nearest neighbor (ANN/HNSW) indexing. | 🌱 **New Package** *(Planned)* |
| 47 | **`cattery-ocr`** | AI & Local ML | Embedded Optical Character Recognition (Tesseract / ONNX vision) for document and image scanning. | 🌱 **New Package** *(Planned)* |
| 48 | **`cattery-sse`** | Core Networking | Server-Sent Events (SSE) streaming protocol manager for low-overhead server push. | 🌱 **New Package** *(Planned)* |
| 49 | **`cattery-dns`** | Core Networking | Asynchronous non-blocking DNS resolution and DNS-over-HTTPS (DoH) client. | 🌱 **New Package** *(Planned)* |
| 50 | **`cattery-mqtt`** | Advanced Protocols | Lightweight MQTT 3.1.1 / 5.0 client for IoT messaging and real-time event brokering. | 🌱 **New Package** *(Planned)* |
| 51 | **`cattery-smtp`** | Advanced Protocols | Non-blocking SMTP email client with MIME multi-part attachments and TLS encryption. | 🌱 **New Package** *(Planned)* |
| 52 | **`cattery-totp`** | Security & Testing | Time-based and HMAC-based One-Time Password (TOTP / HOTP) two-factor authentication engine. | 🌱 **New Package** *(Planned)* |
| 53 | **`cattery-crypto-keys`** | Security & Testing | Key derivation (Argon2id, PBKDF2) and native OS credential vault / keychain access. | 🌱 **New Package** *(Planned)* |
| 54 | **`cattery-zip`** | Processing & Archive | High-throughput ZIP and TAR archive decompression and streaming archive creation. | 🌱 **New Package** *(Planned)* |
| 55 | **`cattery-toml`** | Processing & Media | Fast TOML configuration file parser and serializer for package and build manifests. | 🌱 **New Package** *(Planned)* |
| 56 | **`cattery-xml`** | Processing & Media | Streaming XML/RSS/Atom feed parser and DOM serializer for media metadata and EPG. | 🌱 **New Package** *(Planned)* |
| 57 | **`cattery-gpio`** | Hardware & IoT | Memory-mapped GPIO, I2C, SPI, and PWM pin control for Raspberry Pi and embedded devices. | 🌱 **New Package** *(Planned)* |
| 58 | **`cattery-serial`** | Hardware & IoT | Cross-platform RS-232 / USB serial port communication for microcontrollers and peripherals. | 🌱 **New Package** *(Planned)* |
| 59 | **`cattery-bluetooth`**| Hardware & IoT | Native Bluetooth Low Energy (BLE) scanning, GATT client connection, and characteristic streaming. | 🌱 **New Package** *(Planned)* |
| 60 | **`cattery-dialog`** | Desktop UI | Native operating system file pickers, alert dialogs, and folder selection for `dmstct-keety`. | 🌱 **New Package** *(Planned)* |
| 61 | **`cattery-jsondb`** | Data & Storage | Ultra-lightweight schema-validated flat-file & in-memory JSON document database. | 🌱 **New Package** *(Planned)* |
| 62 | **`cattery-sql-client`**| Data & Storage | Native connection pool & wire-protocol client for external SQL databases (Postgres, MySQL). | 🌱 **New Package** *(Planned)* |
| 63 | **`cattery-mongo`** | Data & Storage | Native MongoDB client supporting BSON wire protocol, document queries, and cursors. | 🌱 **New Package** *(Planned)* |
