//
// Created by PAWEL on 22/10/2026.
// BorrowChecker.cpp - Compile-Time Memory Safety & Lifetime Engine for KeetScript 🐱
//

#include "BorrowChecker.h"

#include <algorithm>
#include <iostream>

namespace Keety {

    bool BorrowChecker::check(const SourceFile& file, std::string_view sourceContent) {
        _diagnostics.clear();

        if (file.isPureModule && file.moduleBlock) {
            for (const auto& fn : file.moduleBlock->functions) {
                checkFunction(fn->name, fn->parameters, fn->returnType, fn->bodyTokens, fn->loc, sourceContent);
            }
            for (const auto& cls : file.moduleBlock->classes) {
                for (const auto& ctor : cls->constructors) {
                    checkFunction(ctor->name, ctor->parameters, "void", ctor->bodyTokens, ctor->loc, sourceContent);
                }
                for (const auto& method : cls->methods) {
                    checkFunction(cls->name + "::" + method->name, method->parameters, method->returnType, method->bodyTokens, method->loc, sourceContent);
                }
            }
        } else if (file.script) {
            for (const auto& fn : file.script->functions) {
                checkFunction(fn->name, fn->parameters, fn->returnType, fn->bodyTokens, fn->loc, sourceContent);
            }
        }

        return _diagnostics.empty();
    }

    void BorrowChecker::checkOrThrow(const SourceFile& file, std::string_view sourceContent) {
        if (!check(file, sourceContent) && !_diagnostics.empty()) {
            throw DiagnosticException(_diagnostics.front(), sourceContent);
        }
    }

    void BorrowChecker::_report(Diagnostic d) {
        _diagnostics.push_back(std::move(d));
    }

    void BorrowChecker::_reportUseOfMoved(const std::string& name, const SourceLocation& useLoc, const VariableSymbol& sym) {
        Diagnostic d(ErrorCode::E0501_UseOfMovedValue,
                     "use of moved value '" + name + "'",
                     useLoc,
                     name.size(),
                     "value used here after move");
        if (sym.movedLoc.line > 0) {
            d.withSecondary(sym.movedLoc, name.size(), "value moved here");
        }
        d.withHelp("value was moved earlier. Consider borrowing instead: '&" + name + "'");
        _report(d);
    }

    void BorrowChecker::checkFunction(const std::string& name,
                                      const std::vector<ParameterDecl>& parameters,
                                      const std::string& returnType,
                                      const std::vector<Token>& bodyTokens,
                                      const SourceLocation& loc,
                                      std::string_view sourceContent) {
        (void)name;
        (void)sourceContent;

        BorrowScope rootScope(nullptr, 0);
        std::vector<std::unique_ptr<BorrowScope>> scopePool;
        BorrowScope* currentScope = &rootScope;

        // Populate parameters into root scope
        for (const auto& param : parameters) {
            VariableSymbol sym;
            sym.name = param.name;
            sym.type.baseType = param.typeName;
            sym.type.refKind = param.refKind;
            sym.type.lifetime = param.lifetime;
            sym.type.isMove = param.isMove;
            sym.isParameter = true;
            sym.declaredLoc = loc;

            if (param.refKind == ReferenceKind::ImmutableRef) {
                sym.state = OwnershipState::BorrowedImmutable;
                sym.immutableBorrowCount = 1;
            } else if (param.refKind == ReferenceKind::MutableRef) {
                sym.state = OwnershipState::Owned;
                sym.isMutableBinding = true;
            } else {
                sym.state = OwnershipState::Owned;
            }
            rootScope.variables[param.name] = sym;
        }

        auto pushScope = [&]() {
            auto s = std::make_unique<BorrowScope>(currentScope, currentScope->depth + 1);
            currentScope = s.get();
            scopePool.push_back(std::move(s));
        };

        auto popScope = [&]() {
            if (!currentScope) return;
            for (const auto& ab : currentScope->activeBorrows) {
                VariableSymbol* target = currentScope->lookup(ab.targetVar);
                if (target) {
                    if (!target->borrowLocs.empty()) {
                        target->borrowLocs.pop_back();
                    }
                    if (ab.kind == ReferenceKind::MutableRef) {
                        target->state = OwnershipState::Owned;
                    } else if (ab.kind == ReferenceKind::ImmutableRef) {
                        target->immutableBorrowCount = std::max(0, target->immutableBorrowCount - 1);
                        if (target->immutableBorrowCount == 0) {
                            target->state = OwnershipState::Owned;
                        }
                    }
                }
            }
            currentScope = currentScope->parent;
        };

        std::vector<ActiveBorrow> tempBorrows;
        auto clearTempBorrows = [&]() {
            for (const auto& ab : tempBorrows) {
                VariableSymbol* target = currentScope->lookup(ab.targetVar);
                if (target) {
                    if (!target->borrowLocs.empty()) {
                        target->borrowLocs.pop_back();
                    }
                    if (ab.kind == ReferenceKind::MutableRef) {
                        target->state = OwnershipState::Owned;
                    } else if (ab.kind == ReferenceKind::ImmutableRef) {
                        target->immutableBorrowCount = std::max(0, target->immutableBorrowCount - 1);
                        if (target->immutableBorrowCount == 0) {
                            target->state = OwnershipState::Owned;
                        }
                    }
                }
            }
            tempBorrows.clear();
        };

        auto checkExpression = [&](size_t& cur, const std::string& boundVar) {
            int parenDepth = 0;
            int bracketDepth = 0;
            int braceDepth = 0;

            while (cur < bodyTokens.size()) {
                if (parenDepth == 0 && bracketDepth == 0 && braceDepth == 0) {
                    if (bodyTokens[cur].is(TokenKind::Semicolon) || bodyTokens[cur].is(TokenKind::RightBrace)) {
                        break;
                    }
                }

                if (bodyTokens[cur].is(TokenKind::LeftParen)) { parenDepth++; cur++; continue; }
                if (bodyTokens[cur].is(TokenKind::RightParen)) { if (parenDepth > 0) parenDepth--; cur++; continue; }
                if (bodyTokens[cur].is(TokenKind::LeftBracket)) { bracketDepth++; cur++; continue; }
                if (bodyTokens[cur].is(TokenKind::RightBracket)) { if (bracketDepth > 0) bracketDepth--; cur++; continue; }
                if (bodyTokens[cur].is(TokenKind::LeftBrace)) { braceDepth++; cur++; continue; }
                if (bodyTokens[cur].is(TokenKind::RightBrace)) { if (braceDepth > 0) braceDepth--; cur++; continue; }

                // 1. Ownership transfer: move <var>
                if (bodyTokens[cur].is(TokenKind::KwMove)) {
                    Token moveTok = bodyTokens[cur++];
                    if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::Identifier)) {
                        Token targetTok = bodyTokens[cur++];
                        VariableSymbol* target = currentScope->lookup(targetTok.text);
                        if (target) {
                            if (target->state == OwnershipState::Moved) {
                                _reportUseOfMoved(targetTok.text, targetTok.loc, *target);
                            } else if (target->type.isReference()) {
                                size_t len = targetTok.loc.column + targetTok.text.size() - moveTok.loc.column;
                                Diagnostic d(ErrorCode::E0506_CannotMoveBorrowed,
                                             "cannot move out of borrowed reference '" + targetTok.text + "'",
                                             moveTok.loc, len, "cannot move out of reference");
                                d.withSecondary(target->declaredLoc, targetTok.text.size(), "reference declared here");
                                d.withHelp("borrowed references do not have ownership of the underlying data");
                                _report(d);
                            } else if (target->state == OwnershipState::BorrowedImmutable || target->state == OwnershipState::BorrowedMutable) {
                                size_t len = targetTok.loc.column + targetTok.text.size() - moveTok.loc.column;
                                Diagnostic d(ErrorCode::E0506_CannotMoveBorrowed,
                                             "cannot move out of '" + targetTok.text + "' because it is borrowed",
                                             moveTok.loc, len, "cannot move out of borrowed value");
                                SourceLocation bLoc = target->borrowLocs.empty() ? target->declaredLoc : target->borrowLocs.back();
                                d.withSecondary(bLoc, targetTok.text.size(), "borrow occurs here");
                                d.withHelp("borrow must end before value can be moved");
                                _report(d);
                            } else {
                                target->state = OwnershipState::Moved;
                                target->movedLoc = moveTok.loc;
                            }
                        }
                    }
                    continue;
                }

                // 2. Borrow expression: &mut <var> or &<var>
                if (bodyTokens[cur].is(TokenKind::Ampersand)) {
                    Token ampTok = bodyTokens[cur++];
                    bool isMut = false;
                    if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::KwMut)) {
                        isMut = true;
                        cur++;
                    }
                    if (cur < bodyTokens.size() && bodyTokens[cur].is(TokenKind::Identifier)) {
                        Token targetTok = bodyTokens[cur++];
                        VariableSymbol* target = currentScope->lookup(targetTok.text);
                        if (target) {
                            if (target->state == OwnershipState::Moved) {
                                _reportUseOfMoved(targetTok.text, targetTok.loc, *target);
                            }
                            size_t spanLen = targetTok.loc.column + targetTok.text.size() - ampTok.loc.column;
                            if (isMut) {
                                if (target->state == OwnershipState::BorrowedImmutable || target->immutableBorrowCount > 0) {
                                    Diagnostic d(ErrorCode::E0502_BorrowConflict,
                                                 "cannot borrow '" + targetTok.text + "' as mutable because it is also borrowed as immutable",
                                                 ampTok.loc, spanLen, "mutable borrow occurs here");
                                    SourceLocation prevLoc = target->borrowLocs.empty() ? target->declaredLoc : target->borrowLocs.front();
                                    d.withSecondary(prevLoc, targetTok.text.size(), "immutable borrow occurs here");
                                    d.withHelp("mutable borrow conflicts with existing immutable borrow(s)");
                                    _report(d);
                                } else if (target->state == OwnershipState::BorrowedMutable) {
                                    Diagnostic d(ErrorCode::E0503_MultipleMutableBorrows,
                                                 "cannot borrow '" + targetTok.text + "' as mutable more than once at a time",
                                                 ampTok.loc, spanLen, "second mutable borrow occurs here");
                                    SourceLocation prevLoc = target->borrowLocs.empty() ? target->declaredLoc : target->borrowLocs.front();
                                    d.withSecondary(prevLoc, targetTok.text.size(), "first mutable borrow occurs here");
                                    d.withHelp("first mutable borrow must end before borrowing again");
                                    _report(d);
                                } else {
                                    target->state = OwnershipState::BorrowedMutable;
                                    target->borrowLocs.push_back(ampTok.loc);
                                    ActiveBorrow ab;
                                    ab.targetVar = targetTok.text;
                                    ab.borrowerVar = boundVar;
                                    ab.kind = ReferenceKind::MutableRef;
                                    ab.loc = ampTok.loc;
                                    ab.scopeDepth = currentScope->depth;
                                    if (!boundVar.empty()) {
                                        currentScope->activeBorrows.push_back(ab);
                                    } else {
                                        tempBorrows.push_back(ab);
                                    }
                                }
                            } else {
                                if (target->state == OwnershipState::BorrowedMutable) {
                                    Diagnostic d(ErrorCode::E0502_BorrowConflict,
                                                 "cannot borrow '" + targetTok.text + "' as immutable because it is also borrowed as mutable",
                                                 ampTok.loc, spanLen, "immutable borrow occurs here");
                                    SourceLocation prevLoc = target->borrowLocs.empty() ? target->declaredLoc : target->borrowLocs.front();
                                    d.withSecondary(prevLoc, targetTok.text.size(), "mutable borrow occurs here");
                                    d.withHelp("immutable borrow conflicts with active mutable borrow");
                                    _report(d);
                                } else {
                                    target->state = OwnershipState::BorrowedImmutable;
                                    target->immutableBorrowCount++;
                                    target->borrowLocs.push_back(ampTok.loc);
                                    ActiveBorrow ab;
                                    ab.targetVar = targetTok.text;
                                    ab.borrowerVar = boundVar;
                                    ab.kind = ReferenceKind::ImmutableRef;
                                    ab.loc = ampTok.loc;
                                    ab.scopeDepth = currentScope->depth;
                                    if (!boundVar.empty()) {
                                        currentScope->activeBorrows.push_back(ab);
                                    } else {
                                        tempBorrows.push_back(ab);
                                    }
                                }
                            }
                        }
                    }
                    continue;
                }

                // 3. Identifier usage
                if (bodyTokens[cur].is(TokenKind::Identifier)) {
                    Token idTok = bodyTokens[cur++];
                    bool isMember = (cur >= 2 && bodyTokens[cur - 2].is(TokenKind::Dot));
                    if (!isMember) {
                        VariableSymbol* sym = currentScope->lookup(idTok.text);
                        if (sym && sym->state == OwnershipState::Moved) {
                            _reportUseOfMoved(idTok.text, idTok.loc, *sym);
                        }
                    }
                    continue;
                }

                cur++;
            }
        };

        size_t cursor = 0;
        if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::LeftBrace)) {
            cursor++;
        }

        while (cursor < bodyTokens.size()) {
            // Nested block enter
            if (bodyTokens[cursor].is(TokenKind::LeftBrace)) {
                pushScope();
                cursor++;
                continue;
            }

            // Block exit
            if (bodyTokens[cursor].is(TokenKind::RightBrace)) {
                if (currentScope->parent != nullptr) {
                    popScope();
                    cursor++;
                    continue;
                } else {
                    cursor++;
                    break;
                }
            }

            // Semicolon statement terminator
            if (bodyTokens[cursor].is(TokenKind::Semicolon)) {
                clearTempBorrows();
                cursor++;
                continue;
            }

            // Variable binding: let [mut] <name> [: <type>] [= <expr>];
            if (bodyTokens[cursor].is(TokenKind::KwLet) || bodyTokens[cursor].is(TokenKind::KwConst)) {
                cursor++; // skip 'let' or 'const'
                bool isMut = false;
                if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::KwMut)) {
                    isMut = true;
                    cursor++;
                }
                if (cursor >= bodyTokens.size() || !bodyTokens[cursor].is(TokenKind::Identifier)) {
                    cursor++;
                    continue;
                }
                Token nameTok = bodyTokens[cursor++];
                std::string varName = nameTok.text;
                TypeAnnotation ann;

                if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::Colon)) {
                    cursor++;
                    std::string rawType;
                    while (cursor < bodyTokens.size() && !bodyTokens[cursor].is(TokenKind::Assign) && !bodyTokens[cursor].is(TokenKind::Semicolon)) {
                        if (!rawType.empty() && rawType.back() != '&') rawType += " ";
                        rawType += bodyTokens[cursor++].text;
                    }
                    ann = TypeAnnotation::parse(rawType);
                }

                VariableSymbol sym;
                sym.name = varName;
                sym.type = ann;
                sym.isMutableBinding = isMut;
                sym.declaredLoc = nameTok.loc;
                sym.state = OwnershipState::Owned;
                currentScope->variables[varName] = sym;

                if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::Assign)) {
                    cursor++; // skip '='
                    checkExpression(cursor, varName);
                }
                continue;
            }

            // Return statement: return [&|&mut] [<expr>];
            if (bodyTokens[cursor].is(TokenKind::KwReturn)) {
                Token retTok = bodyTokens[cursor++];
                if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::Ampersand)) {
                    Token ampTok = bodyTokens[cursor++];
                    bool isMut = false;
                    if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::KwMut)) {
                        isMut = true;
                        cursor++;
                    }
                    if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::Identifier)) {
                        Token targetTok = bodyTokens[cursor++];
                        VariableSymbol* target = currentScope->lookup(targetTok.text);
                        if (target) {
                            if (target->state == OwnershipState::Moved) {
                                _reportUseOfMoved(targetTok.text, targetTok.loc, *target);
                            }
                            if (!target->isParameter) {
                                size_t len = targetTok.loc.column + targetTok.text.size() - ampTok.loc.column;
                                Diagnostic d(ErrorCode::E0505_DanglingReference,
                                             "cannot return reference to local variable '" + targetTok.text + "'",
                                             ampTok.loc, len,
                                             "returns a reference to data owned by the current function");
                                d.withSecondary(target->declaredLoc, targetTok.text.size(), "local variable declared here");
                                d.withHelp("return an owned value instead or extend the lifetime of the referenced object");
                                _report(d);
                            }
                        }
                    }
                }
                checkExpression(cursor, "");
                continue;
            }

            // Assignment: <name> = <expr>;
            if (bodyTokens[cursor].is(TokenKind::Identifier) && cursor + 1 < bodyTokens.size() &&
                (bodyTokens[cursor + 1].is(TokenKind::Assign) || bodyTokens[cursor + 1].text == "+=" || bodyTokens[cursor + 1].text == "-=")) {
                Token targetTok = bodyTokens[cursor];
                Token opTok = bodyTokens[cursor + 1];
                VariableSymbol* target = currentScope->lookup(targetTok.text);
                if (target) {
                    if (target->state == OwnershipState::Moved) {
                        if (opTok.is(TokenKind::Assign)) {
                            target->state = OwnershipState::Owned;
                        } else {
                            _reportUseOfMoved(targetTok.text, targetTok.loc, *target);
                        }
                    }
                    if (target->state == OwnershipState::BorrowedImmutable ||
                        (target->state == OwnershipState::BorrowedMutable && !target->borrowLocs.empty())) {
                        Diagnostic d(ErrorCode::E0504_MutationDuringBorrow,
                                     "cannot assign to or mutate '" + targetTok.text + "' because it is borrowed",
                                     targetTok.loc, targetTok.text.size(), "assignment occurs here");
                        SourceLocation bLoc = target->borrowLocs.empty() ? target->declaredLoc : target->borrowLocs.back();
                        d.withSecondary(bLoc, targetTok.text.size(), "borrow occurs here");
                        d.withHelp("cannot modify variable while active borrows exist");
                        _report(d);
                    }
                }
                cursor += 2;
                checkExpression(cursor, "");
                continue;
            }

            // Method mutation access: <name>.<mutatingMethod>(...)
            if (bodyTokens[cursor].is(TokenKind::Identifier) && cursor + 1 < bodyTokens.size() &&
                bodyTokens[cursor + 1].is(TokenKind::Dot)) {
                Token targetTok = bodyTokens[cursor];
                VariableSymbol* target = currentScope->lookup(targetTok.text);
                if (target) {
                    if (target->state == OwnershipState::Moved) {
                        _reportUseOfMoved(targetTok.text, targetTok.loc, *target);
                    } else if (cursor + 2 < bodyTokens.size()) {
                        std::string method = bodyTokens[cursor + 2].text;
                        if (method == "append" || method == "clear" || method == "push" ||
                            method == "push_back" || method == "insert" || method == "write") {
                            if (target->state == OwnershipState::BorrowedImmutable) {
                                Diagnostic d(ErrorCode::E0504_MutationDuringBorrow,
                                             "cannot assign to or mutate '" + targetTok.text + "' because it is borrowed",
                                             targetTok.loc, targetTok.text.size(), "mutation occurs here");
                                SourceLocation bLoc = target->borrowLocs.empty() ? target->declaredLoc : target->borrowLocs.back();
                                d.withSecondary(bLoc, targetTok.text.size(), "borrow occurs here");
                                d.withHelp("cannot modify variable while active borrows exist");
                                _report(d);
                            }
                        }
                    }
                }
                cursor += 2;
                checkExpression(cursor, "");
                continue;
            }

            // For loop
            if (bodyTokens[cursor].is(TokenKind::KwFor)) {
                cursor++;
                if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::LeftParen)) cursor++;
                if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::KwLet)) cursor++;
                if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::Identifier)) {
                    Token itemTok = bodyTokens[cursor++];
                    while (cursor < bodyTokens.size() && !bodyTokens[cursor].is(TokenKind::LeftBrace)) {
                        cursor++;
                    }
                    if (cursor < bodyTokens.size() && bodyTokens[cursor].is(TokenKind::LeftBrace)) {
                        pushScope();
                        cursor++;
                        VariableSymbol sym;
                        sym.name = itemTok.text;
                        sym.type.baseType = "auto";
                        sym.declaredLoc = itemTok.loc;
                        currentScope->variables[itemTok.text] = sym;
                        continue;
                    }
                }
            }

            // Generic expression statement
            checkExpression(cursor, "");
        }
    }

} // namespace Keety
