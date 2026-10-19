//
// Created by PAWEL on 19/10/2026.
//

#include "Parser.h"
#include <sstream>

namespace Keety {

    std::unique_ptr<SourceFile> Parser::parse() {
        auto file = std::make_unique<SourceFile>(_componentName, _peek().loc);

        while (!_isAtEnd()) {
            if (_check(TokenKind::TagScriptOpen)) {
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
