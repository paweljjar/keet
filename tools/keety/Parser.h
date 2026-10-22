//
// Created by PAWEL on 19/10/2026.
//

#ifndef KEETY_PARSER_H
#define KEETY_PARSER_H

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "AST.h"
#include "Token.h"

namespace Keety {

    class ParserException : public std::runtime_error { // Specialized error indicating a syntax or grammar violation during parsing
        public:
            ParserException(std::string message, SourceLocation loc)
                : std::runtime_error(loc.toString() + ": error: " + message), _loc(std::move(loc)) {}

            [[nodiscard]] const SourceLocation& location() const { return _loc; }

        private:
            SourceLocation _loc;
    };

    class Parser { // Recursive descent parser building a complete SourceFile AST from token streams
        public:
            explicit Parser(std::vector<Token> tokens, std::string componentName = "Component")
                : _tokens(std::move(tokens)), _componentName(std::move(componentName)) {}

            std::unique_ptr<SourceFile> parse(); // Parse token stream into root SourceFile AST

        private:
            // Token navigation helpers
            [[nodiscard]] bool _isAtEnd() const { return _cursor >= _tokens.size() || _peek().is(TokenKind::Eof); }
            [[nodiscard]] const Token& _peek(size_t offset = 0) const {
                if (_cursor + offset >= _tokens.size()) return _tokens.back();
                return _tokens[_cursor + offset];
            }
            const Token& _advance() {
                if (!_isAtEnd()) _cursor++;
                return _tokens[_cursor - 1];
            }
            bool _check(TokenKind kind) const {
                if (_isAtEnd()) return false;
                return _peek().is(kind);
            }
            bool _match(TokenKind kind) {
                if (_check(kind)) {
                    _advance();
                    return true;
                }
                return false;
            }
            const Token& _consume(TokenKind kind, const std::string& errorMessage) {
                if (_check(kind)) return _advance();
                throw ParserException(errorMessage + " (got '" + _peek().text + "' of type " +
                                      std::string(Token::kindToString(_peek().kind)) + ")", _peek().loc);
            }

            // Section parsers
            std::unique_ptr<ServerBlock> _parseServer();
            std::unique_ptr<ScriptBlock> _parseScript();
            std::unique_ptr<TemplateBlock> _parseTemplate();
            std::unique_ptr<StyleBlock> _parseStyle();

            // Server declaration parsers
            std::shared_ptr<DatabaseDecl> _parseDatabase();
            std::shared_ptr<TableDecl> _parseTable();
            std::shared_ptr<TableColumnDecl> _parseTableColumn();
            std::shared_ptr<BackendServerDecl> _parseBackend();
            std::shared_ptr<RouteDecl> _parseRoute();
            std::shared_ptr<FrontendServerDecl> _parseFrontend();

            // Script declaration parsers
            std::shared_ptr<ImportCppDecl> _parseImportCpp();
            std::shared_ptr<ImportCppDecl> _parseImport();
            std::shared_ptr<StateDecl> _parseStateDecl();
            std::shared_ptr<LetDecl> _parseLetDecl();
            std::shared_ptr<ConstDecl> _parseConst(bool isExported);
            std::shared_ptr<FunctionDecl> _parseFunctionDecl();

            // Template element and control flow parsers
            std::shared_ptr<TemplateNode> _parseTemplateNode();
            std::shared_ptr<ElementNode> _parseElement();
            AttributeBinding _parseAttribute();
            EventExpression _parseEventExpression(const std::string& raw);
            std::shared_ptr<IfBlockNode> _parseIfBlock();
            std::shared_ptr<ForBlockNode> _parseForBlock();

            // Style rule parsers
            std::shared_ptr<StyleRule> _parseStyleRule();

            // Standalone pure module and OOP parsers
            std::unique_ptr<PureModule> _parsePureModule();
            std::shared_ptr<ClassDecl> _parseClass(bool isExported);
            std::shared_ptr<StructDecl> _parseStruct(bool isExported);
            std::shared_ptr<ErrtypeDecl> _parseErrtype(bool isExported);
            std::shared_ptr<StandaloneFunctionDecl> _parseStandaloneFunction(bool isExported);
            std::string _parseTypeName();
            std::string _parseStatementBody();
            static std::string transformZonk(std::string s);

            std::vector<Token> _tokens;
            std::string _componentName;
            size_t _cursor = 0;
    };

} // namespace Keety

#endif // KEETY_PARSER_H
