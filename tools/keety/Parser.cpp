//
// Created by PAWEL on 19/10/2026.
//

#include "Parser.h"
#include <sstream>

namespace Keety {

    std::unique_ptr<SourceFile> Parser::parse() {
        auto file = std::make_unique<SourceFile>(_componentName, _peek().loc);

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
                script->imports.push_back(_parseImportCpp());
            } else if (_check(TokenKind::KwState)) {
                script->stateDecls.push_back(_parseStateDecl());
            } else if (_check(TokenKind::KwLet)) {
                script->letDecls.push_back(_parseLetDecl());
            } else if (_check(TokenKind::KwDef)) {
                script->functions.push_back(_parseFunctionDecl());
            } else {
                throw ParserException("Unexpected declaration in <script>: '" + _peek().text + "'", _peek().loc);
            }
        }

        _consume(TokenKind::TagScriptClose, "Expected '</script>' to close script section");
        return script;
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

    std::shared_ptr<FunctionDecl> Parser::_parseFunctionDecl() {
        SourceLocation loc = _consume(TokenKind::KwDef, "Expected 'def'").loc;
        auto fn = std::make_shared<FunctionDecl>(loc);
        fn->name = _consume(TokenKind::Identifier, "Expected function name").text;
        _consume(TokenKind::LeftParen, "Expected '(' after function name");

        while (!_isAtEnd() && !_check(TokenKind::RightParen)) {
            ParameterDecl param;
            param.name = _consume(TokenKind::Identifier, "Expected parameter name").text;
            if (_match(TokenKind::Colon)) {
                param.typeName = _consume(TokenKind::Identifier, "Expected parameter type annotation").text;
            }
            fn->parameters.push_back(param);
            if (!_check(TokenKind::RightParen)) {
                _consume(TokenKind::Comma, "Expected ',' between parameters");
            }
        }
        _consume(TokenKind::RightParen, "Expected ')' closing parameter list");

        if (_match(TokenKind::Arrow)) {
            fn->returnType = _consume(TokenKind::Identifier, "Expected return type").text;
        }

        _consume(TokenKind::LeftBrace, "Expected '{' before function body");
        int braceDepth = 1;
        std::string body;
        while (!_isAtEnd() && braceDepth > 0) {
            if (_check(TokenKind::LeftBrace)) braceDepth++;
            else if (_check(TokenKind::RightBrace)) {
                braceDepth--;
                if (braceDepth == 0) break;
            }
            if (!body.empty()) body += " ";
            if (_peek().is(TokenKind::StringLiteral)) {
                body += "\"" + _advance().text + "\"";
            } else {
                body += _advance().text;
            }
        }
        _consume(TokenKind::RightBrace, "Expected '}' closing function body");
        fn->body = body;
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
            return std::make_shared<InterpolationNode>(expr, loc);
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
        std::string tagName = _consume(TokenKind::Identifier, "Expected HTML tag name").text;
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
        std::string closeTag = _consume(TokenKind::Identifier, "Expected closing tag name").text;
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

        attr.name = _consume(TokenKind::Identifier, "Expected attribute name").text;

        if (_match(TokenKind::Assign)) {
            if (_check(TokenKind::StringLiteral)) {
                attr.kind = AttributeKind::StaticLiteral;
                attr.rawValue = _advance().text;
            } else if (_match(TokenKind::LeftBrace)) {
                std::string expr = _consume(TokenKind::RawText, "Expected expression inside attribute value").text;
                _consume(TokenKind::RightBrace, "Expected '}' closing attribute expression");
                attr.rawValue = expr;

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

} // namespace Keety
