//
// Created by PAWEL on 19/10/2026.
//

#ifndef KEETY_AST_H
#define KEETY_AST_H

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Token.h"

namespace Keety {

    class ASTVisitor; // Forward declaration for AST visitor interface

    struct ASTNode { // Abstract base class for all Abstract Syntax Tree nodes in KeetScript
        SourceLocation loc; // Source coordinate where this syntax construct begins

        explicit ASTNode(SourceLocation l = {}) : loc(std::move(l)) {}
        virtual ~ASTNode() = default;

        virtual void accept(ASTVisitor& visitor) const = 0; // Accept visitor for AST traversal, code generation, and inspection
    };

    // Forward declarations of specific AST nodes
    struct SourceFile;
    struct ServerBlock;
    struct DatabaseDecl;
    struct TableDecl;
    struct TableColumnDecl;
    struct BackendServerDecl;
    struct RouteDecl;
    struct FrontendServerDecl;
    struct ScriptBlock;
    struct ImportCppDecl;
    struct StateDecl;
    struct LetDecl;
    struct FunctionDecl;
    struct TemplateBlock;
    struct TemplateNode;
    struct ElementNode;
    struct TextNode;
    struct InterpolationNode;
    struct IfBlockNode;
    struct ForBlockNode;
    struct StyleBlock;
    struct StyleRule;
    struct ClassFieldDecl;
    struct ConstructorDecl;
    struct ClassMethodDecl;
    struct ClassDecl;
    struct StructDecl;
    struct ConstDecl;
    struct ErrtypeDecl;
    struct StandaloneFunctionDecl;
    struct PureModule;

    class ASTVisitor { // Visitor interface enabling clean traversal of the KeetScript AST hierarchy
        public:
            virtual ~ASTVisitor() = default;

            virtual void visit(const SourceFile& node) = 0;
            virtual void visit(const ServerBlock& node) = 0;
            virtual void visit(const DatabaseDecl& node) = 0;
            virtual void visit(const TableDecl& node) = 0;
            virtual void visit(const TableColumnDecl& node) = 0;
            virtual void visit(const BackendServerDecl& node) = 0;
            virtual void visit(const RouteDecl& node) = 0;
            virtual void visit(const FrontendServerDecl& node) = 0;
            virtual void visit(const ScriptBlock& node) = 0;
            virtual void visit(const ImportCppDecl& node) = 0;
            virtual void visit(const StateDecl& node) = 0;
            virtual void visit(const LetDecl& node) = 0;
            virtual void visit(const FunctionDecl& node) = 0;
            virtual void visit(const TemplateBlock& node) = 0;
            virtual void visit(const ElementNode& node) = 0;
            virtual void visit(const TextNode& node) = 0;
            virtual void visit(const InterpolationNode& node) = 0;
            virtual void visit(const IfBlockNode& node) = 0;
            virtual void visit(const ForBlockNode& node) = 0;
            virtual void visit(const StyleBlock& node) = 0;
            virtual void visit(const StyleRule& node) = 0;
            virtual void visit(const ClassFieldDecl& node) = 0;
            virtual void visit(const ConstructorDecl& node) = 0;
            virtual void visit(const ClassMethodDecl& node) = 0;
            virtual void visit(const ClassDecl& node) = 0;
            virtual void visit(const StructDecl& node) = 0;
            virtual void visit(const ConstDecl& node) = 0;
            virtual void visit(const ErrtypeDecl& node) = 0;
            virtual void visit(const StandaloneFunctionDecl& node) = 0;
            virtual void visit(const PureModule& node) = 0;
    };

    // =========================================================================
    // Full-Stack Server Block AST Definitions (<server>)
    // =========================================================================

    struct TableColumnDecl : public ASTNode { // Column schema declaration inside table <name> { ... }
        std::string name; // Column name (e.g. "id", "title")
        std::string typeName = "string"; // Data type ("int", "string", "bool", "float", "array", "object", "blob")
        bool isPrimaryKey = false; // primary_key
        bool isAutoIncrement = false; // auto_increment
        bool isUnique = false; // unique
        bool isNotNull = false; // not_null / required
        std::string defaultValueExpr; // Optional default value expression

        explicit TableColumnDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct TableDecl : public ASTNode { // Table schema declaration inside database <name> { ... }
        std::string name; // Table name
        std::vector<std::shared_ptr<TableColumnDecl>> columns; // Columns in this table

        explicit TableDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct DatabaseDecl : public ASTNode { // Database declaration (database db("app.db") { ... })
        std::string name = "db"; // Variable name for database
        std::string filePath = "app.db"; // Database file path
        std::vector<std::shared_ptr<TableDecl>> tables; // Defined tables

        explicit DatabaseDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct RouteDecl : public ASTNode { // Backend REST endpoint route (e.g. get "/api/todos" => (req) { ... })
        std::string method = "GET"; // "GET", "POST", "PUT", "DELETE"
        std::string path = "/"; // Route path (e.g. "/api/todos")
        std::string paramName = "req"; // Name of request param
        std::string body; // Handler C++ statement block

        explicit RouteDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct BackendServerDecl : public ASTNode { // Backend server declaration (backend api(port: 8000, threads: 4) { ... })
        std::string name = "api"; // Variable name for backend server
        int port = 8000; // Listening port
        int threads = 4; // Worker thread pool size
        std::vector<std::shared_ptr<RouteDecl>> routes; // Registered endpoints

        explicit BackendServerDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct FrontendServerDecl : public ASTNode { // Frontend server declaration (frontend web(port: 8080, api: "...", title: "..."))
        std::string name = "web"; // Variable name for frontend server
        int port = 8080; // Listening port
        int threads = 4; // Worker thread pool size
        std::string apiBaseUrl = "http://localhost:8000"; // Configured backend base URL
        std::string pageTitle = "Keet App"; // HTML shell document title

        explicit FrontendServerDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ServerBlock : public ASTNode { // Container node aggregating all server definitions inside <server>
        std::shared_ptr<DatabaseDecl> database; // Optional database declaration
        std::shared_ptr<BackendServerDecl> backend; // Optional backend server declaration
        std::shared_ptr<FrontendServerDecl> frontend; // Optional frontend server declaration

        explicit ServerBlock(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    // =========================================================================
    // Phase 1.3: External C++ Bridge AST Definition
    // =========================================================================

    struct ImportCppDecl : public ASTNode { // Represents an external native C++ bridge import directive
        enum class ImportKind {
            ModuleAlias, // import cpp from "Header.h" as ModuleName;
            SelectiveSymbols // import cpp { funcA, ClassB } from "Header.h";
        };

        ImportKind kind = ImportKind::ModuleAlias; // Mode of C++ header inclusion
        std::string headerPath; // Relative or absolute file path to the C++ header
        std::string asAlias; // Qualified module namespace alias (e.g., "Analytics")
        std::vector<std::string> symbols; // Explicit symbol names when using selective import syntax

        explicit ImportCppDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    // =========================================================================
    // Phase 1.1: Script State & Logic AST Definitions
    // =========================================================================

    struct StateDecl : public ASTNode { // Represents a reactive component state variable declaration (state count: int = 0;)
        std::string name; // Name of the state variable
        std::string typeName = "int"; // Declared type (e.g., "int", "string", "bool", "double")
        std::string initialValueExpr = "{}"; // Expression representing initial value

        explicit StateDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct LetDecl : public ASTNode { // Represents an immutable local or property declaration (let title: string = "Dashboard";)
        std::string name; // Property or variable identifier
        std::string typeName = "string"; // Data type
        std::string defaultValueExpr; // Optional fallback default expression

        explicit LetDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ConstDecl : public ASTNode { // Represents a constant declaration (const PI: float = 3.14159;)
        std::string name; // Constant identifier
        std::string typeName = "auto"; // Data type
        std::string valueExpr; // Initial value expression
        bool isExported = false; // export const ...

        explicit ConstDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    enum class ReferenceKind {
        Value,          // T (owned value)
        ImmutableRef,   // &T (immutable borrow)
        MutableRef      // &mut T (mutable borrow)
    };

    struct TypeAnnotation {
        std::string baseType;
        ReferenceKind refKind = ReferenceKind::Value;
        std::string lifetime;
        bool isMove = false;

        [[nodiscard]] bool isReference() const { return refKind != ReferenceKind::Value; }
        [[nodiscard]] bool isMutableRef() const { return refKind == ReferenceKind::MutableRef; }
        [[nodiscard]] bool isImmutableRef() const { return refKind == ReferenceKind::ImmutableRef; }

        [[nodiscard]] std::string toString() const {
            std::string s;
            if (refKind == ReferenceKind::ImmutableRef) {
                s += "&";
                if (!lifetime.empty()) s += lifetime + " ";
            } else if (refKind == ReferenceKind::MutableRef) {
                s += "&mut ";
                if (!lifetime.empty()) s += lifetime + " ";
            }
            s += baseType;
            return s;
        }

        static TypeAnnotation parse(std::string_view raw) {
            TypeAnnotation t;
            std::string s(raw);
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();

            if (s.rfind("&", 0) == 0) {
                t.refKind = ReferenceKind::ImmutableRef;
                s = s.substr(1);
                while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
                if (!s.empty() && s.front() == '\'') {
                    size_t sp = s.find(' ');
                    if (sp != std::string::npos) {
                        t.lifetime = s.substr(0, sp);
                        s = s.substr(sp + 1);
                    } else {
                        t.lifetime = s;
                        s.clear();
                    }
                }
                while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
                if (s.rfind("mut", 0) == 0 && (s.size() == 3 || std::isspace(static_cast<unsigned char>(s[3])))) {
                    t.refKind = ReferenceKind::MutableRef;
                    s = s.substr(3);
                }
            }
            while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
            t.baseType = s;
            return t;
        }
    };

    struct ParameterDecl { // Helper struct representing a formal parameter in a def function
        std::string name; // Parameter variable name
        std::string typeName; // Parameter type annotation
        ReferenceKind refKind = ReferenceKind::Value; // Ownership/borrow kind
        std::string lifetime; // Optional lifetime annotation
        bool isMove = false; // Accepts value via move
    };

    struct FunctionDecl : public ASTNode { // Represents a helper function or event handler method (def reset() { ... })
        std::string name; // Function name
        std::vector<ParameterDecl> parameters; // Formal parameter list
        std::string returnType = "void"; // Declared return type
        std::string body; // Raw or parsed statement body
        std::vector<Token> bodyTokens; // Tokens within function body for borrow checker & K-IR pipeline

        explicit FunctionDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ScriptBlock : public ASTNode { // Container node aggregating all declarations within <script>
        std::vector<std::shared_ptr<ImportCppDecl>> imports; // External C++ header bridge imports
        std::vector<std::shared_ptr<StateDecl>> stateDecls; // Reactive state variables
        std::vector<std::shared_ptr<LetDecl>> letDecls; // Props and local constants
        std::vector<std::shared_ptr<ConstDecl>> constants; // Immutable constants
        std::vector<std::shared_ptr<FunctionDecl>> functions; // Component methods
        std::vector<std::shared_ptr<ErrtypeDecl>> errtypes; // Error type definitions

        explicit ScriptBlock(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    // =========================================================================
    // Phase 1.2: Event Expression & Two-Way Binding AST Definitions
    // =========================================================================

    enum class AttributeKind {
        StaticLiteral, // Standard HTML string attribute (e.g., class="btn")
        DynamicExpression, // Dynamic attribute expression (e.g., title={pageTitle})
        EventCallback, // DOM event handler callback (e.g., onClick={() => count += 1})
        TwoWayBinding // Two-way state synchronizer (e.g., bind:value={searchQuery})
    };

    enum class EventParamStyle {
        ZeroArg, // () => count += 1
        EventArg, // (e) => query = e.value
        MethodReference // onClick={handleReset}
    };

    struct EventExpression { // Parsed representation of client-side state mutation event callback
        EventParamStyle paramStyle = EventParamStyle::ZeroArg; // Invocation signature
        std::string eventArgName = "e"; // Parameter name when paramStyle is EventArg
        std::string rawExpression; // Exact body expression (e.g., "count += 1" or "handleClick()")
        std::string targetState; // Identified state variable target if this is a direct assignment mutation
        std::string mutationOp; // Operator for mutations ("=", "+=", "-=")
        std::string mutationValue; // Right-hand side expression for direct state mutations
    };

    struct AttributeBinding { // Represents an attribute, directive, or event attached to an element node
        AttributeKind kind = AttributeKind::StaticLiteral; // Classification of attribute
        std::string name; // Attribute or event name (e.g., "class", "onClick", "bind:value")
        std::string rawValue; // Raw literal or expression text
        std::string bindTargetProperty; // Target HTML property name for two-way bindings (e.g., "value", "checked")
        std::string bindStateVariable; // Target State<T> identifier for two-way bindings
        EventExpression eventExpr; // Parsed event model when kind is EventCallback
        SourceLocation loc; // Source coordinate for error diagnostics
    };

    // =========================================================================
    // Phase 1.1: Template & Layout AST Definitions
    // =========================================================================

    struct TemplateNode : public ASTNode { // Base class for all nodes inside <template>
        explicit TemplateNode(SourceLocation l = {}) : ASTNode(std::move(l)) {}
    };

    struct TextNode : public TemplateNode { // Raw static text node in the Virtual DOM tree
        std::string text; // Plain text content

        TextNode(std::string t, SourceLocation l = {})
            : TemplateNode(std::move(l)), text(std::move(t)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct InterpolationNode : public TemplateNode { // Dynamic expression interpolation within markup ({count + 1})
        std::string expression; // Evaluated expression string

        InterpolationNode(std::string expr, SourceLocation l = {})
            : TemplateNode(std::move(l)), expression(std::move(expr)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ElementNode : public TemplateNode { // Represents an HTML element or custom component tag (<div ...>)
        std::string tagName; // Tag name (e.g., "div", "button", "input", "CustomCard")
        std::vector<AttributeBinding> attributes; // List of static, dynamic, event, and bind attributes
        std::vector<std::shared_ptr<TemplateNode>> children; // Child template elements
        bool isSelfClosing = false; // True if tag terminates with "/>"

        ElementNode(std::string tag, SourceLocation l = {})
            : TemplateNode(std::move(l)), tagName(std::move(tag)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }

        [[nodiscard]] const AttributeBinding* findAttribute(const std::string& attrName) const {
            for (const auto& attr : attributes) {
                if (attr.name == attrName) return &attr;
            }
            return nullptr;
        }
    };

    struct IfBranch { // Individual conditional branch inside an if-elif-else construct
        std::string conditionExpr; // Condition expression (empty string indicates an unconditional else branch)
        std::vector<std::shared_ptr<TemplateNode>> children; // Branch body nodes
        SourceLocation loc; // Branch starting location
    };

    struct IfBlockNode : public TemplateNode { // Represents Pythonic conditional control flow (if / elif / else)
        std::vector<IfBranch> branches; // Ordered branches; index 0 is 'if', followed by 'elif', optional 'else'

        explicit IfBlockNode(SourceLocation l = {}) : TemplateNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ForBlockNode : public TemplateNode { // Represents Pythonic loop iteration (for item in items:)
        std::string itemIdentifier; // Loop variable name (e.g., "item")
        std::string collectionExpr; // Target iterable collection expression (e.g., "users")
        std::vector<std::shared_ptr<TemplateNode>> children; // Loop body elements

        explicit ForBlockNode(SourceLocation l = {}) : TemplateNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct TemplateBlock : public ASTNode { // Container node holding the root children of <template>
        std::vector<std::shared_ptr<TemplateNode>> roots; // Root child elements of the component template

        explicit TemplateBlock(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    // =========================================================================
    // Phase 1.1: Scoped Style AST Definitions
    // =========================================================================

    struct StyleDeclaration { // Individual CSS property declaration (e.g., background: #1e293b;)
        std::string property; // CSS property name
        std::string value; // CSS property value
        SourceLocation loc; // Source coordinate
    };

    struct StyleRule : public ASTNode { // CSS selector rule block (.card { ... })
        std::string selector; // Target CSS selector
        std::vector<StyleDeclaration> declarations; // Key-value CSS property declarations

        explicit StyleRule(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct StyleBlock : public ASTNode { // Container node for all CSS rules inside <style>
        std::vector<std::shared_ptr<StyleRule>> rules; // Parsed CSS selector rule blocks
        std::string rawCss; // Raw CSS text content for fallback or direct streaming

        explicit StyleBlock(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    // =========================================================================
    // Standalone KeetScript Pure Module & OOP AST Definitions
    // =========================================================================

    enum class Visibility { // Access modifier for class fields and methods
        Public,
        Private,
        Protected
    };

    struct ClassFieldDecl : public ASTNode { // Field definition inside a class or struct (public let c: int;)
        std::string name; // Variable identifier
        std::string typeName = "int"; // Type name
        Visibility visibility = Visibility::Public; // Access visibility
        std::string defaultValueExpr; // Optional initial value

        explicit ClassFieldDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ConstructorDecl : public ASTNode { // Constructor declaration inside class Foo(a: int, b: int) { ... }
        std::string name; // Class constructor identifier
        std::vector<ParameterDecl> parameters; // Formal parameters
        std::string body; // Constructor implementation statements
        std::vector<Token> bodyTokens; // Tokens within constructor body

        explicit ConstructorDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ClassMethodDecl : public ASTNode { // Method declaration inside class (fn subtract(): int { ... })
        std::string name; // Method identifier
        std::string returnType = "void"; // Declared return type
        Visibility visibility = Visibility::Public; // Access visibility
        std::vector<ParameterDecl> parameters; // Formal parameters
        std::string body; // Method implementation statements
        std::vector<Token> bodyTokens; // Tokens within method body

        explicit ClassMethodDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ClassDecl : public ASTNode { // Object-oriented class declaration (export class Foo { ... })
        std::string name; // Class name identifier
        bool isExported = false; // export class ...
        std::vector<std::shared_ptr<ClassFieldDecl>> fields; // Class fields
        std::vector<std::shared_ptr<ConstructorDecl>> constructors; // Constructors
        std::vector<std::shared_ptr<ClassMethodDecl>> methods; // Class methods

        explicit ClassDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct StructDecl : public ASTNode { // Value type struct declaration (struct Point { x: int; y: int; })
        std::string name; // Struct name identifier
        bool isExported = false; // export struct ...
        std::vector<std::shared_ptr<ClassFieldDecl>> fields; // Struct fields

        explicit StructDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ErrtypeField { // Single entry in an error enum
        std::string name;
        int code = 0;
        bool hasCode = false;
    };

    struct ErrtypeDecl : public ASTNode { // Strongly-typed error enum (errtype FileError { NotFound = 404, ... })
        std::string name; // Error enum identifier
        bool isExported = false; // export errtype ...
        std::vector<ErrtypeField> fields; // Enumeration entries

        explicit ErrtypeDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct StandaloneFunctionDecl : public ASTNode { // Top-level pure function (fn add(a: int, b: int): int { ... })
        std::string name; // Function name
        std::string returnType = "void"; // Return type
        bool isExported = false; // export fn ...
        bool isMain = false; // fn main(): void entry point
        std::vector<ParameterDecl> parameters; // Formal parameters
        std::string body; // Function implementation statements
        std::vector<Token> bodyTokens; // Tokens within function body

        explicit StandaloneFunctionDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct PureModule : public ASTNode { // Container for a pure KeetScript file (.keet without XML tags)
        std::string moduleName; // Optional module namespace
        std::vector<std::shared_ptr<ImportCppDecl>> imports; // C++ header imports
        std::vector<std::shared_ptr<ClassDecl>> classes; // Class declarations
        std::vector<std::shared_ptr<StructDecl>> structs; // Struct declarations
        std::vector<std::shared_ptr<ConstDecl>> constants; // Constant declarations
        std::vector<std::shared_ptr<ErrtypeDecl>> errtypes; // Error type enum declarations
        std::vector<std::shared_ptr<StandaloneFunctionDecl>> functions; // Top-level functions

        explicit PureModule(SourceLocation l = {}) : ASTNode(std::move(l)) {}
        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    // =========================================================================
    // Root AST Node: Single-File Component or Pure Module
    // =========================================================================

    struct SourceFile : public ASTNode { // Top-level root AST node representing a complete .keet file
        std::string componentName; // Derived component or module name (e.g., "CounterCard", "MathUtils")
        bool isPureModule = false; // True if this file is a standalone pure module without XML tags
        std::unique_ptr<PureModule> moduleBlock; // Pure module AST when isPureModule == true
        std::unique_ptr<ServerBlock> server; // Optional <server> section AST (Full-Stack App mode)
        std::unique_ptr<ScriptBlock> script; // Optional <script> section AST
        std::unique_ptr<TemplateBlock> templateBlock; // Optional <template> section AST
        std::unique_ptr<StyleBlock> styleBlock; // Optional <style> section AST

        explicit SourceFile(std::string name = "Component", SourceLocation l = {})
            : ASTNode(std::move(l)), componentName(std::move(name)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

} // namespace Keety

#endif // KEETY_AST_H
