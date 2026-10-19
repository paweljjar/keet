//
// Created by PAWEL on 19/10/2026.
//

#include "Lexer.h"

namespace Keety {

    void Lexer::_skipWhitespaceAndComments() {
        while (!_isAtEnd()) {
            char c = _peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _advance();
            } else if (c == '/' && _peek(1) == '/') { // Single line comment
                _advance(); _advance();
                while (!_isAtEnd() && _peek() != '\n') {
                    _advance();
                }
            } else if (c == '/' && _peek(1) == '*') { // Block comment
                _advance(); _advance();
                while (!_isAtEnd()) {
                    if (_peek() == '*' && _peek(1) == '/') {
                        _advance(); _advance();
                        break;
                    }
                    _advance();
                }
            } else if (c == '<' && _peek(1) == '!' && _peek(2) == '-' && _peek(3) == '-') { // HTML comment <!-- ... -->
                _advance(); _advance(); _advance(); _advance();
                while (!_isAtEnd()) {
                    if (_peek() == '-' && _peek(1) == '-' && _peek(2) == '>') {
                        _advance(); _advance(); _advance();
                        break;
                    }
                    _advance();
                }
            } else {
                break;
            }
        }
    }

    Token Lexer::_lexIdentifierOrKeyword() {
        SourceLocation loc = _currentLoc();
        size_t start = _cursor;
        while (!_isAtEnd()) {
            char c = _peek();
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') {
                _advance();
            } else {
                break;
            }
        }
        std::string text = _source.substr(start, _cursor - start);

        // Check reserved keywords
        if (text == "import") return Token(TokenKind::KwImport, text, loc);
        if (text == "cpp") return Token(TokenKind::KwCpp, text, loc);
        if (text == "from") return Token(TokenKind::KwFrom, text, loc);
        if (text == "as") return Token(TokenKind::KwAs, text, loc);
        if (text == "state") return Token(TokenKind::KwState, text, loc);
        if (text == "let") return Token(TokenKind::KwLet, text, loc);
        if (text == "def") return Token(TokenKind::KwDef, text, loc);
        if (text == "return") return Token(TokenKind::KwReturn, text, loc);
        if (text == "if") return Token(TokenKind::KwIf, text, loc);
        if (text == "elif") return Token(TokenKind::KwElif, text, loc);
        if (text == "else") return Token(TokenKind::KwElse, text, loc);
        if (text == "for") return Token(TokenKind::KwFor, text, loc);
        if (text == "in") return Token(TokenKind::KwIn, text, loc);

        return Token(TokenKind::Identifier, text, loc);
    }

    Token Lexer::_lexNumber() {
        SourceLocation loc = _currentLoc();
        size_t start = _cursor;
        bool hasDot = false;
        while (!_isAtEnd()) {
            char c = _peek();
            if (std::isdigit(static_cast<unsigned char>(c))) {
                _advance();
            } else if (c == '.' && !hasDot && std::isdigit(static_cast<unsigned char>(_peek(1)))) {
                hasDot = true;
                _advance();
            } else {
                break;
            }
        }
        return Token(TokenKind::NumberLiteral, _source.substr(start, _cursor - start), loc);
    }

    Token Lexer::_lexString(char quote) {
        SourceLocation loc = _currentLoc();
        _advance(); // Consume opening quote
        size_t start = _cursor;
        while (!_isAtEnd() && _peek() != quote) {
            if (_peek() == '\\' && _peek(1) != '\0') {
                _advance(); // Skip escape slash
            }
            _advance();
        }
        if (_isAtEnd()) {
            throw LexerException("Unterminated string literal", loc);
        }
        std::string str = _source.substr(start, _cursor - start);
        _advance(); // Consume closing quote
        return Token(TokenKind::StringLiteral, str, loc);
    }

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;

        while (!_isAtEnd()) {
            if (_mode == LexerMode::Root) {
                _skipWhitespaceAndComments();
                if (_isAtEnd()) break;

                SourceLocation loc = _currentLoc();
                if (_matchString("<script>")) {
                    tokens.emplace_back(TokenKind::TagScriptOpen, "<script>", loc);
                    _mode = LexerMode::Script;
                } else if (_matchString("<template>")) {
                    tokens.emplace_back(TokenKind::TagTemplateOpen, "<template>", loc);
                    _mode = LexerMode::Template;
                } else if (_matchString("<style>")) {
                    tokens.emplace_back(TokenKind::TagStyleOpen, "<style>", loc);
                    _mode = LexerMode::Style;
                } else {
                    throw LexerException(std::string("Unexpected token outside SFC sections: '") + _peek() + "'", loc);
                }
            } else if (_mode == LexerMode::Script) {
                _skipWhitespaceAndComments();
                if (_isAtEnd()) break;

                SourceLocation loc = _currentLoc();
                if (_matchString("</script>")) {
                    tokens.emplace_back(TokenKind::TagScriptClose, "</script>", loc);
                    _mode = LexerMode::Root;
                    continue;
                }

                char c = _peek();
                if (c == '"' || c == '\'') {
                    tokens.push_back(_lexString(c));
                } else if (std::isdigit(static_cast<unsigned char>(c))) {
                    tokens.push_back(_lexNumber());
                } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                    tokens.push_back(_lexIdentifierOrKeyword());
                } else if (c == '-' && _peek(1) == '>') {
                    _advance(); _advance();
                    tokens.emplace_back(TokenKind::Arrow, "->", loc);
                } else if (c == '=' && _peek(1) == '>') {
                    _advance(); _advance();
                    tokens.emplace_back(TokenKind::FatArrow, "=>", loc);
                } else if (c == '+' && _peek(1) == '=') {
                    _advance(); _advance();
                    tokens.emplace_back(TokenKind::PlusAssign, "+=", loc);
                } else if (c == '-' && _peek(1) == '=') {
                    _advance(); _advance();
                    tokens.emplace_back(TokenKind::MinusAssign, "-=", loc);
                } else if (c == '{') {
                    _advance();
                    tokens.emplace_back(TokenKind::LeftBrace, "{", loc);
                } else if (c == '}') {
                    _advance();
                    tokens.emplace_back(TokenKind::RightBrace, "}", loc);
                } else if (c == '(') {
                    _advance();
                    tokens.emplace_back(TokenKind::LeftParen, "(", loc);
                } else if (c == ')') {
                    _advance();
                    tokens.emplace_back(TokenKind::RightParen, ")", loc);
                } else if (c == '[') {
                    _advance();
                    tokens.emplace_back(TokenKind::LeftBracket, "[", loc);
                } else if (c == ']') {
                    _advance();
                    tokens.emplace_back(TokenKind::RightBracket, "]", loc);
                } else if (c == ':') {
                    _advance();
                    tokens.emplace_back(TokenKind::Colon, ":", loc);
                } else if (c == ';') {
                    _advance();
                    tokens.emplace_back(TokenKind::Semicolon, ";", loc);
                } else if (c == ',') {
                    _advance();
                    tokens.emplace_back(TokenKind::Comma, ",", loc);
                } else if (c == '.') {
                    _advance();
                    tokens.emplace_back(TokenKind::Dot, ".", loc);
                } else if (c == '=') {
                    _advance();
                    tokens.emplace_back(TokenKind::Assign, "=", loc);
                } else {
                    // Single char operator or unrecognized
                    std::string sym(1, _advance());
                    tokens.emplace_back(TokenKind::Invalid, sym, loc);
                }
            } else if (_mode == LexerMode::Template) {
                // Inside <template>, we can see:
                // 1) </template>
                // 2) HTML open/close tags: <tag, </tag>
                // 3) Control flow keywords at statement start: if, elif, else, for
                // 4) Interpolation: {expr}
                // 5) Text nodes
                SourceLocation loc = _currentLoc();

                if (_matchString("</template>")) {
                    tokens.emplace_back(TokenKind::TagTemplateClose, "</template>", loc);
                    _mode = LexerMode::Root;
                    continue;
                }

                // Check HTML comments
                if (_peek() == '<' && _peek(1) == '!' && _peek(2) == '-' && _peek(3) == '-') {
                    _skipWhitespaceAndComments();
                    continue;
                }

                // Check closing element tag </tag>
                if (_peek() == '<' && _peek(1) == '/') {
                    _advance(); _advance();
                    tokens.emplace_back(TokenKind::LessThanSlash, "</", loc);
                    _mode = LexerMode::TemplateTag;
                    continue;
                }

                // Check opening element tag <tag
                if (_peek() == '<' && (std::isalpha(static_cast<unsigned char>(_peek(1))) || _peek(1) == '_')) {
                    _advance();
                    tokens.emplace_back(TokenKind::LessThan, "<", loc);
                    _mode = LexerMode::TemplateTag;
                    continue;
                }

                // Check interpolation {expr}
                if (_peek() == '{') {
                    _advance();
                    tokens.emplace_back(TokenKind::LeftBrace, "{", loc);
                    // Read expression until matching '}'
                    size_t exprStart = _cursor;
                    int braceDepth = 1;
                    SourceLocation exprLoc = _currentLoc();
                    while (!_isAtEnd() && braceDepth > 0) {
                        char ec = _peek();
                        if (ec == '{') braceDepth++;
                        else if (ec == '}') {
                            braceDepth--;
                            if (braceDepth == 0) break;
                        }
                        _advance();
                    }
                    if (_isAtEnd() && braceDepth > 0) {
                        throw LexerException("Unclosed expression interpolation '{'", exprLoc);
                    }
                    std::string expr = _source.substr(exprStart, _cursor - exprStart);
                    tokens.emplace_back(TokenKind::RawText, expr, exprLoc);
                    SourceLocation closeBraceLoc = _currentLoc();
                    _advance(); // Consume '}'
                    tokens.emplace_back(TokenKind::RightBrace, "}", closeBraceLoc);
                    continue;
                }

                // Skip formatting whitespace between tags and control flow
                if (std::isspace(static_cast<unsigned char>(_peek()))) {
                    size_t wsStart = _cursor;
                    bool hasNewline = false;
                    while (!_isAtEnd() && std::isspace(static_cast<unsigned char>(_peek()))) {
                        if (_peek() == '\n') hasNewline = true;
                        _advance();
                    }
                    if (_isAtEnd()) continue;

                    // If followed by tag, interpolation, control flow keyword, or tag close
                    char nextC = _peek();
                    if (nextC == '<' || nextC == '{' ||
                        _source.compare(_cursor, 3, "if ") == 0 ||
                        _source.compare(_cursor, 5, "elif ") == 0 ||
                        _source.compare(_cursor, 5, "else:") == 0 ||
                        _source.compare(_cursor, 5, "else ") == 0 ||
                        _source.compare(_cursor, 4, "for ") == 0) {
                        continue; // Discard formatting whitespace
                    }

                    // If it was inline whitespace inside text, rewind to preserve single space
                    if (!hasNewline) {
                        tokens.emplace_back(TokenKind::RawText, " ", loc);
                    }
                    continue;
                }

                // Check Pythonic control flow: if, elif, else, for
                if (_peek() == 'i' && _peek(1) == 'f' && (std::isspace(static_cast<unsigned char>(_peek(2))) || _peek(2) == '(')) {
                    _advance(); _advance();
                    tokens.emplace_back(TokenKind::KwIf, "if", loc);
                    // Read condition until ':'
                    _skipWhitespaceAndComments();
                    size_t condStart = _cursor;
                    while (!_isAtEnd() && _peek() != ':') {
                        _advance();
                    }
                    std::string cond = _source.substr(condStart, _cursor - condStart);
                    while (!cond.empty() && std::isspace(static_cast<unsigned char>(cond.back()))) cond.pop_back();
                    tokens.emplace_back(TokenKind::RawText, cond, _currentLoc());
                    if (_peek() == ':') {
                        SourceLocation cLoc = _currentLoc();
                        _advance();
                        tokens.emplace_back(TokenKind::Colon, ":", cLoc);
                    }
                    continue;
                }
                if (_peek() == 'e' && _peek(1) == 'l' && _peek(2) == 'i' && _peek(3) == 'f' && (std::isspace(static_cast<unsigned char>(_peek(4))) || _peek(4) == '(')) {
                    _advance(); _advance(); _advance(); _advance();
                    tokens.emplace_back(TokenKind::KwElif, "elif", loc);
                    _skipWhitespaceAndComments();
                    size_t condStart = _cursor;
                    while (!_isAtEnd() && _peek() != ':') {
                        _advance();
                    }
                    std::string cond = _source.substr(condStart, _cursor - condStart);
                    while (!cond.empty() && std::isspace(static_cast<unsigned char>(cond.back()))) cond.pop_back();
                    tokens.emplace_back(TokenKind::RawText, cond, _currentLoc());
                    if (_peek() == ':') {
                        SourceLocation cLoc = _currentLoc();
                        _advance();
                        tokens.emplace_back(TokenKind::Colon, ":", cLoc);
                    }
                    continue;
                }
                if (_peek() == 'e' && _peek(1) == 'l' && _peek(2) == 's' && _peek(3) == 'e' && (_peek(4) == ':' || std::isspace(static_cast<unsigned char>(_peek(4))))) {
                    _advance(); _advance(); _advance(); _advance();
                    tokens.emplace_back(TokenKind::KwElse, "else", loc);
                    _skipWhitespaceAndComments();
                    if (_peek() == ':') {
                        SourceLocation cLoc = _currentLoc();
                        _advance();
                        tokens.emplace_back(TokenKind::Colon, ":", cLoc);
                    }
                    continue;
                }
                if (_peek() == 'f' && _peek(1) == 'o' && _peek(2) == 'r' && std::isspace(static_cast<unsigned char>(_peek(3)))) {
                    _advance(); _advance(); _advance();
                    tokens.emplace_back(TokenKind::KwFor, "for", loc);
                    continue;
                }
                if (_peek() == 'i' && _peek(1) == 'n' && std::isspace(static_cast<unsigned char>(_peek(2)))) {
                    _advance(); _advance();
                    tokens.emplace_back(TokenKind::KwIn, "in", loc);
                    continue;
                }
                // Otherwise, read raw text until next '<', '{', or newline before potential control flow
                size_t textStart = _cursor;
                while (!_isAtEnd() && _peek() != '<' && _peek() != '{' && _peek() != '\n') {
                    _advance();
                }

                std::string raw = _source.substr(textStart, _cursor - textStart);
                if (!raw.empty()) {
                    tokens.emplace_back(TokenKind::RawText, raw, loc);
                }
            } else if (_mode == LexerMode::TemplateTag) {
                _skipWhitespaceAndComments();
                if (_isAtEnd()) break;

                SourceLocation loc = _currentLoc();

                // Self closing tag end: />
                if (_peek() == '/' && _peek(1) == '>') {
                    _advance(); _advance();
                    tokens.emplace_back(TokenKind::SlashGreaterThan, "/>", loc);
                    _mode = LexerMode::Template;
                    continue;
                }

                // Standard tag end: >
                if (_peek() == '>') {
                    _advance();
                    tokens.emplace_back(TokenKind::GreaterThan, ">", loc);
                    _mode = LexerMode::Template;
                    continue;
                }

                // Two-way binding directive: bind:value or bind:checked
                if (_matchString("bind:")) {
                    tokens.emplace_back(TokenKind::DirectiveBind, "bind:", loc);
                    continue;
                }

                // Attribute assignment: =
                if (_peek() == '=') {
                    _advance();
                    tokens.emplace_back(TokenKind::Assign, "=", loc);
                    continue;
                }

                // Quoted attribute value: "..."
                if (_peek() == '"' || _peek() == '\'') {
                    tokens.push_back(_lexString(_peek()));
                    continue;
                }

                // Dynamic attribute value: {expr}
                if (_peek() == '{') {
                    _advance();
                    tokens.emplace_back(TokenKind::LeftBrace, "{", loc);
                    size_t exprStart = _cursor;
                    int braceDepth = 1;
                    SourceLocation exprLoc = _currentLoc();
                    while (!_isAtEnd() && braceDepth > 0) {
                        char ec = _peek();
                        if (ec == '{') braceDepth++;
                        else if (ec == '}') {
                            braceDepth--;
                            if (braceDepth == 0) break;
                        }
                        _advance();
                    }
                    if (_isAtEnd() && braceDepth > 0) {
                        throw LexerException("Unclosed expression inside attribute '{'", exprLoc);
                    }
                    std::string expr = _source.substr(exprStart, _cursor - exprStart);
                    tokens.emplace_back(TokenKind::RawText, expr, exprLoc);
                    SourceLocation closeBraceLoc = _currentLoc();
                    _advance(); // Consume '}'
                    tokens.emplace_back(TokenKind::RightBrace, "}", closeBraceLoc);
                    continue;
                }

                // Identifier (tag name or attribute name)
                if (std::isalpha(static_cast<unsigned char>(_peek())) || _peek() == '_') {
                    tokens.push_back(_lexIdentifierOrKeyword());
                    continue;
                }

                // Unknown tag token
                std::string sym(1, _advance());
                tokens.emplace_back(TokenKind::Invalid, sym, loc);
            } else if (_mode == LexerMode::Style) {
                _skipWhitespaceAndComments();
                if (_isAtEnd()) break;

                SourceLocation loc = _currentLoc();
                if (_matchString("</style>")) {
                    tokens.emplace_back(TokenKind::TagStyleClose, "</style>", loc);
                    _mode = LexerMode::Root;
                    continue;
                }

                char c = _peek();
                if (c == '{') {
                    _advance();
                    tokens.emplace_back(TokenKind::LeftBrace, "{", loc);
                } else if (c == '}') {
                    _advance();
                    tokens.emplace_back(TokenKind::RightBrace, "}", loc);
                } else if (c == ':') {
                    _advance();
                    tokens.emplace_back(TokenKind::Colon, ":", loc);
                } else if (c == ';') {
                    _advance();
                    tokens.emplace_back(TokenKind::Semicolon, ";", loc);
                } else if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '#' || c == '-' || c == '_' || c == '*' || c == '@' || c == '%') {
                    size_t start = _cursor;
                    while (!_isAtEnd()) {
                        char sc = _peek();
                        if (std::isalnum(static_cast<unsigned char>(sc)) || sc == '.' || sc == '#' || sc == '-' || sc == '_' || sc == '*' || sc == '@' || sc == '%' || sc == '(' || sc == ')' || sc == ',') {
                            _advance();
                        } else {
                            break;
                        }
                    }
                    std::string text = _source.substr(start, _cursor - start);
                    tokens.emplace_back(TokenKind::Identifier, text, loc);
                } else if (c == '"' || c == '\'') {
                    tokens.push_back(_lexString(c));
                } else {
                    std::string sym(1, _advance());
                    tokens.emplace_back(TokenKind::Invalid, sym, loc);
                }
            }
        }

        tokens.emplace_back(TokenKind::Eof, "", _currentLoc());
        return tokens;
    }

} // namespace Keety
