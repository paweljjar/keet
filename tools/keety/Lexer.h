//
// Created by PAWEL on 19/10/2026.
//

#ifndef KEETY_LEXER_H
#define KEETY_LEXER_H

#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "Token.h"

namespace Keety {

    class LexerException : public std::runtime_error { // Specialized error indicating a syntax or lexical scanning fault
        public:
            LexerException(std::string message, SourceLocation loc)
                : std::runtime_error(loc.toString() + ": error: " + message), _loc(std::move(loc)) {}

            [[nodiscard]] const SourceLocation& location() const { return _loc; }

        private:
            SourceLocation _loc;
    };

    class Lexer { // Lexical scanner converting KeetScript (.keet) SFC source into a stream of tokens
        public:
            explicit Lexer(std::string source, std::string filename = "Component.keet")
                : _source(std::move(source)), _filename(std::move(filename)) {}

            std::vector<Token> tokenize(); // Process entire input stream and return token vector ending in Eof

        private:
            // Mode flags for context-sensitive SFC lexing
            enum class LexerMode {
                Root, // Expecting <script>, <template>, or <style> tags
                Script, // Inside <script> ... </script>
                Template, // Inside <template> ... </template>
                TemplateTag, // Inside an XML/HTML tag header: <div ... >
                Style // Inside <style> ... </style>
            };

            [[nodiscard]] bool _isAtEnd() const { return _cursor >= _source.size(); }
            [[nodiscard]] char _peek(size_t offset = 0) const {
                if (_cursor + offset >= _source.size()) return '\0';
                return _source[_cursor + offset];
            }
            char _advance() {
                if (_isAtEnd()) return '\0';
                char c = _source[_cursor++];
                if (c == '\n') {
                    _line++;
                    _column = 1;
                } else {
                    _column++;
                }
                return c;
            }
            bool _match(char expected) {
                if (_isAtEnd() || _source[_cursor] != expected) return false;
                _advance();
                return true;
            }
            bool _matchString(std::string_view expected) {
                if (_cursor + expected.size() > _source.size()) return false;
                if (_source.compare(_cursor, expected.size(), expected) == 0) {
                    for (size_t i = 0; i < expected.size(); ++i) {
                        _advance();
                    }
                    return true;
                }
                return false;
            }

            void _skipWhitespaceAndComments();
            Token _lexIdentifierOrKeyword();
            Token _lexNumber();
            Token _lexString(char quote);
            Token _lexRawTextUntil(std::string_view stopTag);

            SourceLocation _currentLoc() const {
                return SourceLocation{_filename, _line, _column};
            }

            std::string _source;
            std::string _filename;
            size_t _cursor = 0;
            size_t _line = 1;
            size_t _column = 1;
            LexerMode _mode = LexerMode::Root;
    };

} // namespace Keety

#endif // KEETY_LEXER_H
