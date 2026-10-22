//
// Created by PAWEL on 19/10/2026.
//

#ifndef KEETY_ASTPRINTER_H
#define KEETY_ASTPRINTER_H

#include <iostream>
#include <sstream>
#include <string>

#include "AST.h"

namespace Keety {

    class ASTPrinter : public ASTVisitor { // Visual AST tree serializer and inspector for diagnostics and tests
        public:
            explicit ASTPrinter(int indentStep = 2) : _indentStep(indentStep) {}

            std::string print(const ASTNode& node) {
                _out.str("");
                _out.clear();
                _indent = 0;
                node.accept(*this);
                return _out.str();
            }

            void visit(const SourceFile& node) override {
                _emitIndent();
                _out << "SourceFile: " << node.componentName << "\n";
                _indent += _indentStep;
                if (node.isPureModule && node.moduleBlock) node.moduleBlock->accept(*this);
                if (node.server) node.server->accept(*this);
                if (node.script) node.script->accept(*this);
                if (node.templateBlock) node.templateBlock->accept(*this);
                if (node.styleBlock) node.styleBlock->accept(*this);
                _indent -= _indentStep;
            }

            void visit(const ServerBlock& node) override {
                _emitIndent();
                _out << "ServerBlock\n";
                _indent += _indentStep;
                if (node.database) node.database->accept(*this);
                if (node.backend) node.backend->accept(*this);
                if (node.frontend) node.frontend->accept(*this);
                _indent -= _indentStep;
            }

            void visit(const DatabaseDecl& node) override {
                _emitIndent();
                _out << "Database: " << node.name << "(\"" << node.filePath << "\") with "
                     << node.tables.size() << " tables\n";
                _indent += _indentStep;
                for (const auto& tbl : node.tables) tbl->accept(*this);
                _indent -= _indentStep;
            }

            void visit(const TableDecl& node) override {
                _emitIndent();
                _out << "Table: " << node.name << " (" << node.columns.size() << " columns)\n";
                _indent += _indentStep;
                for (const auto& col : node.columns) col->accept(*this);
                _indent -= _indentStep;
            }

            void visit(const TableColumnDecl& node) override {
                _emitIndent();
                _out << "Column: " << node.name << ": " << node.typeName;
                if (node.isPrimaryKey) _out << " primary_key";
                if (node.isAutoIncrement) _out << " auto_increment";
                if (node.isUnique) _out << " unique";
                if (node.isNotNull) _out << " not_null";
                if (!node.defaultValueExpr.empty()) _out << " default " << node.defaultValueExpr;
                _out << "\n";
            }

            void visit(const BackendServerDecl& node) override {
                _emitIndent();
                _out << "BackendServer: " << node.name << " (port: " << node.port
                     << ", threads: " << node.threads << ", " << node.routes.size() << " routes)\n";
                _indent += _indentStep;
                for (const auto& route : node.routes) route->accept(*this);
                _indent -= _indentStep;
            }

            void visit(const RouteDecl& node) override {
                _emitIndent();
                _out << "Route: " << node.method << " \"" << node.path << "\" => ("
                     << node.paramName << ")\n";
            }

            void visit(const FrontendServerDecl& node) override {
                _emitIndent();
                _out << "FrontendServer: " << node.name << " (port: " << node.port
                     << ", api: \"" << node.apiBaseUrl << "\", title: \"" << node.pageTitle << "\")\n";
            }

            void visit(const ScriptBlock& node) override {
                _emitIndent();
                _out << "ScriptBlock (" << node.imports.size() << " imports, "
                     << node.stateDecls.size() << " states, "
                     << node.letDecls.size() << " lets, "
                     << node.functions.size() << " functions)\n";
                _indent += _indentStep;
                for (const auto& imp : node.imports) imp->accept(*this);
                for (const auto& st : node.stateDecls) st->accept(*this);
                for (const auto& lt : node.letDecls) lt->accept(*this);
                for (const auto& fn : node.functions) fn->accept(*this);
                _indent -= _indentStep;
            }

            void visit(const ImportCppDecl& node) override {
                _emitIndent();
                _out << "ImportCpp: \"" << node.headerPath << "\"";
                if (node.kind == ImportCppDecl::ImportKind::ModuleAlias) {
                    _out << " as " << node.asAlias;
                } else {
                    _out << " { ";
                    for (size_t i = 0; i < node.symbols.size(); ++i) {
                        if (i > 0) _out << ", ";
                        _out << node.symbols[i];
                    }
                    _out << " }";
                }
                _out << "\n";
            }

            void visit(const StateDecl& node) override {
                _emitIndent();
                _out << "StateDecl: " << node.name << ": " << node.typeName
                     << " = " << node.initialValueExpr << "\n";
            }

            void visit(const LetDecl& node) override {
                _emitIndent();
                _out << "LetDecl: " << node.name << ": " << node.typeName;
                if (!node.defaultValueExpr.empty()) {
                    _out << " = " << node.defaultValueExpr;
                }
                _out << "\n";
            }

            void visit(const ConstDecl& node) override {
                _emitIndent();
                _out << "ConstDecl: " << (node.isExported ? "export " : "") << "const " << node.name << ": " << node.typeName;
                if (!node.valueExpr.empty()) {
                    _out << " = " << node.valueExpr;
                }
                _out << "\n";
            }

            void visit(const FunctionDecl& node) override {
                _emitIndent();
                _out << "FunctionDecl: def " << node.name << "(";
                for (size_t i = 0; i < node.parameters.size(); ++i) {
                    if (i > 0) _out << ", ";
                    _out << node.parameters[i].name << ": " << node.parameters[i].typeName;
                }
                _out << ") -> " << node.returnType << "\n";
            }

            void visit(const TemplateBlock& node) override {
                _emitIndent();
                _out << "TemplateBlock (" << node.roots.size() << " roots)\n";
                _indent += _indentStep;
                for (const auto& child : node.roots) {
                    child->accept(*this);
                }
                _indent -= _indentStep;
            }

            void visit(const ElementNode& node) override {
                _emitIndent();
                _out << "<" << node.tagName;
                for (const auto& attr : node.attributes) {
                    if (attr.kind == AttributeKind::StaticLiteral) {
                        _out << " " << attr.name << "=\"" << attr.rawValue << "\"";
                    } else if (attr.kind == AttributeKind::DynamicExpression) {
                        _out << " " << attr.name << "={" << attr.rawValue << "}";
                    } else if (attr.kind == AttributeKind::TwoWayBinding) {
                        _out << " bind:" << attr.bindTargetProperty << "={" << attr.bindStateVariable << "}";
                    } else if (attr.kind == AttributeKind::EventCallback) {
                        _out << " " << attr.name << "={" << attr.rawValue << "}";
                    }
                }
                if (node.isSelfClosing && node.children.empty()) {
                    _out << " />\n";
                    return;
                }
                _out << ">\n";
                _indent += _indentStep;
                for (const auto& child : node.children) {
                    child->accept(*this);
                }
                _indent -= _indentStep;
                _emitIndent();
                _out << "</" << node.tagName << ">\n";
            }

            void visit(const TextNode& node) override {
                _emitIndent();
                _out << "Text: \"" << node.text << "\"\n";
            }

            void visit(const InterpolationNode& node) override {
                _emitIndent();
                _out << "Interpolation: {" << node.expression << "}\n";
            }

            void visit(const IfBlockNode& node) override {
                for (size_t i = 0; i < node.branches.size(); ++i) {
                    _emitIndent();
                    if (i == 0) {
                        _out << "If: " << node.branches[i].conditionExpr << "\n";
                    } else if (!node.branches[i].conditionExpr.empty()) {
                        _out << "Elif: " << node.branches[i].conditionExpr << "\n";
                    } else {
                        _out << "Else:\n";
                    }
                    _indent += _indentStep;
                    for (const auto& child : node.branches[i].children) {
                        child->accept(*this);
                    }
                    _indent -= _indentStep;
                }
            }

            void visit(const ForBlockNode& node) override {
                _emitIndent();
                _out << "For: " << node.itemIdentifier << " in " << node.collectionExpr << "\n";
                _indent += _indentStep;
                for (const auto& child : node.children) {
                    child->accept(*this);
                }
                _indent -= _indentStep;
            }

            void visit(const StyleBlock& node) override {
                _emitIndent();
                _out << "StyleBlock (" << node.rules.size() << " rules)\n";
                _indent += _indentStep;
                for (const auto& rule : node.rules) {
                    rule->accept(*this);
                }
                _indent -= _indentStep;
            }

            void visit(const StyleRule& node) override {
                _emitIndent();
                _out << "Rule: \"" << node.selector << "\" {\n";
                _indent += _indentStep;
                for (const auto& decl : node.declarations) {
                    _emitIndent();
                    _out << decl.property << ": " << decl.value << ";\n";
                }
                _indent -= _indentStep;
                _emitIndent();
                _out << "}\n";
            }

            void visit(const ClassFieldDecl& node) override {
                _emitIndent();
                std::string vis = node.visibility == Visibility::Public ? "public" :
                                  (node.visibility == Visibility::Private ? "private" : "protected");
                _out << "ClassField: " << vis << " " << node.name << ": " << node.typeName;
                if (!node.defaultValueExpr.empty()) {
                    _out << " = " << node.defaultValueExpr;
                }
                _out << "\n";
            }

            void visit(const ConstructorDecl& node) override {
                _emitIndent();
                _out << "Constructor: " << node.name << "(";
                for (size_t i = 0; i < node.parameters.size(); ++i) {
                    if (i > 0) _out << ", ";
                    _out << node.parameters[i].name << ": " << node.parameters[i].typeName;
                }
                _out << ")\n";
            }

            void visit(const ClassMethodDecl& node) override {
                _emitIndent();
                std::string vis = node.visibility == Visibility::Public ? "public" :
                                  (node.visibility == Visibility::Private ? "private" : "protected");
                _out << "ClassMethod: " << vis << " fn " << node.name << "(";
                for (size_t i = 0; i < node.parameters.size(); ++i) {
                    if (i > 0) _out << ", ";
                    _out << node.parameters[i].name << ": " << node.parameters[i].typeName;
                }
                _out << "): " << node.returnType << "\n";
            }

            void visit(const ClassDecl& node) override {
                _emitIndent();
                if (node.isExported) _out << "export ";
                _out << "Class: " << node.name << " (" << node.fields.size() << " fields, "
                     << node.constructors.size() << " ctors, " << node.methods.size() << " methods)\n";
                _indent += _indentStep;
                for (const auto& fld : node.fields) fld->accept(*this);
                for (const auto& ctor : node.constructors) ctor->accept(*this);
                for (const auto& mtd : node.methods) mtd->accept(*this);
                _indent -= _indentStep;
            }

            void visit(const StructDecl& node) override {
                _emitIndent();
                if (node.isExported) _out << "export ";
                _out << "Struct: " << node.name << " (" << node.fields.size() << " fields)\n";
                _indent += _indentStep;
                for (const auto& fld : node.fields) fld->accept(*this);
                _indent -= _indentStep;
            }

            void visit(const ErrtypeDecl& node) override {
                _emitIndent();
                if (node.isExported) _out << "export ";
                _out << "Errtype: " << node.name << " (" << node.fields.size() << " entries)\n";
                _indent += _indentStep;
                for (const auto& entry : node.fields) {
                    _emitIndent();
                    _out << entry.name;
                    if (entry.hasCode) _out << " = " << entry.code;
                    _out << "\n";
                }
                _indent -= _indentStep;
            }

            void visit(const StandaloneFunctionDecl& node) override {
                _emitIndent();
                if (node.isExported) _out << "export ";
                _out << "Fn: " << node.name << "(";
                for (size_t i = 0; i < node.parameters.size(); ++i) {
                    if (i > 0) _out << ", ";
                    _out << node.parameters[i].name << ": " << node.parameters[i].typeName;
                }
                _out << "): " << node.returnType;
                if (node.isMain) _out << " [main]";
                _out << "\n";
            }

            void visit(const PureModule& node) override {
                _emitIndent();
                _out << "PureModule: " << node.moduleName << " (" << node.imports.size() << " imports, "
                     << node.classes.size() << " classes, " << node.structs.size() << " structs, "
                     << node.errtypes.size() << " errtypes, "
                     << node.functions.size() << " functions)\n";
                _indent += _indentStep;
                for (const auto& imp : node.imports) imp->accept(*this);
                for (const auto& cls : node.classes) cls->accept(*this);
                for (const auto& st : node.structs) st->accept(*this);
                for (const auto& et : node.errtypes) et->accept(*this);
                for (const auto& fn : node.functions) fn->accept(*this);
                _indent -= _indentStep;
            }

        private:
            void _emitIndent() {
                for (int i = 0; i < _indent; ++i) {
                    _out << ' ';
                }
            }

            int _indent = 0;
            int _indentStep = 2;
            std::ostringstream _out;
    };

} // namespace Keety

#endif // KEETY_ASTPRINTER_H
