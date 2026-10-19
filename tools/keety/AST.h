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

    class ASTVisitor { // Visitor interface enabling clean traversal of the KeetScript AST hierarchy
        public:
            virtual ~ASTVisitor() = default;

            virtual void visit(const SourceFile& node) = 0;
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

    struct ParameterDecl { // Helper struct representing a formal parameter in a def function
        std::string name; // Parameter variable name
        std::string typeName; // Parameter type annotation
    };

    struct FunctionDecl : public ASTNode { // Represents a helper function or event handler method (def reset() { ... })
        std::string name; // Function name
        std::vector<ParameterDecl> parameters; // Formal parameter list
        std::string returnType = "void"; // Declared return type
        std::string body; // Raw or parsed statement body

        explicit FunctionDecl(SourceLocation l = {}) : ASTNode(std::move(l)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

    struct ScriptBlock : public ASTNode { // Container node aggregating all declarations within <script>
        std::vector<std::shared_ptr<ImportCppDecl>> imports; // External C++ header bridge imports
        std::vector<std::shared_ptr<StateDecl>> stateDecls; // Reactive state variables
        std::vector<std::shared_ptr<LetDecl>> letDecls; // Props and local constants
        std::vector<std::shared_ptr<FunctionDecl>> functions; // Component methods

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
    // Root AST Node: Single-File Component
    // =========================================================================

    struct SourceFile : public ASTNode { // Top-level root AST node representing a complete .keet Single-File Component
        std::string componentName; // Derived component class name (e.g., "CounterCard")
        std::unique_ptr<ScriptBlock> script; // Optional <script> section AST
        std::unique_ptr<TemplateBlock> templateBlock; // Optional <template> section AST
        std::unique_ptr<StyleBlock> styleBlock; // Optional <style> section AST

        explicit SourceFile(std::string name = "Component", SourceLocation l = {})
            : ASTNode(std::move(l)), componentName(std::move(name)) {}

        void accept(ASTVisitor& visitor) const override { visitor.visit(*this); }
    };

} // namespace Keety

#endif // KEETY_AST_H
