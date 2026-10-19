# Keet Development Roadmap 🦜

---

## Feature 1: Scoped Component Styles & Automatic Style Harvesting
- **Description:** Currently, each component's `styles()` method must be manually merged into the parent component's stylesheet (e.g., `s.merge(Child{}.styles())` in `App::styles()`), and all CSS selectors are emitted into a single global `/styles.css` file. This feature introduces automated traversal to harvest styles from rendered component trees during render passes, as well as automatic CSS class scoping (e.g., using unique component attributes or generated hashes) to prevent style leakage and naming collisions across components.
- **Phases:**
    - **Phase 1: Automatic Tree-Wide Style Harvesting**
        - 1.1: Implement recursive VNode tree traversal in `UIEngine` to collect declared `StyleSheet` objects from all active component instances during a render pass.
        - 1.2: Cache and merge collected rules dynamically into the frontend server's `/styles.css` endpoint response without requiring manual parent `merge()` calls.
        - 1.3: Deprecate manual style merging in component parent nodes while maintaining backward compatibility for explicit overrides.
    - **Phase 2: CSS Scoping & Isolation Engine**
        - 2.1: Implement automatic scope attribute tagging (e.g., `data-keet-scope="ComponentName#ID"`) on component root VNodes.
        - 2.2: Rewrite and namespace selector rules during CSS serialization so declarations apply strictly within the matching component scope.
        - 2.3: Add support for conditional class directives (e.g., `class:active="isActive"`) directly inside VNode attribute maps.

---

## Feature 2: Built-in Internationalization & Localization (i18n)
- **Description:** Provides an out-of-the-box localization subsystem to translate UI text dynamically across multiple languages. The module manages thread-safe JSON dictionary storage, parameter interpolation, pluralization rules, and automatic client locale negotiation via `Accept-Language` headers and cookie overrides without requiring full-page reloads.
- **Phases:**
    - **Phase 1: Core Localization Engine (`keet/Localization.h`)**
        - 1.1: Create the `Localization` singleton to parse and store JSON-based translation catalogs (e.g., `locales/en.json`, `locales/pl.json`).
        - 1.2: Implement translation lookup with fallback language resolution and parameter replacement (e.g., `__("welcome", {{"name", "User"}})`).
        - 1.3: Expose ergonomic global translation helpers (`__("key")`) accessible directly within component `render()` methods.
    - **Phase 2: Session Integration & Dynamic Language Switching**
        - 2.1: Add locale detection based on the HTTP `Accept-Language` header and session cookie (`lorikeet_locale`).
        - 2.2: Store the active locale within the user's `Session` instance, guarded by `session->mutex`.
        - 2.3: Provide an API endpoint or event trigger to switch the active session locale and immediately re-render the VNode tree with translated strings.

---

## Feature 3: Dedicated KeetScript DSL & `keety` AOT Compiler 🐱
- **Description:** Introduces **KeetScript** (`.keet`), a high-level, human-friendly Single-File Component (SFC) syntax combining JavaScript/Python ergonomics with declarative layout and scoped CSS. The companion compiler, **`keety`** (*"little cat"* 🐱), transpiles `.keet` files ahead-of-time (AOT) into standard, optimized C++20 code (`.h` / `.cpp`) during the CMake build process, delivering developer-friendly syntax with zero runtime performance penalty.
- **Phases:**
    - **Phase 1: KeetScript Grammar & AST Definition**
        - 1.1: Specify the `.keet` file format containing dedicated sections for state/logic (`let`, `state`, Pythonic control flow), markup layout, and scoped `<style>`.
        - 1.2: Design an intuitive event expression model for client-side state mutations (`onClick=() => count += 1`, `bind:value=text`).
        - 1.3: Define the external C++ bridge syntax (`import cpp from "Header.h" as Module`) for delegating complex tasks to native C++ functions.
    - **Phase 2: `keety` Compiler CLI Implementation**
        - 2.1: Build the lexer, parser, and code generator for the standalone `keety` executable.
        - 2.2: Implement C++20 code generation emitting `UIEngine::Component` subclasses, static string interning, and pre-allocated child vectors.
        - 2.3: Integrate `keety` into CMake via `add_custom_command()` to automate `.keet` $\rightarrow$ `.cpp` transpilation on file changes.
    - **Phase 3: IDE Tooling & Language Support**
        - 3.1: Create a TextMate grammar bundle (`keet.tmLanguage.json`) for syntax highlighting in CLion and VS Code.
        - 3.2: Develop a dedicated CLion/IntelliJ Platform plugin with auto-completion and error diagnostics.

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
