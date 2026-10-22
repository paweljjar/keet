//
// Created by PAWEL on 19/10/2026.
//

#include "Parser.h"
#include <sstream>

namespace Keety {

    std::unique_ptr<SourceFile> Parser::parse() {
        auto file = std::make_unique<SourceFile>(_componentName, _peek().loc);

        if (_isAtEnd()) {
            return file;
        }

        // If the first non-EOF token is NOT an SFC section tag, this is a pure KeetScript module
        if (!_check(TokenKind::TagServerOpen) &&
            !_check(TokenKind::TagScriptOpen) &&
            !_check(TokenKind::TagTemplateOpen) &&
            !_check(TokenKind::TagStyleOpen)) {
            file->isPureModule = true;
            file->moduleBlock = _parsePureModule();
            file->isSystemsMode = _isSystemsMode || (file->moduleBlock && file->moduleBlock->isSystems);
            return file;
        }

        while (!_isAtEnd()) {
            if (_check(TokenKind::TagServerOpen)) {
                if (file->server) {
                    throw ParserException("Duplicate <server> block in SFC", _peek().loc);
                }
                file->server = _parseServer();
            } else if (_check(TokenKind::TagScriptOpen)) {
                if (file->script) {
                    throw ParserException("Duplicate <script> block in SFC", _peek().loc);
                }
                file->script = _parseScript();
            } else if (_check(TokenKind::TagTemplateOpen)) {
                if (file->templateBlock) {
                    throw ParserException("Duplicate <template> block in SFC", _peek().loc);
                }
                file->templateBlock = _parseTemplate();
            } else if (_check(TokenKind::TagStyleOpen)) {
                if (file->styleBlock) {
                    throw ParserException("Duplicate <style> block in SFC", _peek().loc);
                }
                file->styleBlock = _parseStyle();
            } else {
                throw ParserException("Unexpected token at top level of SFC: '" + _peek().text + "'", _peek().loc);
            }
        }

        file->isSystemsMode = _isSystemsMode;
        return file;
    }

    std::unique_ptr<ServerBlock> Parser::_parseServer() {
        SourceLocation loc = _consume(TokenKind::TagServerOpen, "Expected '<server>'").loc;
        auto server = std::make_unique<ServerBlock>(loc);

        while (!_isAtEnd() && !_check(TokenKind::TagServerClose)) {
            if (_check(TokenKind::KwDatabase)) {
                if (server->database) {
                    throw ParserException("Duplicate database declaration in <server>", _peek().loc);
                }
                server->database = _parseDatabase();
            } else if (_check(TokenKind::KwBackend)) {
                if (server->backend) {
                    throw ParserException("Duplicate backend declaration in <server>", _peek().loc);
                }
                server->backend = _parseBackend();
            } else if (_check(TokenKind::KwFrontend)) {
                if (server->frontend) {
                    throw ParserException("Duplicate frontend declaration in <server>", _peek().loc);
                }
                server->frontend = _parseFrontend();
            } else {
                throw ParserException("Unexpected declaration in <server>: '" + _peek().text + "'", _peek().loc);
            }
        }

        _consume(TokenKind::TagServerClose, "Expected '</server>' to close server section");
        return server;
    }

    std::shared_ptr<DatabaseDecl> Parser::_parseDatabase() {
        SourceLocation loc = _consume(TokenKind::KwDatabase, "Expected 'database'").loc;
        auto db = std::make_shared<DatabaseDecl>(loc);

        db->name = _consume(TokenKind::Identifier, "Expected database variable identifier").text;
        _consume(TokenKind::LeftParen, "Expected '(' after database name");
        db->filePath = _consume(TokenKind::StringLiteral, "Expected database file path string").text;
        _consume(TokenKind::RightParen, "Expected ')' after database file path");

        _consume(TokenKind::LeftBrace, "Expected '{' to start database table definitions");
        while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
            db->tables.push_back(_parseTable());
        }
        _consume(TokenKind::RightBrace, "Expected '}' after database table definitions");
        _match(TokenKind::Semicolon); // Optional trailing semicolon

        return db;
    }

    std::shared_ptr<TableDecl> Parser::_parseTable() {
        SourceLocation loc = _consume(TokenKind::KwTable, "Expected 'table'").loc;
        auto table = std::make_shared<TableDecl>(loc);

        table->name = _consume(TokenKind::Identifier, "Expected table name identifier").text;
        _consume(TokenKind::LeftBrace, "Expected '{' to begin table column definitions");

        while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
            table->columns.push_back(_parseTableColumn());
            if (_check(TokenKind::Comma)) {
                _advance();
            }
        }

        _consume(TokenKind::RightBrace, "Expected '}' to close table column definitions");
        _match(TokenKind::Semicolon); // Optional trailing semicolon
        return table;
    }

    std::shared_ptr<TableColumnDecl> Parser::_parseTableColumn() {
        SourceLocation loc = _peek().loc;
        auto col = std::make_shared<TableColumnDecl>(loc);

        col->name = _consume(TokenKind::Identifier, "Expected column name").text;
        _consume(TokenKind::Colon, "Expected ':' after column name");
        col->typeName = _consume(TokenKind::Identifier, "Expected column type name").text;

        // Parse modifiers: primary_key, auto_increment, unique, not_null, default <val>
        while (!_isAtEnd() && !_check(TokenKind::Comma) && !_check(TokenKind::RightBrace)) {
            if (_match(TokenKind::KwPrimaryKey)) {
                col->isPrimaryKey = true;
            } else if (_match(TokenKind::KwAutoIncrement)) {
                col->isAutoIncrement = true;
            } else if (_match(TokenKind::KwUnique)) {
                col->isUnique = true;
            } else if (_match(TokenKind::KwNotNull)) {
                col->isNotNull = true;
            } else if (_match(TokenKind::KwDefault)) {
                if (_check(TokenKind::StringLiteral) || _check(TokenKind::NumberLiteral) || _check(TokenKind::Identifier)) {
                    col->defaultValueExpr = _advance().text;
                }
            } else {
                break;
            }
        }

        return col;
    }

    std::shared_ptr<BackendServerDecl> Parser::_parseBackend() {
        SourceLocation loc = _consume(TokenKind::KwBackend, "Expected 'backend'").loc;
        auto backend = std::make_shared<BackendServerDecl>(loc);

        backend->name = _consume(TokenKind::Identifier, "Expected backend server variable identifier").text;
        _consume(TokenKind::LeftParen, "Expected '(' after backend identifier");

        // Parse arguments: port: <num>, threads: <num>
        while (!_isAtEnd() && !_check(TokenKind::RightParen)) {
            std::string key = _consume(TokenKind::Identifier, "Expected argument name in backend configuration").text;
            _consume(TokenKind::Colon, "Expected ':' after configuration key");
            if (key == "port") {
                backend->port = std::stoi(_consume(TokenKind::NumberLiteral, "Expected port number").text);
            } else if (key == "threads") {
                backend->threads = std::stoi(_consume(TokenKind::NumberLiteral, "Expected thread count").text);
            } else {
                _advance(); // Skip unrecognized argument
            }
            if (_check(TokenKind::Comma)) _advance();
        }
        _consume(TokenKind::RightParen, "Expected ')' after backend configuration");

        _consume(TokenKind::LeftBrace, "Expected '{' to start backend route definitions");
        while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
            backend->routes.push_back(_parseRoute());
        }
        _consume(TokenKind::RightBrace, "Expected '}' after backend route definitions");
        _match(TokenKind::Semicolon); // Optional trailing semicolon

        return backend;
    }

    std::shared_ptr<RouteDecl> Parser::_parseRoute() {
        SourceLocation loc = _peek().loc;
        auto route = std::make_shared<RouteDecl>(loc);

        if (_match(TokenKind::KwGet)) {
            route->method = "GET";
        } else if (_match(TokenKind::KwPost)) {
            route->method = "POST";
        } else if (_match(TokenKind::KwPut)) {
            route->method = "PUT";
        } else if (_match(TokenKind::KwDelete)) {
            route->method = "DELETE";
        } else {
            throw ParserException("Expected HTTP method (get, post, put, delete) for route", loc);
        }

        route->path = _consume(TokenKind::StringLiteral, "Expected route path string").text;
        _consume(TokenKind::FatArrow, "Expected '=>' after route path");

        _consume(TokenKind::LeftParen, "Expected '(' for route handler parameter");
        if (_check(TokenKind::Identifier)) {
            route->paramName = _advance().text;
        }
        _consume(TokenKind::RightParen, "Expected ')' after route handler parameter");

        // Parse route handler body: { ... }
        _consume(TokenKind::LeftBrace, "Expected '{' to start route handler body");
        int braceDepth = 1;
        std::ostringstream bodyOss;
        while (!_isAtEnd() && braceDepth > 0) {
            if (_check(TokenKind::LeftBrace)) {
                braceDepth++;
                bodyOss << _advance().text << " ";
            } else if (_check(TokenKind::RightBrace)) {
                braceDepth--;
                if (braceDepth > 0) {
                    bodyOss << _advance().text << " ";
                } else {
                    _advance(); // Consume outer closing brace
                    break;
                }
            } else if (_check(TokenKind::StringLiteral)) {
                bodyOss << "\"" << _advance().text << "\" ";
            } else {
                bodyOss << _advance().text << " ";
            }
        }
        route->body = bodyOss.str();
        _match(TokenKind::Semicolon); // Optional trailing semicolon

        return route;
    }

    std::shared_ptr<FrontendServerDecl> Parser::_parseFrontend() {
        SourceLocation loc = _consume(TokenKind::KwFrontend, "Expected 'frontend'").loc;
        auto fe = std::make_shared<FrontendServerDecl>(loc);

        fe->name = _consume(TokenKind::Identifier, "Expected frontend server variable identifier").text;
        _consume(TokenKind::LeftParen, "Expected '(' after frontend identifier");

        // Parse arguments: port: <num>, threads: <num>, api: <str>, title: <str>
        while (!_isAtEnd() && !_check(TokenKind::RightParen)) {
            std::string key = _consume(TokenKind::Identifier, "Expected argument name in frontend configuration").text;
            _consume(TokenKind::Colon, "Expected ':' after configuration key");
            if (key == "port") {
                fe->port = std::stoi(_consume(TokenKind::NumberLiteral, "Expected port number").text);
            } else if (key == "threads") {
                fe->threads = std::stoi(_consume(TokenKind::NumberLiteral, "Expected thread count").text);
            } else if (key == "api") {
                fe->apiBaseUrl = _consume(TokenKind::StringLiteral, "Expected API base URL string").text;
            } else if (key == "title") {
                fe->pageTitle = _consume(TokenKind::StringLiteral, "Expected page title string").text;
            } else {
                _advance(); // Skip unrecognized
            }
            if (_check(TokenKind::Comma)) _advance();
        }
        _consume(TokenKind::RightParen, "Expected ')' after frontend configuration");
        _match(TokenKind::Semicolon); // Semicolon optional or expected

        return fe;
    }

    std::unique_ptr<ScriptBlock> Parser::_parseScript() {
        SourceLocation loc = _consume(TokenKind::TagScriptOpen, "Expected '<script>'").loc;
        auto script = std::make_unique<ScriptBlock>(loc);

        while (!_isAtEnd() && !_check(TokenKind::TagScriptClose)) {
            if (_check(TokenKind::KwImport)) {
                script->imports.push_back(_parseImport());
            } else if (_check(TokenKind::KwState)) {
                script->stateDecls.push_back(_parseStateDecl());
            } else if (_check(TokenKind::KwLet)) {
                script->letDecls.push_back(_parseLetDecl());
            } else if (_check(TokenKind::KwConst)) {
                script->constants.push_back(_parseConst(false));
            } else if (_check(TokenKind::KwDef)) {
                script->functions.push_back(_parseFunctionDecl());
            } else if (_check(TokenKind::KwErrtype) || (_check(TokenKind::Identifier) && _peek().text == "errtype")) {
                script->errtypes.push_back(_parseErrtype(false));
            } else {
                throw ParserException("Unexpected declaration in <script>: '" + _peek().text + "'", _peek().loc);
            }
        }

        _consume(TokenKind::TagScriptClose, "Expected '</script>' to close script section");
        return script;
    }

    std::shared_ptr<ImportCppDecl> Parser::_parseImport() {
        if (_peek(1).is(TokenKind::KwCpp)) {
            return _parseImportCpp();
        }

        SourceLocation loc = _consume(TokenKind::KwImport, "Expected 'import'").loc;
        auto importDecl = std::make_shared<ImportCppDecl>(loc);

        // Case 1: import { SymbolA, SymbolB } from "path";
        if (_match(TokenKind::LeftBrace)) {
            importDecl->kind = ImportCppDecl::ImportKind::SelectiveSymbols;
            while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
                importDecl->symbols.push_back(_consume(TokenKind::Identifier, "Expected imported symbol identifier").text);
                if (!_check(TokenKind::RightBrace)) {
                    _consume(TokenKind::Comma, "Expected ',' between imported symbols");
                }
            }
            _consume(TokenKind::RightBrace, "Expected '}' closing imported symbol list");
            if (_check(TokenKind::KwFrom) || (_check(TokenKind::Identifier) && _peek().text == "from")) {
                _advance(); // consume 'from'
            }
            std::string path = _consume(TokenKind::StringLiteral, "Expected module path string").text;
            if (path.size() >= 5 && path.substr(path.size() - 5) == ".keet") {
                path = path.substr(0, path.size() - 5) + ".h";
            }
            importDecl->headerPath = path;
            _match(TokenKind::Semicolon);
            return importDecl;
        }

        // Case 2: import Symbol from "path"; OR import file; / import io;
        if (_check(TokenKind::Identifier)) {
            std::string ident = _advance().text;
            if (_check(TokenKind::KwFrom) || (_check(TokenKind::Identifier) && _peek().text == "from")) {
                _advance(); // consume 'from'
                std::string path = _consume(TokenKind::StringLiteral, "Expected module path string").text;
                if (path.size() >= 5 && path.substr(path.size() - 5) == ".keet") {
                    path = path.substr(0, path.size() - 5) + ".h";
                }
                importDecl->kind = ImportCppDecl::ImportKind::ModuleAlias;
                importDecl->asAlias = ident;
                importDecl->headerPath = path;
                _match(TokenKind::Semicolon);
                return importDecl;
            } else {
                // Built-in module or standard import: import file; import io; import math;
                importDecl->kind = ImportCppDecl::ImportKind::ModuleAlias;
                importDecl->asAlias = ident;
                if (ident == "file") importDecl->headerPath = "<keet/runtime/File.h>";
                else if (ident == "io") importDecl->headerPath = "<keet/runtime/IO.h>";
                else if (ident == "math") importDecl->headerPath = "<keet/runtime/Math.h>";
                else if (ident == "memory" || ident == "mem") importDecl->headerPath = "<keet/runtime/Memory.h>";
                else if (ident == "collections") importDecl->headerPath = "<keet/runtime/Collections.h>";
                else importDecl->headerPath = "\"" + ident + ".h\"";
                _match(TokenKind::Semicolon);
                return importDecl;
            }
        }

        // Case 3: import "path";
        if (_check(TokenKind::StringLiteral)) {
            std::string path = _advance().text;
            if (path.size() >= 5 && path.substr(path.size() - 5) == ".keet") {
                path = path.substr(0, path.size() - 5) + ".h";
            }
            importDecl->kind = ImportCppDecl::ImportKind::ModuleAlias;
            importDecl->headerPath = path;
            _match(TokenKind::Semicolon);
            return importDecl;
        }

        throw ParserException("Invalid import syntax; expected '{', identifier, or string path", _peek().loc);
    }

    std::shared_ptr<ImportCppDecl> Parser::_parseImportCpp() {
        SourceLocation loc = _consume(TokenKind::KwImport, "Expected 'import'").loc;
        _consume(TokenKind::KwCpp, "Expected 'cpp' after 'import'");

        auto importDecl = std::make_shared<ImportCppDecl>(loc);

        if (_match(TokenKind::KwFrom)) {
            // import cpp from "Header.h" as Module;
            importDecl->kind = ImportCppDecl::ImportKind::ModuleAlias;
            importDecl->headerPath = _consume(TokenKind::StringLiteral, "Expected header file path string").text;
            _consume(TokenKind::KwAs, "Expected 'as' after header file path");
            importDecl->asAlias = _consume(TokenKind::Identifier, "Expected module alias identifier").text;
            _consume(TokenKind::Semicolon, "Expected ';' after import statement");
        } else if (_match(TokenKind::LeftBrace)) {
            // import cpp { funcA, funcB } from "Header.h";
            importDecl->kind = ImportCppDecl::ImportKind::SelectiveSymbols;
            while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
                importDecl->symbols.push_back(_consume(TokenKind::Identifier, "Expected imported symbol identifier").text);
                if (!_check(TokenKind::RightBrace)) {
                    _consume(TokenKind::Comma, "Expected ',' between imported symbols");
                }
            }
            _consume(TokenKind::RightBrace, "Expected '}' closing imported symbol list");
            _consume(TokenKind::KwFrom, "Expected 'from' after imported symbols");
            importDecl->headerPath = _consume(TokenKind::StringLiteral, "Expected header file path string").text;
            _consume(TokenKind::Semicolon, "Expected ';' after import statement");
        } else {
            throw ParserException("Invalid C++ bridge import syntax; expected 'from' or '{'", _peek().loc);
        }

        return importDecl;
    }

    std::shared_ptr<StateDecl> Parser::_parseStateDecl() {
        SourceLocation loc = _consume(TokenKind::KwState, "Expected 'state'").loc;
        auto stateDecl = std::make_shared<StateDecl>(loc);
        stateDecl->name = _consume(TokenKind::Identifier, "Expected state variable identifier").text;
        _consume(TokenKind::Colon, "Expected ':' after state variable identifier");
        stateDecl->typeName = _consume(TokenKind::Identifier, "Expected state type annotation").text;

        if (_match(TokenKind::Assign)) {
            // Read initial value tokens until ';'
            std::string expr;
            while (!_isAtEnd() && !_check(TokenKind::Semicolon)) {
                if (!expr.empty()) expr += " ";
                if (_peek().is(TokenKind::StringLiteral)) {
                    expr += "\"" + _advance().text + "\"";
                } else {
                    expr += _advance().text;
                }
            }
            stateDecl->initialValueExpr = expr;
        }

        _consume(TokenKind::Semicolon, "Expected ';' after state declaration");
        return stateDecl;
    }

    std::shared_ptr<LetDecl> Parser::_parseLetDecl() {
        SourceLocation loc = _consume(TokenKind::KwLet, "Expected 'let'").loc;
        auto letDecl = std::make_shared<LetDecl>(loc);
        letDecl->name = _consume(TokenKind::Identifier, "Expected variable identifier").text;

        if (_match(TokenKind::Colon)) {
            letDecl->typeName = _consume(TokenKind::Identifier, "Expected type annotation").text;
        }

        if (_match(TokenKind::Assign)) {
            std::string expr;
            while (!_isAtEnd() && !_check(TokenKind::Semicolon)) {
                if (!expr.empty()) expr += " ";
                if (_peek().is(TokenKind::StringLiteral)) {
                    expr += "\"" + _advance().text + "\"";
                } else {
                    expr += _advance().text;
                }
            }
            letDecl->defaultValueExpr = expr;
        }

        _consume(TokenKind::Semicolon, "Expected ';' after let declaration");
        return letDecl;
    }

    std::shared_ptr<ConstDecl> Parser::_parseConst(bool isExported) {
        SourceLocation loc = _consume(TokenKind::KwConst, "Expected 'const'").loc;
        auto constDecl = std::make_shared<ConstDecl>(loc);
        constDecl->name = _consume(TokenKind::Identifier, "Expected constant identifier").text;
        constDecl->isExported = isExported;

        if (_match(TokenKind::Colon)) {
            constDecl->typeName = _parseTypeName();
        }

        _consume(TokenKind::Assign, "Expected '=' after constant identifier");

        std::string expr;
        while (!_isAtEnd() && !_check(TokenKind::Semicolon)) {
            if (!expr.empty()) expr += " ";
            if (_peek().is(TokenKind::StringLiteral)) {
                expr += "\"" + _advance().text + "\"";
            } else {
                expr += _advance().text;
            }
        }
        _consume(TokenKind::Semicolon, "Expected ';' after constant declaration");
        constDecl->valueExpr = transformZonk(expr);
        return constDecl;
    }

    std::shared_ptr<FunctionDecl> Parser::_parseFunctionDecl() {
        SourceLocation loc = _consume(TokenKind::KwDef, "Expected 'def'").loc;
        auto fn = std::make_shared<FunctionDecl>(loc);
        fn->name = _consume(TokenKind::Identifier, "Expected function name").text;
        _consume(TokenKind::LeftParen, "Expected '(' after function name");

        while (!_isAtEnd() && !_check(TokenKind::RightParen)) {
            ParameterDecl param;
            if (_match(TokenKind::KwMove)) {
                param.isMove = true;
            }
            param.name = _consume(TokenKind::Identifier, "Expected parameter name").text;
            if (_match(TokenKind::Colon)) {
                param.typeName = _parseTypeName();
                auto ann = TypeAnnotation::parse(param.typeName);
                param.refKind = ann.refKind;
                param.lifetime = ann.lifetime;
            }
            fn->parameters.push_back(param);
            if (!_check(TokenKind::RightParen)) {
                _consume(TokenKind::Comma, "Expected ',' between parameters");
            }
        }
        _consume(TokenKind::RightParen, "Expected ')' closing parameter list");

        if (_match(TokenKind::Arrow) || _match(TokenKind::Colon)) {
            fn->returnType = _parseTypeName();
        }

        fn->body = _parseStatementBody(&fn->bodyTokens);
        return fn;
    }

    std::unique_ptr<TemplateBlock> Parser::_parseTemplate() {
        SourceLocation loc = _consume(TokenKind::TagTemplateOpen, "Expected '<template>'").loc;
        auto tmpl = std::make_unique<TemplateBlock>(loc);

        while (!_isAtEnd() && !_check(TokenKind::TagTemplateClose)) {
            auto node = _parseTemplateNode();
            if (node) {
                tmpl->roots.push_back(node);
            }
        }

        _consume(TokenKind::TagTemplateClose, "Expected '</template>' to close template section");
        return tmpl;
    }

    std::shared_ptr<TemplateNode> Parser::_parseTemplateNode() {
        if (_check(TokenKind::LessThan)) {
            return _parseElement();
        } else if (_check(TokenKind::LeftBrace)) {
            SourceLocation loc = _advance().loc; // Consume '{'
            std::string expr = _consume(TokenKind::RawText, "Expected expression inside interpolation").text;
            _consume(TokenKind::RightBrace, "Expected '}' after interpolation");
            return std::make_shared<InterpolationNode>(transformZonk(expr), loc);
        } else if (_check(TokenKind::KwIf)) {
            return _parseIfBlock();
        } else if (_check(TokenKind::KwFor)) {
            return _parseForBlock();
        } else if (_check(TokenKind::RawText)) {
            Token textTok = _advance();
            return std::make_shared<TextNode>(textTok.text, textTok.loc);
        } else {
            // Advance unexpected token to avoid infinite loop
            _advance();
            return nullptr;
        }
    }

    EventExpression Parser::_parseEventExpression(const std::string& raw) {
        EventExpression ev;
        ev.rawExpression = raw;

        // Parse inline mutation: () => count += 1 or () => count = count + 1
        size_t arrowPos = raw.find("=>");
        if (arrowPos != std::string::npos) {
            std::string params = raw.substr(0, arrowPos);
            std::string body = raw.substr(arrowPos + 2);

            // Trim params
            while (!params.empty() && std::isspace(static_cast<unsigned char>(params.front()))) params.erase(params.begin());
            while (!params.empty() && std::isspace(static_cast<unsigned char>(params.back()))) params.pop_back();

            // Trim body
            while (!body.empty() && std::isspace(static_cast<unsigned char>(body.front()))) body.erase(body.begin());
            while (!body.empty() && std::isspace(static_cast<unsigned char>(body.back()))) body.pop_back();

            if (params.find('(') != std::string::npos && params.find(')') != std::string::npos) {
                size_t pstart = params.find('(') + 1;
                size_t pend = params.find(')');
                std::string innerParam = params.substr(pstart, pend - pstart);
                while (!innerParam.empty() && std::isspace(static_cast<unsigned char>(innerParam.front()))) innerParam.erase(innerParam.begin());
                while (!innerParam.empty() && std::isspace(static_cast<unsigned char>(innerParam.back()))) innerParam.pop_back();
                if (!innerParam.empty()) {
                    ev.paramStyle = EventParamStyle::EventArg;
                    ev.eventArgName = innerParam;
                } else {
                    ev.paramStyle = EventParamStyle::ZeroArg;
                }
            }

            // Inspect body for mutations: target += value, target -= value, target = value
            size_t opPos = std::string::npos;
            if ((opPos = body.find("+=")) != std::string::npos) {
                ev.mutationOp = "+=";
                ev.targetState = body.substr(0, opPos);
                ev.mutationValue = body.substr(opPos + 2);
            } else if ((opPos = body.find("-=")) != std::string::npos) {
                ev.mutationOp = "-=";
                ev.targetState = body.substr(0, opPos);
                ev.mutationValue = body.substr(opPos + 2);
            } else if ((opPos = body.find("=")) != std::string::npos && body[opPos+1] != '=') {
                ev.mutationOp = "=";
                ev.targetState = body.substr(0, opPos);
                ev.mutationValue = body.substr(opPos + 1);
            }

            if (!ev.targetState.empty()) {
                while (!ev.targetState.empty() && std::isspace(static_cast<unsigned char>(ev.targetState.front()))) ev.targetState.erase(ev.targetState.begin());
                while (!ev.targetState.empty() && std::isspace(static_cast<unsigned char>(ev.targetState.back()))) ev.targetState.pop_back();
            }
            if (!ev.mutationValue.empty()) {
                while (!ev.mutationValue.empty() && std::isspace(static_cast<unsigned char>(ev.mutationValue.front()))) ev.mutationValue.erase(ev.mutationValue.begin());
                while (!ev.mutationValue.empty() && std::isspace(static_cast<unsigned char>(ev.mutationValue.back()))) ev.mutationValue.pop_back();
            }
        } else {
            ev.paramStyle = EventParamStyle::MethodReference;
        }

        return ev;
    }

    std::shared_ptr<ElementNode> Parser::_parseElement() {
        SourceLocation loc = _consume(TokenKind::LessThan, "Expected '<'").loc;
        std::string tagName = (_check(TokenKind::Identifier) || _check(TokenKind::KwTable))
                              ? _advance().text
                              : _consume(TokenKind::Identifier, "Expected HTML tag name").text;
        auto elem = std::make_shared<ElementNode>(tagName, loc);

        // Parse attributes until '>' or '/>'
        while (!_isAtEnd() && !_check(TokenKind::GreaterThan) && !_check(TokenKind::SlashGreaterThan)) {
            elem->attributes.push_back(_parseAttribute());
        }

        if (_match(TokenKind::SlashGreaterThan)) {
            elem->isSelfClosing = true;
            return elem;
        }

        _consume(TokenKind::GreaterThan, "Expected '>' to close opening tag");

        // Parse children until '</tagName>'
        while (!_isAtEnd()) {
            if (_check(TokenKind::LessThanSlash)) {
                // Peek ahead to see if closing tag matches
                if (_cursor + 1 < _tokens.size() && _tokens[_cursor + 1].text == tagName) {
                    break;
                }
            }
            if (_check(TokenKind::TagTemplateClose)) {
                break;
            }
            auto child = _parseTemplateNode();
            if (child) {
                elem->children.push_back(child);
            }
        }

        _consume(TokenKind::LessThanSlash, "Expected '</' closing tag");
        std::string closeTag = (_check(TokenKind::Identifier) || _check(TokenKind::KwTable))
                               ? _advance().text
                               : _consume(TokenKind::Identifier, "Expected closing tag name").text;
        if (closeTag != tagName) {
            throw ParserException("Mismatched closing tag: expected '</" + tagName + ">', got '</" + closeTag + ">'", _peek().loc);
        }
        _consume(TokenKind::GreaterThan, "Expected '>' to finish closing tag");

        return elem;
    }

    AttributeBinding Parser::_parseAttribute() {
        AttributeBinding attr;
        attr.loc = _peek().loc;

        if (_match(TokenKind::DirectiveBind)) {
            // bind:value={stateVar} or bind:checked={stateVar}
            attr.kind = AttributeKind::TwoWayBinding;
            attr.bindTargetProperty = _consume(TokenKind::Identifier, "Expected target property after 'bind:'").text;
            attr.name = "bind:" + attr.bindTargetProperty;
            _consume(TokenKind::Assign, "Expected '=' after bind directive");
            _consume(TokenKind::LeftBrace, "Expected '{' in bind directive expression");
            attr.bindStateVariable = _consume(TokenKind::RawText, "Expected state variable name in bind expression").text;
            _consume(TokenKind::RightBrace, "Expected '}' in bind directive expression");
            return attr;
        }

        if (_peek().is(TokenKind::Identifier) || _peek().isKeyword()) {
            attr.name = _advance().text;
        } else {
            attr.name = _consume(TokenKind::Identifier, "Expected attribute name").text;
        }

        if (_match(TokenKind::Assign)) {
            if (_check(TokenKind::StringLiteral)) {
                attr.kind = AttributeKind::StaticLiteral;
                attr.rawValue = _advance().text;
            } else if (_match(TokenKind::LeftBrace)) {
                std::string expr = _consume(TokenKind::RawText, "Expected expression inside attribute value").text;
                _consume(TokenKind::RightBrace, "Expected '}' closing attribute expression");
                attr.rawValue = transformZonk(expr);

                if (attr.name.rfind("on", 0) == 0 && attr.name.size() > 2 && std::isupper(static_cast<unsigned char>(attr.name[2]))) {
                    attr.kind = AttributeKind::EventCallback;
                    attr.eventExpr = _parseEventExpression(expr);
                } else {
                    attr.kind = AttributeKind::DynamicExpression;
                }
            }
        } else {
            // Boolean static attribute like disabled or checked
            attr.kind = AttributeKind::StaticLiteral;
            attr.rawValue = "true";
        }

        return attr;
    }

    std::shared_ptr<IfBlockNode> Parser::_parseIfBlock() {
        SourceLocation loc = _consume(TokenKind::KwIf, "Expected 'if'").loc;
        auto ifNode = std::make_shared<IfBlockNode>(loc);

        // Parse primary 'if' branch
        IfBranch ifBranch;
        ifBranch.loc = loc;
        std::string cond;
        while (!_isAtEnd() && !_check(TokenKind::Colon)) {
            if (!cond.empty()) cond += " ";
            cond += _advance().text;
        }
        _consume(TokenKind::Colon, "Expected ':' after if condition");
        ifBranch.conditionExpr = cond;

        // Collect children until elif, else, </tag>, or </template>
        while (!_isAtEnd() && !_check(TokenKind::KwElif) && !_check(TokenKind::KwElse) &&
               !_check(TokenKind::LessThanSlash) && !_check(TokenKind::TagTemplateClose)) {
            auto child = _parseTemplateNode();
            if (child) ifBranch.children.push_back(child);
        }
        ifNode->branches.push_back(ifBranch);

        // Parse zero or more 'elif' branches
        while (_match(TokenKind::KwElif)) {
            IfBranch elifBranch;
            elifBranch.loc = _peek().loc;
            std::string elifCond;
            while (!_isAtEnd() && !_check(TokenKind::Colon)) {
                if (!elifCond.empty()) elifCond += " ";
                elifCond += _advance().text;
            }
            _consume(TokenKind::Colon, "Expected ':' after elif condition");
            elifBranch.conditionExpr = elifCond;

            while (!_isAtEnd() && !_check(TokenKind::KwElif) && !_check(TokenKind::KwElse) &&
                   !_check(TokenKind::LessThanSlash) && !_check(TokenKind::TagTemplateClose)) {
                auto child = _parseTemplateNode();
                if (child) elifBranch.children.push_back(child);
            }
            ifNode->branches.push_back(elifBranch);
        }

        // Parse optional 'else' branch
        if (_match(TokenKind::KwElse)) {
            IfBranch elseBranch;
            elseBranch.loc = _peek().loc;
            _consume(TokenKind::Colon, "Expected ':' after else");
            elseBranch.conditionExpr = ""; // unconditional

            while (!_isAtEnd() && !_check(TokenKind::LessThanSlash) && !_check(TokenKind::TagTemplateClose)) {
                auto child = _parseTemplateNode();
                if (child) elseBranch.children.push_back(child);
            }
            ifNode->branches.push_back(elseBranch);
        }

        return ifNode;
    }

    std::shared_ptr<ForBlockNode> Parser::_parseForBlock() {
        SourceLocation loc = _consume(TokenKind::KwFor, "Expected 'for'").loc;
        auto forNode = std::make_shared<ForBlockNode>(loc);

        forNode->itemIdentifier = _consume(TokenKind::Identifier, "Expected loop variable identifier").text;
        _consume(TokenKind::KwIn, "Expected 'in' after loop variable");

        std::string collection;
        while (!_isAtEnd() && !_check(TokenKind::Colon)) {
            if (!collection.empty()) collection += " ";
            collection += _advance().text;
        }
        _consume(TokenKind::Colon, "Expected ':' after for collection expression");
        forNode->collectionExpr = collection;

        while (!_isAtEnd() && !_check(TokenKind::LessThanSlash) && !_check(TokenKind::TagTemplateClose)) {
            auto child = _parseTemplateNode();
            if (child) forNode->children.push_back(child);
        }

        return forNode;
    }

    std::unique_ptr<StyleBlock> Parser::_parseStyle() {
        SourceLocation loc = _consume(TokenKind::TagStyleOpen, "Expected '<style>'").loc;
        auto style = std::make_unique<StyleBlock>(loc);

        while (!_isAtEnd() && !_check(TokenKind::TagStyleClose)) {
            auto rule = _parseStyleRule();
            if (rule) {
                style->rules.push_back(rule);
            }
        }

        _consume(TokenKind::TagStyleClose, "Expected '</style>' to close style section");
        return style;
    }

    std::shared_ptr<StyleRule> Parser::_parseStyleRule() {
        if (_check(TokenKind::TagStyleClose)) return nullptr;

        SourceLocation loc = _peek().loc;
        // Selector can be compound: .card, button:hover, h1, etc.
        std::string selector;
        while (!_isAtEnd() && !_check(TokenKind::LeftBrace) && !_check(TokenKind::TagStyleClose)) {
            if (!selector.empty()) selector += " ";
            selector += _advance().text;
        }

        if (selector.empty() || !_check(TokenKind::LeftBrace)) {
            if (!_isAtEnd() && !_check(TokenKind::TagStyleClose)) _advance();
            return nullptr;
        }

        _consume(TokenKind::LeftBrace, "Expected '{' starting CSS rule declarations");
        auto rule = std::make_shared<StyleRule>(loc);
        rule->selector = selector;

        while (!_isAtEnd() && !_check(TokenKind::RightBrace) && !_check(TokenKind::TagStyleClose)) {
            std::string prop = _consume(TokenKind::Identifier, "Expected CSS property name").text;
            _consume(TokenKind::Colon, "Expected ':' after CSS property name");

            std::string val;
            while (!_isAtEnd() && !_check(TokenKind::Semicolon) && !_check(TokenKind::RightBrace)) {
                if (!val.empty()) val += " ";
                val += _advance().text;
            }
            if (_check(TokenKind::Semicolon)) {
                _advance(); // Consume ';'
            }
            rule->declarations.push_back(StyleDeclaration{.property = prop, .value = val, .loc = loc});
        }

        _consume(TokenKind::RightBrace, "Expected '}' closing CSS rule declarations");
        return rule;
    }

    std::unique_ptr<PureModule> Parser::_parsePureModule() {
        auto mod = std::make_unique<PureModule>(_peek().loc);

        while (!_isAtEnd()) {
            if (_check(TokenKind::Identifier) && _peek().text == "systems" &&
                _cursor + 1 < _tokens.size() && _tokens[_cursor + 1].is(TokenKind::KwModule)) {
                _advance(); // consume 'systems'
                _advance(); // consume 'module'
                mod->moduleName = _consume(TokenKind::Identifier, "Expected module identifier").text;
                _consume(TokenKind::Semicolon, "Expected ';' after module declaration");
                mod->isSystems = true;
                _isSystemsMode = true;
            } else if (_match(TokenKind::KwModule)) {
                mod->moduleName = _consume(TokenKind::Identifier, "Expected module identifier").text;
                if (_check(TokenKind::Identifier) && _peek().text == "systems") {
                    _advance(); // consume 'systems'
                    mod->isSystems = true;
                    _isSystemsMode = true;
                }
                _consume(TokenKind::Semicolon, "Expected ';' after module declaration");
            } else if (_check(TokenKind::Invalid) && _peek().text == "@" &&
                       _cursor + 1 < _tokens.size() && _tokens[_cursor + 1].text == "systems") {
                _advance(); // @
                _advance(); // systems
                mod->isSystems = true;
                _isSystemsMode = true;
            } else if (_check(TokenKind::KwImport)) {
                mod->imports.push_back(_parseImport());
            } else if (_match(TokenKind::KwExport)) {
                if (_check(TokenKind::KwClass)) {
                    mod->classes.push_back(_parseClass(true));
                } else if (_check(TokenKind::KwStruct)) {
                    mod->structs.push_back(_parseStruct(true));
                } else if (_check(TokenKind::KwConst)) {
                    mod->constants.push_back(_parseConst(true));
                } else if (_check(TokenKind::KwErrtype) || (_check(TokenKind::Identifier) && _peek().text == "errtype")) {
                    mod->errtypes.push_back(_parseErrtype(true));
                } else if (_check(TokenKind::KwFn) || _check(TokenKind::KwDef)) {
                    mod->functions.push_back(_parseStandaloneFunction(true));
                } else {
                    throw ParserException("Expected 'class', 'struct', 'const', 'errtype', or 'fn' after 'export'", _peek().loc);
                }
            } else if (_check(TokenKind::KwClass)) {
                mod->classes.push_back(_parseClass(false));
            } else if (_check(TokenKind::KwStruct)) {
                mod->structs.push_back(_parseStruct(false));
            } else if (_check(TokenKind::KwConst)) {
                mod->constants.push_back(_parseConst(false));
            } else if (_check(TokenKind::KwErrtype) || (_check(TokenKind::Identifier) && _peek().text == "errtype")) {
                mod->errtypes.push_back(_parseErrtype(false));
            } else if (_check(TokenKind::KwFn) || _check(TokenKind::KwDef)) {
                mod->functions.push_back(_parseStandaloneFunction(false));
            } else if (_match(TokenKind::Semicolon)) {
                continue;
            } else {
                throw ParserException("Unexpected token in pure KeetScript module: '" + _peek().text + "'", _peek().loc);
            }
        }

        return mod;
    }

    std::string Parser::_parseTypeName() {
        std::string prefix;
        if (_match(TokenKind::Ampersand)) {
            prefix = "&";
            _isSystemsMode = true;
            if (_check(TokenKind::Lifetime)) {
                prefix += _advance().text + " ";
            }
            if (_match(TokenKind::KwMut)) {
                prefix += "mut ";
            }
        }

        if (_check(TokenKind::Star)) {
            throw ParserException("Raw pointers ('*') are not supported in KeetScript. KeetScript uses safe value semantics and managed types.", _peek().loc);
        }

        std::string typeName;
        if (_match(TokenKind::KwCpp)) {
            if (_match(TokenKind::Dot)) {
                typeName = _consume(TokenKind::Identifier, "Expected type name after 'cpp.'").text;
            } else {
                typeName = "cpp";
            }
        } else {
            typeName = _consume(TokenKind::Identifier, "Expected type name").text;
        }
        while ((_check(TokenKind::Colon) && _peek().text == "::") || _check(TokenKind::Dot)) {
            if (_check(TokenKind::Dot)) {
                _advance();
                typeName += "::" + _consume(TokenKind::Identifier, "Expected identifier after '.'").text;
            } else {
                typeName += _advance().text;
                typeName += _consume(TokenKind::Identifier, "Expected identifier after '::'").text;
            }
        }
        if (_match(TokenKind::LessThan)) {
            typeName += "<";
            int depth = 1;
            while (!_isAtEnd() && depth > 0) {
                if (_check(TokenKind::LessThan)) depth++;
                else if (_check(TokenKind::GreaterThan)) {
                    depth--;
                    if (depth == 0) {
                        typeName += ">";
                        _advance();
                        break;
                    }
                }
                typeName += _advance().text;
            }
        }
        if (_check(TokenKind::Star)) {
            throw ParserException("Raw pointers ('*') are not supported in KeetScript. KeetScript uses safe value semantics and managed types.", _peek().loc);
        }
        if (typeName == "io::Buffer" || typeName == "io.Buffer") typeName = "Buffer";
        if (typeName == "file::File" || typeName == "file.File") typeName = "File";
        return prefix + typeName;
    }

    std::shared_ptr<ClassDecl> Parser::_parseClass(bool isExported) {
        SourceLocation loc = _consume(TokenKind::KwClass, "Expected 'class'").loc;
        std::string className = _consume(TokenKind::Identifier, "Expected class name identifier").text;
        auto classDecl = std::make_shared<ClassDecl>(loc);
        classDecl->name = className;
        classDecl->isExported = isExported;

        _consume(TokenKind::LeftBrace, "Expected '{' to start class body");
        Visibility defaultVis = Visibility::Public;

        while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
            // Check visibility block: public:, private:, protected:
            if ((_check(TokenKind::KwPublic) || _check(TokenKind::KwPrivate) || _check(TokenKind::KwProtected)) &&
                _peek(1).is(TokenKind::Colon)) {
                if (_check(TokenKind::KwPublic)) defaultVis = Visibility::Public;
                else if (_check(TokenKind::KwPrivate)) defaultVis = Visibility::Private;
                else defaultVis = Visibility::Protected;
                _advance(); // Consume visibility keyword
                _advance(); // Consume ':'
                continue;
            }

            // Check member-level visibility prefix
            Visibility memberVis = defaultVis;
            if (_match(TokenKind::KwPublic)) {
                memberVis = Visibility::Public;
            } else if (_match(TokenKind::KwPrivate)) {
                memberVis = Visibility::Private;
            } else if (_match(TokenKind::KwProtected)) {
                memberVis = Visibility::Protected;
            }

            // 1) Constructor: ClassName(...)
            if (_check(TokenKind::Identifier) && _peek().text == className && _peek(1).is(TokenKind::LeftParen)) {
                SourceLocation ctorLoc = _advance().loc;
                auto ctor = std::make_shared<ConstructorDecl>(ctorLoc);
                ctor->name = className;
                _consume(TokenKind::LeftParen, "Expected '(' after constructor name");
                while (!_isAtEnd() && !_check(TokenKind::RightParen)) {
                    ParameterDecl param;
                    if (_match(TokenKind::KwMove)) {
                        param.isMove = true;
                        _isSystemsMode = true;
                    }
                    param.name = _consume(TokenKind::Identifier, "Expected constructor parameter name").text;
                    if (_match(TokenKind::Colon)) {
                        param.typeName = _parseTypeName();
                        auto ann = TypeAnnotation::parse(param.typeName);
                        param.refKind = ann.refKind;
                        param.lifetime = ann.lifetime;
                        if (param.refKind != ReferenceKind::Value || !param.lifetime.empty()) {
                            _isSystemsMode = true;
                        }
                    }
                    ctor->parameters.push_back(param);
                    if (!_check(TokenKind::RightParen)) {
                        _consume(TokenKind::Comma, "Expected ',' between constructor parameters");
                    }
                }
                _consume(TokenKind::RightParen, "Expected ')' closing constructor parameters");

                ctor->body = _parseStatementBody(&ctor->bodyTokens);
                classDecl->constructors.push_back(ctor);
                continue;
            }

            // 2) Method: fn methodName(...) or def methodName(...)
            if (_check(TokenKind::KwFn) || _check(TokenKind::KwDef)) {
                SourceLocation methodLoc = _advance().loc;
                auto method = std::make_shared<ClassMethodDecl>(methodLoc);
                method->visibility = memberVis;
                method->name = _consume(TokenKind::Identifier, "Expected method name").text;
                _consume(TokenKind::LeftParen, "Expected '(' after method name");
                while (!_isAtEnd() && !_check(TokenKind::RightParen)) {
                    ParameterDecl param;
                    if (_match(TokenKind::KwMove)) {
                        param.isMove = true;
                        _isSystemsMode = true;
                    }
                    param.name = _consume(TokenKind::Identifier, "Expected method parameter name").text;
                    if (_match(TokenKind::Colon)) {
                        param.typeName = _parseTypeName();
                        auto ann = TypeAnnotation::parse(param.typeName);
                        param.refKind = ann.refKind;
                        param.lifetime = ann.lifetime;
                        if (param.refKind != ReferenceKind::Value || !param.lifetime.empty()) {
                            _isSystemsMode = true;
                        }
                    }
                    method->parameters.push_back(param);
                    if (!_check(TokenKind::RightParen)) {
                        _consume(TokenKind::Comma, "Expected ',' between method parameters");
                    }
                }
                _consume(TokenKind::RightParen, "Expected ')' closing method parameters");

                if (_match(TokenKind::Colon) || _match(TokenKind::Arrow)) {
                    method->returnType = _parseTypeName();
                }

                method->body = _parseStatementBody(&method->bodyTokens);
                classDecl->methods.push_back(method);
                continue;
            }

            // 3) Field: [let] [mut] fieldName: type [= expr];
            if (_check(TokenKind::KwLet) || _check(TokenKind::Identifier)) {
                SourceLocation fldLoc = _peek().loc;
                _match(TokenKind::KwLet); // Optional 'let'
                _match(TokenKind::KwMut); // Optional 'mut'
                auto field = std::make_shared<ClassFieldDecl>(fldLoc);
                field->visibility = memberVis;
                field->name = _consume(TokenKind::Identifier, "Expected field name").text;
                if (_match(TokenKind::Colon)) {
                    field->typeName = _parseTypeName();
                }
                if (_match(TokenKind::Assign)) {
                    std::string expr;
                    while (!_isAtEnd() && !_check(TokenKind::Semicolon)) {
                        if (!expr.empty()) expr += " ";
                        if (_peek().is(TokenKind::StringLiteral)) {
                            expr += "\"" + _advance().text + "\"";
                        } else {
                            expr += _advance().text;
                        }
                    }
                    field->defaultValueExpr = expr;
                }
                _consume(TokenKind::Semicolon, "Expected ';' after field declaration");
                classDecl->fields.push_back(field);
                continue;
            }

            if (_match(TokenKind::Semicolon)) continue;

            throw ParserException("Unexpected declaration in class '" + className + "': '" + _peek().text + "'", _peek().loc);
        }

        _consume(TokenKind::RightBrace, "Expected '}' closing class body");
        _match(TokenKind::Semicolon); // Optional trailing semicolon
        return classDecl;
    }

    std::shared_ptr<StructDecl> Parser::_parseStruct(bool isExported) {
        SourceLocation loc = _consume(TokenKind::KwStruct, "Expected 'struct'").loc;
        std::string structName = _consume(TokenKind::Identifier, "Expected struct name identifier").text;
        auto structDecl = std::make_shared<StructDecl>(loc);
        structDecl->name = structName;
        structDecl->isExported = isExported;

        _consume(TokenKind::LeftBrace, "Expected '{' to start struct body");

        while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
            Visibility memberVis = Visibility::Public;
            if (_match(TokenKind::KwPublic)) memberVis = Visibility::Public;
            else if (_match(TokenKind::KwPrivate)) memberVis = Visibility::Private;
            else if (_match(TokenKind::KwProtected)) memberVis = Visibility::Protected;

            SourceLocation fldLoc = _peek().loc;
            _match(TokenKind::KwLet); // Optional 'let'
            auto field = std::make_shared<ClassFieldDecl>(fldLoc);
            field->visibility = memberVis;
            field->name = _consume(TokenKind::Identifier, "Expected struct field name").text;
            if (_match(TokenKind::Colon)) {
                field->typeName = _parseTypeName();
            }
            if (_match(TokenKind::Assign)) {
                std::string expr;
                while (!_isAtEnd() && !_check(TokenKind::Semicolon) && !_check(TokenKind::Comma) && !_check(TokenKind::RightBrace)) {
                    if (!expr.empty()) expr += " ";
                    if (_peek().is(TokenKind::StringLiteral)) {
                        expr += "\"" + _advance().text + "\"";
                    } else {
                        expr += _advance().text;
                    }
                }
                field->defaultValueExpr = expr;
            }
            if (_match(TokenKind::Semicolon) || _match(TokenKind::Comma)) {
                // optional comma or semicolon
            }
            structDecl->fields.push_back(field);
        }

        _consume(TokenKind::RightBrace, "Expected '}' closing struct body");
        _match(TokenKind::Semicolon); // Optional trailing semicolon
        return structDecl;
    }

    std::shared_ptr<StandaloneFunctionDecl> Parser::_parseStandaloneFunction(bool isExported) {
        SourceLocation loc = _peek().loc;
        if (!_match(TokenKind::KwFn)) {
            _consume(TokenKind::KwDef, "Expected 'fn' or 'def'");
        }

        auto fn = std::make_shared<StandaloneFunctionDecl>(loc);
        fn->isExported = isExported;
        fn->name = _consume(TokenKind::Identifier, "Expected function name").text;
        if (fn->name == "main") {
            fn->isMain = true;
        }

        _consume(TokenKind::LeftParen, "Expected '(' after function name");
        while (!_isAtEnd() && !_check(TokenKind::RightParen)) {
            ParameterDecl param;
            if (_match(TokenKind::KwMove)) {
                param.isMove = true;
                _isSystemsMode = true;
            }
            param.name = _consume(TokenKind::Identifier, "Expected parameter name").text;
            if (_match(TokenKind::Colon)) {
                param.typeName = _parseTypeName();
                auto ann = TypeAnnotation::parse(param.typeName);
                param.refKind = ann.refKind;
                param.lifetime = ann.lifetime;
                if (param.refKind != ReferenceKind::Value || !param.lifetime.empty()) {
                    _isSystemsMode = true;
                }
            }
            fn->parameters.push_back(param);
            if (!_check(TokenKind::RightParen)) {
                _consume(TokenKind::Comma, "Expected ',' between parameters");
            }
        }
        _consume(TokenKind::RightParen, "Expected ')' closing parameter list");

        if (_match(TokenKind::Colon) || _match(TokenKind::Arrow)) {
            fn->returnType = _parseTypeName();
        }

        fn->body = _parseStatementBody(&fn->bodyTokens);
        return fn;
    }

    std::string Parser::_parseStatementBody(std::vector<Token>* outTokens) {
        size_t startTokenIdx = _cursor;
        _consume(TokenKind::LeftBrace, "Expected '{' to start body");
        int braceDepth = 1;
        std::ostringstream oss;
        bool lastWasNoSpaceAfter = true;
        bool lastWasDot = false;

        auto appendToken = [&](const std::string& text, bool spaceBefore = true, bool spaceAfter = true) {
            if (spaceBefore && !lastWasNoSpaceAfter && oss.tellp() > 0) {
                oss << " ";
            }
            oss << text;
            lastWasNoSpaceAfter = !spaceAfter;
        };

        while (!_isAtEnd() && braceDepth > 0) {
            bool isDotBefore = lastWasDot;
            lastWasDot = false;

            // Reject pointer arrow operator '->' in everyday KeetScript statement bodies
            if (_check(TokenKind::Arrow)) {
                throw ParserException("Pointer arrow operator ('->') is not supported in KeetScript. Use '.' dot notation for member access.", _peek().loc);
            }

            // Reject raw pointer dereference '*ptr' in everyday KeetScript
            if (_check(TokenKind::Star)) {
                bool isUnary = (_cursor <= startTokenIdx ||
                                _tokens[_cursor - 1].is(TokenKind::Assign) ||
                                _tokens[_cursor - 1].is(TokenKind::LeftParen) ||
                                _tokens[_cursor - 1].is(TokenKind::Comma) ||
                                _tokens[_cursor - 1].is(TokenKind::Semicolon) ||
                                _tokens[_cursor - 1].is(TokenKind::LeftBrace) ||
                                _tokens[_cursor - 1].is(TokenKind::KwReturn));
                if (isUnary) {
                    throw ParserException("Raw pointer dereference ('*') is not supported in KeetScript. KeetScript uses safe value semantics.", _peek().loc);
                }
            }

            // Object literal / designated initializer: { field: val, ... }
            if (_check(TokenKind::LeftBrace)) {
                size_t look = _cursor + 1;
                if (look + 1 < _tokens.size() && _tokens[look].is(TokenKind::Identifier) &&
                    _tokens[look + 1].is(TokenKind::Colon) && _tokens[look + 1].text == ":") {
                    _advance(); // consume '{'
                    appendToken("{", true, true);
                    braceDepth++;
                    while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
                        std::string fldName = _consume(TokenKind::Identifier, "Expected field name in object literal").text;
                        _consume(TokenKind::Colon, "Expected ':' after field name");
                        appendToken("." + fldName + " =", true, true);

                        int valDepth = 0;
                        while (!_isAtEnd()) {
                            if (valDepth == 0 && (_check(TokenKind::Comma) || _check(TokenKind::RightBrace) || _check(TokenKind::Semicolon))) {
                                break;
                            }
                            if (_check(TokenKind::LeftParen) || _check(TokenKind::LeftBracket) || _check(TokenKind::LeftBrace)) valDepth++;
                            else if (_check(TokenKind::RightParen) || _check(TokenKind::RightBracket) || _check(TokenKind::RightBrace)) valDepth--;

                            if (_peek().is(TokenKind::StringLiteral)) {
                                appendToken("\"" + _advance().text + "\"", true, true);
                            } else if (_check(TokenKind::LeftBracket)) {
                                _advance();
                                appendToken("{", true, true);
                                while (!_isAtEnd() && !_check(TokenKind::RightBracket)) {
                                    if (_peek().is(TokenKind::StringLiteral)) {
                                        appendToken("\"" + _advance().text + "\"", true, true);
                                    } else {
                                        appendToken(_advance().text, true, true);
                                    }
                                }
                                if (_check(TokenKind::RightBracket)) _advance();
                                appendToken("}", true, true);
                            } else {
                                appendToken(_advance().text, true, true);
                            }
                        }
                        if (_match(TokenKind::Comma) || _match(TokenKind::Semicolon)) {
                            appendToken(",", false, true);
                        }
                    }
                    _consume(TokenKind::RightBrace, "Expected '}' closing object literal");
                    appendToken("}", true, true);
                    braceDepth--;
                    continue;
                }
                braceDepth++;
                appendToken("{", true, true);
                _advance();
                continue;
            }
            if (_check(TokenKind::RightBrace)) {
                braceDepth--;
                if (braceDepth == 0) {
                    _advance(); // Consume outer closing '}'
                    break;
                }
                appendToken("}", true, true);
                _advance();
                continue;
            }

            // Arrow function: (...) => expr or (...) => { ... }
            if (_check(TokenKind::LeftParen)) {
                size_t look = _cursor + 1;
                int depth = 1;
                while (look < _tokens.size() && depth > 0) {
                    if (_tokens[look].is(TokenKind::LeftParen)) depth++;
                    else if (_tokens[look].is(TokenKind::RightParen)) depth--;
                    look++;
                }
                if (depth == 0 && look < _tokens.size() && _tokens[look].is(TokenKind::FatArrow)) {
                    _advance(); // consume '('
                    std::string params;
                    while (!_isAtEnd() && !_check(TokenKind::RightParen)) {
                        std::string pName = _consume(TokenKind::Identifier, "Expected arrow parameter name").text;
                        std::string pType = "auto";
                        if (_match(TokenKind::Colon)) {
                            pType = _parseTypeName();
                            if (pType == "string") pType = "std::string";
                            else if (pType == "array<string>" || pType == "vector<string>") pType = "std::vector<std::string>";
                            else if (pType.rfind("array<", 0) == 0) pType = "std::vector<" + pType.substr(6);
                            else if (pType.rfind("vector<", 0) == 0) pType = "std::" + pType;
                        }
                        if (!params.empty()) params += ", ";
                        params += pType + " " + pName;
                        _match(TokenKind::Comma);
                    }
                    _consume(TokenKind::RightParen, "Expected ')' after arrow parameters");
                    _consume(TokenKind::FatArrow, "Expected '=>'");

                    if (_check(TokenKind::LeftBrace)) {
                        appendToken("[&](" + params + ")", true, true);
                        continue;
                    } else {
                        std::string expr;
                        int subDepth = 0;
                        while (!_isAtEnd()) {
                            if (subDepth == 0 && (_check(TokenKind::Semicolon) || _check(TokenKind::Comma) || _check(TokenKind::RightParen) || _check(TokenKind::RightBrace))) {
                                break;
                            }
                            if (_check(TokenKind::LeftParen) || _check(TokenKind::LeftBracket) || _check(TokenKind::LeftBrace)) subDepth++;
                            else if (_check(TokenKind::RightParen) || _check(TokenKind::RightBracket) || _check(TokenKind::RightBrace)) subDepth--;
                            if (!expr.empty()) expr += " ";
                            if (_peek().is(TokenKind::StringLiteral)) {
                                expr += "\"" + _advance().text + "\"";
                            } else {
                                expr += _advance().text;
                            }
                        }
                        appendToken("[&](" + params + ") { return " + expr + "; }", true, false);
                        continue;
                    }
                }
            }

            // For loop: for [let] <var> in <collection> { ... }
            if (_check(TokenKind::KwFor)) {
                size_t look = _cursor + 1;
                bool hasParen = false;
                if (look < _tokens.size() && _tokens[look].is(TokenKind::LeftParen)) {
                    hasParen = true;
                    look++;
                }
                if (look < _tokens.size() && _tokens[look].is(TokenKind::KwLet)) {
                    look++;
                }
                if (look + 1 < _tokens.size() && _tokens[look].is(TokenKind::Identifier) &&
                    (_tokens[look + 1].is(TokenKind::KwIn) || (_tokens[look + 1].is(TokenKind::Identifier) && _tokens[look + 1].text == "in"))) {
                    _advance(); // consume 'for'
                    if (hasParen) _advance(); // consume '('
                    if (_check(TokenKind::KwLet)) _advance(); // optional 'let'
                    std::string itemVar = _consume(TokenKind::Identifier, "Expected loop variable name").text;
                    _advance(); // consume 'in'
                    std::string collExpr;
                    int parenDepth = hasParen ? 1 : 0;
                    while (!_isAtEnd()) {
                        if (hasParen && _check(TokenKind::RightParen) && parenDepth == 1) {
                            _advance(); // consume closing ')'
                            break;
                        }
                        if (!hasParen && _check(TokenKind::LeftBrace)) {
                            break;
                        }
                        if (_check(TokenKind::LeftParen)) parenDepth++;
                        else if (_check(TokenKind::RightParen)) parenDepth--;
                        if (!collExpr.empty()) collExpr += " ";
                        collExpr += _advance().text;
                    }
                    appendToken("for (auto& " + itemVar + " : " + collExpr + ")", true, true);
                    continue;
                }
            }

            // Conditionals: if [cond] { and else if [cond] {
            if (_check(TokenKind::KwIf) && !_peek(1).is(TokenKind::LeftParen)) {
                _advance(); // consume 'if'
                appendToken("if (", true, false);
                int exprDepth = 0;
                while (!_isAtEnd()) {
                    if (_check(TokenKind::LeftBrace) && exprDepth == 0) {
                        break;
                    }
                    if (_check(TokenKind::LeftParen)) exprDepth++;
                    else if (_check(TokenKind::RightParen)) exprDepth--;
                    appendToken(_advance().text, true, true);
                }
                appendToken(")", false, true);
                continue;
            }
            if (_check(TokenKind::KwElse)) {
                if (_peek(1).is(TokenKind::KwIf)) {
                    _advance(); // consume 'else'
                    _advance(); // consume 'if'
                    if (!_peek().is(TokenKind::LeftParen)) {
                        appendToken("else if (", true, false);
                        int exprDepth = 0;
                        while (!_isAtEnd()) {
                            if (_check(TokenKind::LeftBrace) && exprDepth == 0) {
                                break;
                            }
                            if (_check(TokenKind::LeftParen)) exprDepth++;
                            else if (_check(TokenKind::RightParen)) exprDepth--;
                            appendToken(_advance().text, true, true);
                        }
                        appendToken(")", false, true);
                        continue;
                    } else {
                        appendToken("else if", true, true);
                        continue;
                    }
                }
            }

            // C++ native escape: cpp.<ident> -> ::<ident>
            if ((_check(TokenKind::KwCpp) || (_check(TokenKind::Identifier) && _peek().text == "cpp")) && _peek(1).is(TokenKind::Dot)) {
                _advance(); // consume 'cpp'
                _advance(); // consume '.'
                appendToken("::", true, false);
                continue;
            }

            // Member access: this.<ident> -> this-><ident>
            if (_check(TokenKind::KwThis)) {
                _advance();
                if (_match(TokenKind::Dot)) {
                    appendToken("this->", true, false);
                } else {
                    appendToken("this", true, true);
                }
                continue;
            }

            // Constant declaration: const <ident> [: <type>] [= <expr>];
            if (_check(TokenKind::KwConst)) {
                _advance(); // Consume 'const'
                std::string varName = _consume(TokenKind::Identifier, "Expected constant name after 'const'").text;
                std::string varType = "auto";
                if (_match(TokenKind::Colon)) {
                    varType = _parseTypeName();
                }
                auto ann = TypeAnnotation::parse(varType);
                std::string base = ann.baseType;
                if (base == "string") base = "std::string";
                else if (base == "io::Buffer" || base == "io.Buffer" || base == "Buffer") base = "Buffer";
                else if (base == "file::File" || base == "file.File" || base == "File") base = "File";
                else if (base == "vector<string>" || base == "array<string>") base = "std::vector<std::string>";
                else if (base.rfind("vector<", 0) == 0) base = "std::" + base;
                else if (base.rfind("array<", 0) == 0) base = "std::vector<" + base.substr(6);

                if (ann.isReference()) {
                    appendToken("const " + base + "& " + varName, true, true);
                } else if (varType == "auto" && _check(TokenKind::Assign) && _cursor + 1 < _tokens.size() && _tokens[_cursor + 1].is(TokenKind::Ampersand)) {
                    appendToken("const auto& " + varName, true, true);
                } else {
                    appendToken("const " + base + " " + varName, true, true);
                }
                continue;
            }

            // Variable declaration: let [mut] <ident> [: <type>] [= <expr>];
            if (_check(TokenKind::KwLet)) {
                _advance(); // Consume 'let'
                bool isMut = false;
                if (_match(TokenKind::KwMut)) {
                    isMut = true;
                }
                std::string varName = _consume(TokenKind::Identifier, "Expected variable name after 'let'").text;
                std::string varType = "auto";
                if (_match(TokenKind::Colon)) {
                    varType = _parseTypeName();
                }
                auto ann = TypeAnnotation::parse(varType);
                std::string base = ann.baseType;
                if (base == "string") base = "std::string";
                else if (base == "io::Buffer" || base == "io.Buffer" || base == "Buffer") base = "Buffer";
                else if (base == "file::File" || base == "file.File" || base == "File") base = "File";
                else if (base == "vector<string>" || base == "array<string>") base = "std::vector<std::string>";
                else if (base.rfind("vector<", 0) == 0) base = "std::" + base;
                else if (base.rfind("array<", 0) == 0) base = "std::vector<" + base.substr(6);

                if (ann.isMutableRef()) {
                    appendToken(base + "& " + varName, true, true);
                } else if (ann.isImmutableRef()) {
                    appendToken("const " + base + "& " + varName, true, true);
                } else if (varType == "auto" && _check(TokenKind::Assign) && _cursor + 1 < _tokens.size()) {
                    if (_tokens[_cursor + 1].is(TokenKind::Ampersand)) {
                        if (_cursor + 2 < _tokens.size() && _tokens[_cursor + 2].is(TokenKind::KwMut)) {
                            appendToken("auto& " + varName, true, true);
                        } else {
                            appendToken("const auto& " + varName, true, true);
                        }
                    } else {
                        appendToken("auto " + varName, true, true);
                    }
                } else {
                    appendToken(base + " " + varName, true, true);
                }
                continue;
            }

            // Ownership transfer: move <ident_or_expr> -> std::move(<expr>)
            if (_check(TokenKind::KwMove)) {
                _advance(); // consume 'move'
                _isSystemsMode = true;
                appendToken("std::move(", true, false);
                int mDepth = 0;
                bool firstTok = true;
                while (!_isAtEnd()) {
                    if (mDepth == 0 && (_check(TokenKind::Semicolon) || _check(TokenKind::Comma) || _check(TokenKind::RightParen) || _check(TokenKind::RightBracket) || _check(TokenKind::RightBrace))) {
                        break;
                    }
                    if (_check(TokenKind::LeftParen) || _check(TokenKind::LeftBracket) || _check(TokenKind::LeftBrace)) mDepth++;
                    else if (_check(TokenKind::RightParen) || _check(TokenKind::RightBracket) || _check(TokenKind::RightBrace)) mDepth--;
                    Token mTok = _advance();
                    if (mTok.is(TokenKind::Dot)) {
                        appendToken(mTok.text, false, false);
                    } else if (mTok.is(TokenKind::LeftParen) || mTok.is(TokenKind::LeftBracket)) {
                        appendToken(mTok.text, false, false);
                    } else if (mTok.is(TokenKind::RightParen) || mTok.is(TokenKind::RightBracket)) {
                        appendToken(mTok.text, false, false);
                    } else {
                        appendToken(mTok.text, !firstTok, false);
                    }
                    firstTok = false;
                }
                appendToken(")", false, true);
                continue;
            }

            // Borrow operators: &mut <expr> or &<expr>
            if (_check(TokenKind::Ampersand)) {
                _advance(); // consume '&'
                if (_match(TokenKind::KwMut)) {
                    // &mut <expr> in C++ call context is the lvalue reference itself
                    _isSystemsMode = true;
                    continue;
                } else {
                    bool isPrefix = (_cursor <= 1 || _tokens[_cursor - 2].is(TokenKind::LeftParen) ||
                                     _tokens[_cursor - 2].is(TokenKind::Comma) ||
                                     _tokens[_cursor - 2].is(TokenKind::Assign) ||
                                     _tokens[_cursor - 2].is(TokenKind::Colon));
                    if (isPrefix) {
                        // Unary immutable borrow &x in C++ is the lvalue itself
                        _isSystemsMode = true;
                        continue;
                    } else {
                        // Binary bitwise &
                        appendToken("&", true, true);
                        continue;
                    }
                }
            }

            // Stream print: print(...) -> keet::io::print(...) (only root print, not obj.print)
            if ((_check(TokenKind::KwPrint) || (_check(TokenKind::Identifier) && _peek().text == "print")) && !isDotBefore) {
                _advance();
                appendToken("keet::io::print", true, false);
                continue;
            }

            // Array literal: ["Alice", "Bob"] -> {"Alice", "Bob"} when in assignment or value position (not subscript indexing)
            bool isSubscript = false;
            if (_cursor > 0) {
                const auto& prev = _tokens[_cursor - 1];
                if (prev.is(TokenKind::Identifier) || prev.is(TokenKind::RightParen) || prev.is(TokenKind::RightBracket)) {
                    isSubscript = true;
                }
            }

            if (_check(TokenKind::LeftBracket) && !isDotBefore && !isSubscript) {
                _advance();
                appendToken("{", true, true);
                while (!_isAtEnd() && !_check(TokenKind::RightBracket)) {
                    if (_peek().is(TokenKind::StringLiteral)) {
                        appendToken("\"" + _advance().text + "\"", true, true);
                    } else if (_check(TokenKind::Comma)) {
                        appendToken(",", false, true);
                        _advance();
                    } else {
                        appendToken(_advance().text, true, true);
                    }
                }
                if (_check(TokenKind::RightBracket)) _advance();
                appendToken("}", true, true);
                continue;
            }

            // Ternary operator: ?
            if (_check(TokenKind::Question)) {
                appendToken("?", true, true);
                _advance();
                continue;
            }

            // Punctuation formatting
            if (_check(TokenKind::Semicolon)) {
                appendToken(";", false, true);
                _advance();
                continue;
            }
            if (_check(TokenKind::Comma)) {
                appendToken(",", false, true);
                _advance();
                continue;
            }
            if (_check(TokenKind::LeftParen)) {
                appendToken("(", false, false);
                _advance();
                continue;
            }
            if (_check(TokenKind::RightParen)) {
                appendToken(")", false, true);
                _advance();
                continue;
            }
            if (_check(TokenKind::LeftBracket)) {
                appendToken("[", false, false);
                _advance();
                continue;
            }
            if (_check(TokenKind::RightBracket)) {
                appendToken("]", false, true);
                _advance();
                continue;
            }
            if (_check(TokenKind::Dot)) {
                appendToken(".", false, false);
                _advance();
                lastWasDot = true;
                continue;
            }
            if (_check(TokenKind::Colon) && _peek().text == "::") {
                appendToken("::", false, false);
                _advance();
                continue;
            }

            if (_peek().is(TokenKind::StringLiteral)) {
                appendToken("\"" + _advance().text + "\"", true, true);
            } else {
                appendToken(_advance().text, true, true);
            }
        }

        if (outTokens && startTokenIdx < _tokens.size() && _cursor <= _tokens.size()) {
            outTokens->assign(_tokens.begin() + startTokenIdx, _tokens.begin() + _cursor);
        }

        return transformZonk(oss.str());
    }

    std::shared_ptr<ErrtypeDecl> Parser::_parseErrtype(bool isExported) {
        SourceLocation loc = (_check(TokenKind::KwErrtype) || _check(TokenKind::Identifier)) ? _advance().loc : _peek().loc;
        std::string name = _consume(TokenKind::Identifier, "Expected errtype name identifier").text;
        _consume(TokenKind::LeftBrace, "Expected '{' after errtype name");
        auto errtype = std::make_shared<ErrtypeDecl>(loc);
        errtype->name = name;
        errtype->isExported = isExported;

        while (!_isAtEnd() && !_check(TokenKind::RightBrace)) {
            std::string entryName = _consume(TokenKind::Identifier, "Expected error entry identifier").text;
            int code = 0;
            bool hasCode = false;
            if (_match(TokenKind::Assign)) {
                std::string numStr = _consume(TokenKind::NumberLiteral, "Expected numeric error code after '='").text;
                code = std::stoi(numStr);
                hasCode = true;
            }
            errtype->fields.push_back(ErrtypeField{.name = entryName, .code = code, .hasCode = hasCode});
            if (_match(TokenKind::Comma) || _match(TokenKind::Semicolon)) {
                // optional delimiter
            }
        }
        _consume(TokenKind::RightBrace, "Expected '}' closing errtype block");
        return errtype;
    }

    std::string Parser::transformZonk(std::string s) {
        size_t pos = 0;
        while (true) {
            pos = s.find("zonk", pos);
            if (pos == std::string::npos) break;

            bool validBefore = (pos == 0) || (!std::isalnum(static_cast<unsigned char>(s[pos - 1])) && s[pos - 1] != '_');
            bool validAfter = (pos + 4 >= s.size()) || (!std::isalnum(static_cast<unsigned char>(s[pos + 4])) && s[pos + 4] != '_');

            if (!validBefore || !validAfter) {
                pos += 4;
                continue;
            }

            bool insideStr = false;
            for (size_t k = 0; k < pos; ++k) {
                if (s[k] == '"' && (k == 0 || s[k - 1] != '\\')) {
                    insideStr = !insideStr;
                }
            }
            if (insideStr) {
                pos += 4;
                continue;
            }

            int64_t i = static_cast<int64_t>(pos) - 1;
            while (i >= 0 && std::isspace(static_cast<unsigned char>(s[i]))) i--;

            int pParen = 0, pBracket = 0, pBrace = 0;
            bool inQuote = false;
            int64_t lhsEnd = i + 1;
            int64_t lhsStart = 0;

            for (; i >= 0; --i) {
                char c = s[i];
                if (c == '"' && (i == 0 || s[i - 1] != '\\')) {
                    inQuote = !inQuote;
                    continue;
                }
                if (inQuote) continue;

                if (c == ')') pParen++;
                else if (c == '(') {
                    if (pParen == 0) { lhsStart = i + 1; break; }
                    pParen--;
                }
                else if (c == ']') pBracket++;
                else if (c == '[') {
                    if (pBracket == 0) { lhsStart = i + 1; break; }
                    pBracket--;
                }
                else if (c == '}') pBrace++;
                else if (c == '{') {
                    if (pBrace == 0) { lhsStart = i + 1; break; }
                    pBrace--;
                }
                else if (pParen == 0 && pBracket == 0 && pBrace == 0) {
                    if (c == '=' || c == ';' || c == ',' || c == ':') {
                        lhsStart = i + 1;
                        break;
                    }
                    if (i >= 6 && s.substr(i - 6, 7) == "return ") {
                        lhsStart = i + 1;
                        break;
                    }
                }
            }
            if (i < 0) lhsStart = 0;

            while (lhsStart < lhsEnd && std::isspace(static_cast<unsigned char>(s[lhsStart]))) lhsStart++;
            while (lhsEnd > lhsStart && std::isspace(static_cast<unsigned char>(s[lhsEnd - 1]))) lhsEnd--;

            std::string lhs = s.substr(lhsStart, lhsEnd - lhsStart);

            size_t j = pos + 4;
            while (j < s.size() && std::isspace(static_cast<unsigned char>(s[j]))) j++;
            size_t rhsStart = j;

            pParen = 0; pBracket = 0; pBrace = 0; inQuote = false;
            size_t rhsEnd = s.size();

            for (; j < s.size(); ++j) {
                char c = s[j];
                if (c == '"' && (j == 0 || s[j - 1] != '\\')) {
                    inQuote = !inQuote;
                    continue;
                }
                if (inQuote) continue;

                if (c == '(') pParen++;
                else if (c == ')') {
                    if (pParen == 0) { rhsEnd = j; break; }
                    pParen--;
                }
                else if (c == '[') pBracket++;
                else if (c == ']') {
                    if (pBracket == 0) { rhsEnd = j; break; }
                    pBracket--;
                }
                else if (c == '{') pBrace++;
                else if (c == '}') {
                    if (pBrace == 0) { rhsEnd = j; break; }
                    pBrace--;
                }
                else if (pParen == 0 && pBracket == 0 && pBrace == 0) {
                    if (c == ';' || c == ',') {
                        rhsEnd = j;
                        break;
                    }
                }
            }
            while (rhsEnd > rhsStart && std::isspace(static_cast<unsigned char>(s[rhsEnd - 1]))) rhsEnd--;
            std::string rhs = s.substr(rhsStart, rhsEnd - rhsStart);

            std::string replacement = "KEET_ZONK(" + lhs + ", " + rhs + ")";
            s.replace(lhsStart, rhsEnd - lhsStart, replacement);
            pos = lhsStart + replacement.size();
        }
        return s;
    }

} // namespace Keety
