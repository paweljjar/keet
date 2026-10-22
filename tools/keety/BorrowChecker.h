//
// Created by PAWEL on 22/10/2026.
// BorrowChecker.h - Compile-Time Memory Safety & Lifetime Engine for KeetScript 🐱
//

#ifndef KEETY_BORROW_CHECKER_H
#define KEETY_BORROW_CHECKER_H

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "AST.h"
#include "Diagnostic.h"
#include "Token.h"

namespace Keety {

    enum class OwnershipState {
        Uninitialized,
        Owned,
        Moved,
        BorrowedImmutable,
        BorrowedMutable
    };

    struct VariableSymbol {
        std::string name;
        TypeAnnotation type;
        OwnershipState state = OwnershipState::Owned;
        int immutableBorrowCount = 0;
        bool isMutableBinding = false;
        bool isParameter = false;
        SourceLocation declaredLoc;
        SourceLocation movedLoc;
        std::vector<SourceLocation> borrowLocs;
    };

    struct ActiveBorrow {
        std::string targetVar;
        std::string borrowerVar;
        ReferenceKind kind = ReferenceKind::ImmutableRef;
        SourceLocation loc;
        size_t scopeDepth = 0;
    };

    class BorrowScope {
    public:
        size_t depth = 0;
        std::unordered_map<std::string, VariableSymbol> variables;
        std::vector<ActiveBorrow> activeBorrows;
        BorrowScope* parent = nullptr;

        explicit BorrowScope(BorrowScope* p = nullptr, size_t d = 0) : parent(p), depth(d) {}

        [[nodiscard]] VariableSymbol* lookup(const std::string& name) {
            auto it = variables.find(name);
            if (it != variables.end()) return &it->second;
            if (parent) return parent->lookup(name);
            return nullptr;
        }

        [[nodiscard]] const VariableSymbol* lookup(const std::string& name) const {
            auto it = variables.find(name);
            if (it != variables.end()) return &it->second;
            if (parent) return parent->lookup(name);
            return nullptr;
        }

        [[nodiscard]] bool hasVariable(const std::string& name) const {
            if (variables.find(name) != variables.end()) return true;
            if (parent) return parent->hasVariable(name);
            return false;
        }
    };

    class BorrowChecker {
    public:
        BorrowChecker() = default;

        bool check(const SourceFile& file, std::string_view sourceContent = "");
        void checkOrThrow(const SourceFile& file, std::string_view sourceContent = "");

        void checkFunction(const std::string& name,
                           const std::vector<ParameterDecl>& parameters,
                           const std::string& returnType,
                           const std::vector<Token>& bodyTokens,
                           const SourceLocation& loc,
                           std::string_view sourceContent = "");

        [[nodiscard]] const std::vector<Diagnostic>& getDiagnostics() const { return _diagnostics; }
        [[nodiscard]] bool hasErrors() const { return !_diagnostics.empty(); }

    private:
        void _report(Diagnostic d);
        void _reportUseOfMoved(const std::string& name, const SourceLocation& useLoc, const VariableSymbol& sym);

        std::vector<Diagnostic> _diagnostics;
    };

} // namespace Keety

#endif // KEETY_BORROW_CHECKER_H
