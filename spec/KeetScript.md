# KeetScript (.keet) Language & Grammar Specification 🐱

**Version:** 1.0.0  
**Status:** Phase 1 Design Specification  
**Compiler:** `keety` Ahead-of-Time (AOT) C++20 Transpiler  

---

## 1. Overview & Architecture

**KeetScript** (`.keet`) is a declarative Single-File Component (SFC) format designed for Keet. It synthesizes the clean ergonomics of modern UI component systems (such as Svelte and Vue SFCs) with the performance and predictability of standard **C++20**.

A `.keet` file consists of three dedicated, optional top-level sections:
1. `<script>`: Component state declarations (`state`), local constants (`let`), functions (`def`), and native C++ header imports (`import cpp`).
2. `<template>`: Declarative HTML markup, reactive expressions (`{expr}`), event handlers (`onClick`), two-way bindings (`bind:*`), and Pythonic control flow (`if:`, `elif:`, `else:`, `for item in items:`).
3. `<style>`: CSS rules that are automatically scoped to the component instance via Keet's `UIEngine::StyleSheet` and `data-keet-scope` attribute matching.

During the CMake build process, `keety` transpiles each `.keet` file into a standard C++20 class derived from `UIEngine::Component`, emitting an optimized `.h` and `.cpp` file with zero runtime overhead.

```text
+-------------------------------------------------------------+
|                        Component.keet                       |
|  +-------------------------------------------------------+  |
|  | <script>      State, Let, Native C++ Imports, defs    |  |
|  +-------------------------------------------------------+  |
|  | <template>    HTML layout, Control Flow, Bindings     |  |
|  +-------------------------------------------------------+  |
|  | <style>       Scoped CSS-in-Component rules          |  |
|  +-------------------------------------------------------+  |
+-------------------------------------------------------------+
                               |
                               | (AOT Transpilation by keety)
                               v
               Component.h & Component.cpp
           (Standard C++20 UIEngine::Component)
```

---

## 2. File Structure & Section Grammar

A `.keet` file may contain at most one of each of the following root sections in any order, though the standard convention is `<script>` $\rightarrow$ `<template>` $\rightarrow$ `<style>`:

```keet
<script>
  // State, variables, logic, and native C++ imports
</script>

<template>
  <!-- Declarative HTML layout and control flow -->
</template>

<style>
  /* Scoped CSS rules */
</style>
```

### 2.1 Lexical Conventions

- **Encoding:** UTF-8.
- **Whitespace:** Insignificant outside of Pythonic template control flow indentation and string literals.
- **Comments:**
  - Script section: C++ style single-line (`// ...`) and block comments (`/* ... */`).
  - Template section: HTML comments (`<!-- ... -->`).
  - Style section: CSS comments (`/* ... */`).
- **Identifiers:** `[a-zA-Z_][a-zA-Z0-9_]*`.

---

## 3. Section 1: `<script>` Grammar & Semantics

The `<script>` block declares the reactive state, props, local constants, helper functions, and native C++ bridges of the component.

### 3.1 State Declarations (`state`)

Reactive state variables are declared using the `state` keyword:
```keet
state <identifier>: <type> = <initial_value>;
```

- **Supported Types:** `int`, `int64`, `double`, `float`, `bool`, `string` (maps to `std::string`), or any valid C++ type name.
- **Semantics:** Transpiles to a member `UIEngine::State<T>` initialized with the resolved component scope key.
- **Example:**
  ```keet
  state count: int = 0;
  state query: string = "";
  state isVisible: bool = true;
  ```

### 3.2 Constants and Properties (`let`)

Immutable or local values and properties passed from parent components are declared using `let`:
```keet
let <identifier>: <type> = <default_value>;
```

- **Semantics:**
  - If a property with matching name is provided in `_props`, it is extracted via `getProp()`, `getBoolProp()`, etc.
  - If omitted, the default value is used.
- **Example:**
  ```keet
  let title: string = "Dashboard";
  let maxItems: int = 10;
  ```

### 3.3 Helper Functions (`def`)

Component methods and event handlers can be defined using Pythonic function syntax:
```keet
def <function_name>(<param1>: <type1>, ...) -> <return_type> {
  // Statements
}
```
- The return type arrow (`-> <type>`) is optional and defaults to `void`.
- State variables referenced within functions automatically resolve their `.get()` and `.set()` methods in C++.
- **Example:**
  ```keet
  def resetCount() {
    count = 0;
  }

  def calculateTotal(price: double, qty: int) -> double {
    return price * qty;
  }
  ```

### 3.4 Native C++ Bridge Syntax (`import cpp`)

To integrate high-performance native C++ algorithms, database access, or external libraries without rewriting them in KeetScript, `<script>` supports native bridge imports:

#### Namespace/Module Import:
```keet
import cpp from "path/to/Header.h" as ModuleName;
```
- Emits `#include "path/to/Header.h"` into the generated C++ header/source.
- Makes `ModuleName::*` available in `<script>` and event expressions.

#### Selective Symbol Import:
```keet
import cpp { FunctionA, ClassB, CONSTANT_C } from "path/to/Header.h";
```
- Emits `#include "path/to/Header.h"` and brings the selected symbols into the component's scope.

#### Example:
```keet
<script>
  import cpp from "services/Analytics.h" as Analytics;
  import cpp { formatCurrency } from "utils/Formatter.h";

  state balance: double = 1250.50;

  def trackView() {
    Analytics::logEvent("page_view", "dashboard");
  }
</script>
```

---

## 4. Section 2: `<template>` Grammar & Semantics

The `<template>` block defines the virtual DOM hierarchy returned by the component's `render()` method.

### 4.1 Elements and Attributes

- **HTML Elements:** Standard elements (`div`, `span`, `button`, `input`, `form`, `h1`, etc.) or nested custom Keet components (`<CustomCard ... />`).
- **Static Attributes:** `class="btn primary" id="submit-btn"`.
- **Dynamic Attribute Expressions:** Enclosed in `{...}`, e.g., `title={tooltipText}`, `class={"btn " + (isActive ? "active" : "")}`.
- **Boolean Attributes:** `disabled={isLoading}`, `checked={isChecked}`.

### 4.2 Text Interpolation

Dynamic expressions embedded within element text nodes are enclosed in single curly braces `{...}`:
```keet
<h1>Welcome, {username}!</h1>
<p>Total clicks: {count + 1}</p>
```
During code generation, expressions are converted to strings via `std::to_string()` or direct string concatenation.

### 4.3 Pythonic Control Flow

KeetScript incorporates clean, Pythonic control flow directly into template markup without cumbersome wrapper tags.

#### 4.3.1 Conditional Rendering (`if / elif / else`)
```keet
if count == 0:
  <p class="empty">No items found.</p>
elif count < 10:
  <p class="few">Found a few ({count}) items.</p>
else:
  <p class="many">Found {count} items!</p>
```
- A colon (`:`) concludes the condition expression.
- Indented child elements (or balanced closing tags) form the branch body.
- Transpiles to C++ `if (...) { ... } else if (...) { ... } else { ... }` node emission.

#### 4.3.2 Loop Iteration (`for ... in ...`)
```keet
for item in items:
  <li class="item-row">{item}</li>
```
- Iterates over any C++ iterable collection (`std::vector`, `std::map`, etc.).
- Loop variable (`item`) is locally scoped within the loop body.
- Key directive support: `<li key={item.id}>{item.name}</li>` for keyed DOM reconciliation.

---

## 5. Event Expressions & Two-Way Binding Model

### 5.1 Event Handlers (`on<Event>`)

Event attributes attach client-to-server event callbacks:
- Supported events: `onClick`, `onInput`, `onChange`, `onSubmit`, `onKeyDown`, `onKeyUp`, `onFocus`, `onBlur`.

#### Inline Mutation Lambda:
```keet
<button onClick={() => count += 1}>Increment</button>
<button onClick={() => count -= 1}>Decrement</button>
```

#### Event Parameter Lambda:
```keet
<input onInput={(e) => text = e.value} />
<button onClick={(e) => handleCoordinateClick(e.clientX, e.clientY)}>Map</button>
```

#### Method Reference:
```keet
<button onClick={resetCount}>Reset</button>
```

### 5.2 Two-Way Data Binding (`bind:*`)

KeetScript provides reactive two-way synchronization between input elements and component state:

#### Text / Number Input Binding (`bind:value`):
```keet
<input type="text" bind:value={query} placeholder="Search..." />
```
**C++ Transpilation Semantics:**
- Automatically sets the element value prop: `props["value"] = query.get();`
- Automatically generates an `input` / `change` event callback:
  ```cpp
  props["input"] = UIEngine::PropValue([this](const UIEngine::Event& e) {
      query.set(e.value);
  });
  ```

#### Checkbox / Radio Binding (`bind:checked`):
```keet
<input type="checkbox" bind:checked={termsAccepted} />
```
**C++ Transpilation Semantics:**
- Automatically sets `props["checked"] = termsAccepted.get();`
- Automatically generates a `change` event callback:
  ```cpp
  props["change"] = UIEngine::PropValue([this](const UIEngine::Event& e) {
      termsAccepted.set(e.checked);
  });
  ```

---

## 6. Section 3: `<style>` Grammar & Scoping

The `<style>` block specifies CSS rules associated with the component:

```keet
<style>
  .card {
    background: #ffffff;
    border-radius: 8px;
    padding: 1.5rem;
  }

  .title {
    font-size: 1.25rem;
    color: #1e293b;
  }

  button:hover {
    filter: brightness(1.1);
  }
</style>
```

### 6.1 Scoping Semantics

- All rules in `<style>` are automatically scoped to the component instance at compile time.
- The transpiler generates an override of `virtual UIEngine::StyleSheet styles() const override`.
- Keet's existing scoping engine tags the root `VNode` with `data-keet-scope="<ComponentName>#<ID>"` and rewrites selectors:
  ```css
  [data-keet-scope="Card#1"] .card { background: #ffffff; ... }
  ```
- This prevents style leaks across components while eliminating manual `StyleSheet::merge()` calls.

---

## 7. C++20 Code Generation Architecture

Given a component `Counter.keet`, `keety` emits:
1. `Counter.h`: Class declaration inheriting from `UIEngine::Component`.
2. `Counter.cpp`: Static interned strings, `render()` definition, event lambdas, and `styles()`.

### 7.1 String Interning & Vector Optimization

To guarantee zero runtime penalty:
```cpp
// Interned static string keys to avoid heap allocation per render pass
namespace {
    static const std::string s_tag_div = "div";
    static const std::string s_tag_button = "button";
    static const std::string s_prop_class = "class";
    static const std::string s_prop_value = "value";
}

UIEngine::VNode Counter::render() const {
    UIEngine::VNode root = UIEngine::VNode::el(s_tag_div, {{s_prop_class, "counter-box"}});
    root.children.reserve(4); // Pre-allocated based on static child count
    ...
    return root;
}
```

---

## 8. Summary of Grammar EBNF (Subset)

```ebnf
SFCFile         ::= ( ScriptBlock | TemplateBlock | StyleBlock )*
ScriptBlock     ::= "<script>" ScriptStatement* "</script>"
ScriptStatement ::= ImportCppDecl | StateDecl | LetDecl | FunctionDecl
ImportCppDecl   ::= "import" "cpp" ( "from" StringLiteral "as" Identifier | "{" IdentifierList "}" "from" StringLiteral ) ";"
StateDecl       ::= "state" Identifier ":" TypeName ( "=" Expression )? ";"
LetDecl         ::= "let" Identifier ( ":" TypeName )? ( "=" Expression )? ";"
FunctionDecl    ::= "def" Identifier "(" ParameterList? ")" ( "->" TypeName )? "{" Statement* "}"

TemplateBlock   ::= "<template>" TemplateElement* "</template>"
TemplateElement ::= ElementNode | TextNode | Interpolation | IfBlock | ForBlock
ElementNode     ::= "<" TagName Attribute* ( "/>" | ">" TemplateElement* "</" TagName ">" )
Attribute       ::= StaticAttr | DynamicAttr | BindAttr | EventAttr
BindAttr        ::= "bind:" ( "value" | "checked" ) "={" Identifier "}"
EventAttr       ::= "on" Identifier "={" EventExpression "}"
EventExpression ::= ArrowFunction | FunctionReference

IfBlock         ::= "if" Expression ":" TemplateElement+ ( "elif" Expression ":" TemplateElement+ )* ( "else" ":" TemplateElement+ )?
ForBlock        ::= "for" Identifier "in" Expression ":" TemplateElement+

StyleBlock      ::= "<style>" CssRule* "</style>"
```
