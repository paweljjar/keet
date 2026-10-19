# Keet 🦜

A lightweight, high-performance, full-stack C++ micro-framework designed for building modern web applications with zero external dependencies.

Keet provides an end-to-end web framework entirely written in modern C++ (C++20). It pairs a concurrent multi-threaded HTTP server and a file-backed JSON document database with a React-inspired server-driven Virtual DOM engine.

---

## Features

- **Component-Driven C++ UI Engine (`UIEngine`):**
    - Declarative Virtual DOM nodes (`VNode`) and tag helpers (`div`, `button`, `form`, `input`, `dialog`, etc.).
    - Scoped persistent state via React-style `useState<T>` hooks and operator streaming.
    - Automatic component style harvesting and CSS class scoping (`data-keet-scope`).
    - Component lifecycle hooks (`onMount`, `onUnmount`) with automatic scope tracking and cleanup.
    - Scoped CSS-in-C++ stylesheet aggregation (`StyleSheet`) served automatically at `/styles.css`.
    - Client-side routing (`Router`, `Link`) with pushState browser history.
    - Isolated multi-session state management (`SessionManager`) with configurable idle TTL pruning.

- **Built-in Internationalization & Localization (`Localization`):**
    - JSON-backed translation catalogs with fallback language negotiation.
    - Contextual parameter interpolation (e.g., `__("welcome", {{"name", "User"}})`).
    - Dynamic plural rule evaluation and automatic locale detection via `Accept-Language` headers and session cookies.

- **KeetScript Single-File Components & Full-Stack Apps (`.keet`):**
    - Declarative SFC format synthesizing reactive UI, backend microservices, and database schemas in a single file.
    - **`<server>`:** Declarative embedded SQLite database tables (`primary_key`, `auto_increment`), backend HTTP REST routes (`get`, `post`, `put`, `delete`), and frontend server mounts.
    - **`<script>`:** Reactive state declarations (`state`), local constants (`let`), helper methods (`def`), and native C++ header bridges (`import cpp`).
    - **`<template>`:** Declarative HTML markup, reactive interpolations (`{expr}`), two-way data bindings (`bind:value`, `bind:checked`), inline event lambdas (`onClick`), and Pythonic control flow (`if:`, `elif:`, `else:`, `for item in items:`).
    - **`<style>`:** Scoped CSS automatically namespaced to component instances.

- **`keety` Ahead-of-Time (AOT) C++20 Compiler:**
    - Standalone transpiler (`tools/keety`) converting `.keet` files into optimized C++20 classes derived from `UIEngine::Component`.
    - **Application Mode (`--app`):** Emits a self-contained executable entry point (`main.cpp`) with database auto-migrations, multithreaded backend REST services, and frontend web server worker pools.
    - Seamless CMake integration via `keet_target_sources()` and `keet_add_application()`.

- **First-Class IDE Tooling:**
    - **Visual Studio Code Extension (`tooling/vscode-plugin`):** Full syntax highlighting, snippet library (`keetapp`, `server`, `database`, `backend`, `bind:*`), live error linting via `keety --lint`, and one-click compilation.
    - **TextMate Grammar (`tooling/textmate`):** Comprehensive syntax definitions (`source.keet`) for VS Code and TextMate-compatible editors.

- **Concurrent Multi-Threaded HTTP Server (`HTTPServer`):**
    - Worker thread pool architecture with a synchronized task queue and condition variable signaling.
    - Zero external dependencies; built using standard POSIX sockets and native C++ threads.
    - RESTful routing (`GET`, `POST`, `PUT`, `DELETE`), custom method dispatchers, CORS preflight handling (`OPTIONS`), and SPA fallback GET handling.
    - Configurable request body/header limits and connection timeouts.

- **Lightweight JSON & Database Engine (`Database`, `Json`):**
    - Type-safe DOM-style JSON parser and serializer backed by `std::variant`.
    - File-backed JSON database supporting SQL-like table schemas, column types (`integer`, `string`, `boolean`, `float`, `array`, `object`), auto-increment primary keys, and uniqueness constraints.
    - Fluent `TableProxy` accessor (`db.table("items").find()`, `.insert()`, `.update()`, `.remove()`) with filter dictionaries and lambda predicates.

- **Resilient Thin-Client Runtime:**
    - Microscopic inline JavaScript runtime powering DOM diffing and keyed reconciliation (`reconcileChildren`).
    - Built-in loading states (`data-loading`, `.keet-pending`, `.keet-submitting`) and toast notification system.
    - Component error boundaries (`createErrorBoundaryNode`) to isolate rendering faults gracefully.

---

## Project Structure

```text
keet/
├── CMakeLists.txt
├── cmake/
│   └── KeetCompiler.cmake      # Ahead-of-Time build macros (keet_add_application)
├── include/
│   └── keet/
│       ├── Database.h          # File-backed JSON database & TableProxy
│       ├── Json.h              # Lightweight JSON parser/serializer
│       ├── Localization.h      # i18n translation catalogs & plural rules
│       ├── Server.h            # Multi-threaded HTTP server & routing
│       └── UIEngine.h          # Virtual DOM, State, Styles & Component lifecycle
├── spec/
│   └── KeetScript.md           # Language specification & EBNF grammar
├── src/
│   ├── Database.cpp
│   ├── Json.cpp
│   ├── Localization.cpp
│   ├── Server.cpp
│   └── UIEngine.cpp
├── tooling/
│   ├── textmate/               # TextMate syntax grammar & language config
│   └── vscode-plugin/          # VS Code extension (highlighting, snippets, linting)
└── tools/
    └── keety/                  # Standalone KeetScript AOT C++20 compiler
        ├── AST.h / ASTPrinter.h
        ├── Lexer.h / Lexer.cpp
        ├── Parser.h / Parser.cpp
        ├── Codegen.h / Codegen.cpp
        └── main.cpp
```

---

## Getting Started

### 1. Requirements
- Modern C++ compiler supporting **C++20** (GCC 11+, Clang 13+, or Apple Clang 13+)
- CMake 3.20+

### 2. Integration via CMake

#### As a Submodule or Subdirectory
Add Keet to your project under `external/keet` and link against it in your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/keet)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE keet)
```

---

## Quick Example

Here is a minimal full-stack counter application built with Keet:

```cpp
#include <keet/Server.h>
#include <keet/UIEngine.h>

class CounterApp : public UIEngine::Component {
public:
    using Component::Component;

    [[nodiscard]] std::string getComponentName() const override { return "CounterApp"; }

    [[nodiscard]] UIEngine::VNode render() const override {
        UIEngine::State<int> count = useState<int>("count", 0);

        return UIEngine::Tags::main({{"class", "container"}}, {
            UIEngine::Tags::h1({}, "Keet Counter"),
            UIEngine::Tags::p({}, "Count: " + std::to_string(count.get())),
            UIEngine::Tags::div({{"class", "actions"}}, {
                UIEngine::Tags::button({
                    {"class", "btn"},
                    {"onClick", [count] { count.set(count.get() - 1); }}
                }, "-1"),
                UIEngine::Tags::button({
                    {"class", "btn"},
                    {"onClick", [count] { count.set(count.get() + 1); }}
                }, "+1")
            })
        });
    }

    [[nodiscard]] UIEngine::StyleSheet styles() const override {
        UIEngine::StyleSheet s;
        s.rule(".container", {
            {"font-family", "system-ui, -apple-system, sans-serif"},
            {"max-width", "400px"},
            {"margin", "50px auto"},
            {"text-align", "center"}
        })
        .rule(".actions", {
            {"display", "flex"},
            {"justify-content", "center"},
            {"gap", "10px"}
        })
        .rule(".btn", {
            {"padding", "8px 16px"},
            {"font-size", "16px"},
            {"border-radius", "6px"},
            {"cursor", "pointer"}
        });
        return s;
    }
};

int main() {
    HTTPServer::Server frontend(8080, "Frontend");

    UIEngine::serveApp(frontend, CounterApp{}, {}, "http://localhost:8000", "Keet App");

    frontend.start();
    return 0;
}
```

### Full-Stack Single-File KeetScript (`App.keet`)

Instead of writing manual C++ orchestration boilerplate, you can write everything directly in KeetScript:

```keet
<server>
  database db("todos.db") {
    table todos {
      id: int primary_key auto_increment,
      title: string,
      completed: bool default false
    };
  };

  backend api(port: 8081, threads: 4) {
    get "/api/todos" => (req) {
      return db.table("todos").find();
    };
    post "/api/todos" => (req) {
      return db.table("todos").insert(req.json().asObject());
    };
  };

  frontend web(port: 8080, api: "http://localhost:8081", title: "Keet Full-Stack App");
</server>

<script>
  state count: int = 0;
</script>

<template>
  <div class="card">
    <h1>Full-Stack Keet App</h1>
    <p>Clicks: {count}</p>
    <button onClick={() => count += 1}>Increment (+1)</button>
    if count > 5:
      <p class="badge">High Activity!</p>
  </div>
</template>

<style>
  .card { padding: 2rem; max-width: 480px; margin: 40px auto; border-radius: 12px; }
  .badge { background: #dcfce7; color: #166534; padding: 4px 12px; border-radius: 9999px; }
</style>
```

#### Build with CMake:
```cmake
include(cmake/KeetCompiler.cmake)
keet_add_application(my_app src/App.keet)
```
Or directly via the `keety` compiler:
```bash
keety --app -i App.keet -o App.cpp -h App.h -c App
clang++ -std=c++20 App.cpp -lkeet -o my_app
```

### Visual Studio Code Extension

Keet provides an official extension for Visual Studio Code located under `tooling/vscode-plugin/`:
- **Full-Stack Syntax Highlighting:** Context-aware styling for `<server>`, `<script>`, `<template>`, and `<style>` blocks.
- **Rich Snippet Library:** Rapidly scaffold components (`keetapp`, `component`), servers (`server`, `database`, `table`, `backend`, `frontend`, `get`, `post`), and directives (`bind:*`, `onClick`).
- **Live Diagnostics:** Instant syntax and semantic feedback directly in your editor via `keety --lint`.
- **One-Click Compilation:** Compile KeetScript components to C++ header/source pairs or complete apps with the VS Code command palette (`KeetScript: Compile File to C++`).

Install the pre-built extension package:
```bash
code --install-extension tooling/vscode-plugin/vscode-keetscript-1.0.0.vsix
```

---

## Architecture Overview

1. **Client Request:** The browser requests `/` and receives a minimal HTML shell with pre-compiled CSS and a lightweight Virtual DOM mounting runtime.
2. **Initial Tree Fetch:** The client requests `/vdom`. The server evaluates the root component's `render()` tree, registers event callbacks to unique handler IDs, serializes the tree to JSON, and returns it.
3. **Event Dispatch:** User interactions (`click`, `submit`, `input`, `keydown`) trigger asynchronous POST requests to `/event` with target handler IDs and payloads.
4. **Server Re-Render & Patch:** Keet executes the associated C++ callback, modifies the persistent session state, re-renders the component tree, and returns the updated VNode JSON.
5. **DOM Diffing:** The client's reconciliation algorithm (`patchDom`) performs minimal in-place updates on the browser DOM without full-page reloads.

---

## License

MIT License.