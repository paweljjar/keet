# Keet Development Roadmap 🦜

---

## Feature 1: Scoped Component Styles & Automatic Style Harvesting [Completed ✅]
- **Description:** Currently, each component's `styles()` method must be manually merged into the parent component's stylesheet (e.g., `s.merge(Child{}.styles())` in `App::styles()`), and all CSS selectors are emitted into a single global `/styles.css` file. This feature introduces automated traversal to harvest styles from rendered component trees during render passes, as well as automatic CSS class scoping (e.g., using unique component attributes or generated hashes) to prevent style leakage and naming collisions across components.
- **Phases:**
    - **Phase 1: Automatic Tree-Wide Style Harvesting** [Completed ✅]
        - 1.1: Implement recursive VNode tree traversal in `UIEngine` to collect declared `StyleSheet` objects from all active component instances during a render pass.
        - 1.2: Cache and merge collected rules dynamically into the frontend server's `/styles.css` endpoint response without requiring manual parent `merge()` calls.
        - 1.3: Deprecate manual style merging in component parent nodes while maintaining backward compatibility for explicit overrides.
    - **Phase 2: CSS Scoping & Isolation Engine** [Completed ✅]
        - 2.1: Implement automatic scope attribute tagging (e.g., `data-keet-scope="ComponentName#ID"`) on component root VNodes.
        - 2.2: Rewrite and namespace selector rules during CSS serialization so declarations apply strictly within the matching component scope.
        - 2.3: Add support for conditional class directives (e.g., `class:active="isActive"`) directly inside VNode attribute maps.

---

## Feature 2: Built-in Internationalization & Localization (i18n) [Completed ✅]
- **Description:** Provides an out-of-the-box localization subsystem to translate UI text dynamically across multiple languages. The module manages thread-safe JSON dictionary storage, parameter interpolation, pluralization rules, and automatic client locale negotiation via `Accept-Language` headers and cookie overrides without requiring full-page reloads.
- **Phases:**
    - **Phase 1: Core Localization Engine (`keet/Localization.h`)** [Completed ✅]
        - 1.1: Create the `Localization` singleton to parse and store JSON-based translation catalogs (e.g., `locales/en.json`, `locales/pl.json`).
        - 1.2: Implement translation lookup with fallback language resolution and parameter replacement (e.g., `__("welcome", {{"name", "User"}})`).
        - 1.3: Expose ergonomic global translation helpers (`__("key")`) accessible directly within component `render()` methods.
    - **Phase 2: Session Integration & Dynamic Language Switching** [Completed ✅]
        - 2.1: Add locale detection based on the HTTP `Accept-Language` header and session cookie (`lorikeet_locale`).
        - 2.2: Store the active locale within the user's `Session` instance, guarded by `session->mutex`.
        - 2.3: Provide an API endpoint or event trigger to switch the active session locale and immediately re-render the VNode tree with translated strings.

---

## Feature 3: Dedicated KeetScript DSL & `keety` AOT Compiler 🐱 [Completed ✅]
- **Description:** Introduces **KeetScript** (`.keet`), a high-level Single-File Component (SFC) and Full-Stack Application format combining modern frontend ergonomics (state, control flow, scoped CSS) with full backend server and database orchestration. The companion compiler, **`keety`** (*"little cat"* 🐱), transpiles `.keet` files ahead-of-time (AOT) into standard, optimized C++20 code (`.h` / `.cpp` or standalone `main.cpp`), delivering zero runtime overhead and single-command builds.
- **Phases:**
    - **Phase 1: KeetScript Grammar & AST Definition** [Completed ✅]
        - 1.1: Specify `.keet` format containing dedicated sections for state/logic (`let`, `state`, Pythonic control flow), markup layout, and scoped `<style>`.
        - 1.2: Design event expression model for client-side state mutations (`onClick=() => count += 1`, `bind:value=text`).
        - 1.3: Define external C++ bridge syntax (`import cpp from "Header.h" as Module`) for native C++ interoperability.
        - 1.4: Design and implement `<server>` grammar: SQLite `database` tables with schema constraints (`primary_key`, `auto_increment`), REST `backend` microservices, and `frontend` server mounts.
    - **Phase 2: `keety` Compiler CLI Implementation & App Mode** [Completed ✅]
        - 2.1: Build lexer, parser, and code generator for the standalone `keety` executable.
        - 2.2: Implement C++20 code generation emitting `UIEngine::Component` subclasses, static string interning, and pre-allocated child vectors.
        - 2.3: Implement Application Mode (`keety --app`) generating standalone `main.cpp` with automatic database table migrations, backend routing dispatchers, and frontend worker threads.
        - 2.4: Integrate `keety` into CMake via `keet_target_sources()` and `keet_add_application()`.
    - **Phase 3: IDE Tooling & Language Support** [Completed ✅]
        - 3.1: Create TextMate grammar bundle (`tooling/textmate/syntaxes/keet.tmLanguage.json`) for full-stack syntax highlighting.
        - 3.2: Develop and package dedicated Visual Studio Code extension (`tooling/vscode-plugin`) with syntax highlighting, live `keety --lint` diagnostics, snippet library (`keetapp`, `server`, `database`, `backend`), and one-click compilation.

---

## Feature 4: WebSocket Transport & Real-Time Push Engine
- **Description:** Upgrades the client-server interaction channel by adding a persistent, duplex WebSocket transport layer alongside standard HTTP POST polling. This eliminates connection handshake latency, reduces packet overhead on weak networks, and empowers the C++ backend to push instant UI updates, live background job progress, and telemetry to connected browsers without client polling.
- **Phases:**
    - **Phase 1: WebSocket Protocol Layer in `Server`**
        - 1.1: Implement RFC 6455 WebSocket handshaking (`Upgrade: websocket`) and frame parsing inside `HTTPServer::Server`.
        - 1.2: Establish persistent socket connections attached to active worker threads or a lightweight event poll loop.
        - 1.3: Implement binary and text frame masking, payload fragmentation handling, and graceful connection closure.
    - **Phase 2: Client Runtime & Server-Push Integration**
        - 2.1: Upgrade the embedded JavaScript runtime in `buildHtmlShell()` to connect via WebSocket with fallback to HTTP POST.
        - 2.2: Route client events over the persistent WebSocket connection, streaming re-rendered VNode JSON patches back instantly.
        - 2.3: Expose a server-push API (`Session::pushUpdate()`) allowing C++ background worker threads to trigger UI patches proactively.

---

## Feature 5: Static Asset Serving & Middleware Pipeline
- **Description:** Adds production-ready infrastructure to the core HTTP engine, including zero-copy static asset file serving (`/public/*`) with MIME detection and caching headers, along with an extensible HTTP middleware pipeline to support global authentication guards, logging, and request filtering.
- **Phases:**
    - **Phase 1: HTTP Middleware Chain**
        - 1.1: Define composable middleware signatures (`std::function<bool(ServerRequest&, ServerResponse&)>`).
        - 1.2: Implement sequential pipeline execution in `Server::_dispatch()` prior to route matching.
        - 1.3: Provide built-in middlewares for request logging, CORS configuration, and security headers.
    - **Phase 2: Static File Server**
        - 2.1: Implement static directory routing for requests matching `/public/*` or a user-defined assets folder.
        - 2.2: Add automatic MIME-type resolution based on file extensions (e.g., `.png`, `.svg`, `.js`, `.woff2`).
        - 2.3: Add support for HTTP caching headers (`ETag`, `Cache-Control`, `If-None-Match`) to minimize bandwidth usage.

---

## Feature 6: Multi-Component Composition & `.keet` Imports
- **Description:** Enables modular, reusable UI hierarchies by allowing `.keet` files to import and nest other `.keet` Single-File Components directly inside `<template>` markup (e.g., `<Navbar title="Dashboard" />`, `<Card item={user} />`). Extends `keety` to parse component import directives, distinguish between native HTML elements and custom component tags via PascalCase naming conventions, map dynamic properties and expressions to component props, and automate multi-file dependency resolution in CMake.
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

## Feature 7: Embedded SQLite Database Engine (True SQL in `<server>`)
- **Description:** Replaces/augments the file-backed JSON document database with SQLite bundled directly as a single `sqlite3.c` amalgamation with zero external dependencies. Empowers developers to declare production-grade relational schemas directly inside `<server>` with ACID guarantees, multi-column indexes, foreign keys, and perform raw SQL queries (`db.query(...)`, `db.execute(...)`) alongside fluent query builder helpers.
- **Phases:**
    - **Phase 1: SQLite Amalgamation & Native C++ Wrapper**
        - 1.1: Integrate the SQLite 3 amalgamation (`sqlite3.c` / `sqlite3.h`) into `src/` and `include/keet/` with zero third-party dependencies.
        - 1.2: Implement `SqliteDatabase` wrapper with RAII connection management, prepared statement caching, and parameterized value binding.
        - 1.3: Expose high-performance query execution APIs returning `Json::Array` result sets and maintain fluent `TableProxy` compatibility.
    - **Phase 2: KeetScript `<server>` SQL Grammar & Schema Codegen**
        - 2.1: Extend `<server>` table grammar to support standard SQL column types (`text`, `integer`, `real`, `blob`) and constraints (`not_null`, `unique`, `foreign_key`, `check`).
        - 2.2: Add raw SQL query execution syntax in backend route handlers (`db.query("SELECT * FROM tasks WHERE completed = false")`).
        - 2.3: Generate automated DDL migration statements (`CREATE TABLE IF NOT EXISTS`, index setup) during startup in `Codegen::_generateAppMain()`.

---

## Feature 8: Live Reload / Hot Module Reloading (HMR) for `.keet` Files
- **Description:** Delivers a Vite/Next.js-style instant development feedback loop for full-stack C++ applications. `keety` watches `.keet` files in development mode, transpiles modified templates, and pushes surgical Virtual DOM patches over WebSocket/SSE directly into active browser sessions without dropping user session state, clearing inputs, or requiring browser page reloads.
- **Phases:**
    - **Phase 1: File Watcher & Dev Server Mode**
        - 1.1: Implement a cross-platform file watcher in `keety` (`--watch` / `keet dev`) detecting `.keet` file modifications in real time.
        - 1.2: Establish a lightweight local reload channel between `keety` and the running frontend server.
        - 1.3: Display in-browser compile diagnostics and syntax error toast overlays when `.keet` parsing fails.
    - **Phase 2: Dynamic Template Reloading & State-Preserving VDOM Patching**
        - 2.1: Implement dynamic component loading or template recompilation to update component rendering trees on the fly.
        - 2.2: Broadcast re-render notifications across active browser sessions and stream surgical VNode patches over the live client transport.
        - 2.3: Preserve component state values (`useState`) and active session scopes across hot reloads.

---

## Feature 9: Server-Driven Slots & Content Projection (`<slot />`)
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


