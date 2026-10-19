//
// Created by PAWEL on 19/10/2026.
//

#ifndef KEETY_TOKEN_H
#define KEETY_TOKEN_H

#include <string>
#include <string_view>

namespace Keety {

    struct SourceLocation { // Precise location within a source .keet file for error diagnostics
        std::string filename; // Path to the source file being processed
        size_t line = 1; // 1-based line number
        size_t column = 1; // 1-based column offset

        [[nodiscard]] std::string toString() const { // Format location into standard file:line:col format
            return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
        }
    };

    enum class TokenKind { // Comprehensive token enumeration for KeetScript SFC lexing
        // Special tokens
        Eof, // End of source input stream
        Invalid, // Unrecognized or malformed character sequence

        // Section delimiters
        TagScriptOpen, // <script>
        TagScriptClose, // </script>
        TagTemplateOpen, // <template>
        TagTemplateClose, // </template>
        TagStyleOpen, // <style>
        TagStyleClose, // </style>

        // Structural and XML syntax
        LessThan, // <
        GreaterThan, // >
        SlashGreaterThan, // />
        LessThanSlash, // </
        LeftBrace, // {
        RightBrace, // }
        LeftParen, // (
        RightParen, // )
        LeftBracket, // [
        RightBracket, // ]
        Colon, // :
        Semicolon, // ;
        Comma, // ,
        Dot, // .
        Arrow, // ->
        FatArrow, // =>
        Assign, // =
        PlusAssign, // +=
        MinusAssign, // -=

        // Keywords
        KwImport, // import
        KwCpp, // cpp
        KwFrom, // from
        KwAs, // as
        KwState, // state
        KwLet, // let
        KwDef, // def
        KwReturn, // return
        KwIf, // if
        KwElif, // elif
        KwElse, // else
        KwFor, // for
        KwIn, // in

        // Literals and identifiers
        Identifier, // Alphanumeric identifier (e.g., count, onClick, title)
        StringLiteral, // Quoted string literal ("..." or '...')
        NumberLiteral, // Integer or floating point number
        RawText, // Raw textual content between template tags

        // Template directives
        DirectiveBind, // bind: (prefix for two-way state binding)
        EventPrefix // on (prefix for event listener attributes)
    };

    struct Token { // Lexical unit emitted by the KeetScript lexer
        TokenKind kind = TokenKind::Invalid; // Discriminator identifying token variety
        std::string text; // Exact string slice from the source code
        SourceLocation loc; // File, line, and column coordinates where token begins

        Token() = default;
        Token(TokenKind k, std::string t, SourceLocation l)
            : kind(k), text(std::move(t)), loc(std::move(l)) {}

        [[nodiscard]] bool is(TokenKind k) const { return kind == k; } // Helper checking if token matches specific kind
        [[nodiscard]] bool isKeyword() const { // Determine whether token is any reserved language keyword
            return kind == TokenKind::KwImport || kind == TokenKind::KwCpp ||
                   kind == TokenKind::KwFrom || kind == TokenKind::KwAs ||
                   kind == TokenKind::KwState || kind == TokenKind::KwLet ||
                   kind == TokenKind::KwDef || kind == TokenKind::KwReturn ||
                   kind == TokenKind::KwIf || kind == TokenKind::KwElif ||
                   kind == TokenKind::KwElse || kind == TokenKind::KwFor ||
                   kind == TokenKind::KwIn;
        }

        [[nodiscard]] static std::string_view kindToString(TokenKind k) { // Human-readable description for debugging and parser error messages
            switch (k) {
                case TokenKind::Eof: return "EOF";
                case TokenKind::Invalid: return "invalid";
                case TokenKind::TagScriptOpen: return "<script>";
                case TokenKind::TagScriptClose: return "</script>";
                case TokenKind::TagTemplateOpen: return "<template>";
                case TokenKind::TagTemplateClose: return "</template>";
                case TokenKind::TagStyleOpen: return "<style>";
                case TokenKind::TagStyleClose: return "</style>";
                case TokenKind::LessThan: return "<";
                case TokenKind::GreaterThan: return ">";
                case TokenKind::SlashGreaterThan: return "/>";
                case TokenKind::LessThanSlash: return "</";
                case TokenKind::LeftBrace: return "{";
                case TokenKind::RightBrace: return "}";
                case TokenKind::LeftParen: return "(";
                case TokenKind::RightParen: return ")";
                case TokenKind::LeftBracket: return "[";
                case TokenKind::RightBracket: return "]";
                case TokenKind::Colon: return ":";
                case TokenKind::Semicolon: return ";";
                case TokenKind::Comma: return ",";
                case TokenKind::Dot: return ".";
                case TokenKind::Arrow: return "->";
                case TokenKind::FatArrow: return "=>";
                case TokenKind::Assign: return "=";
                case TokenKind::PlusAssign: return "+=";
                case TokenKind::MinusAssign: return "-=";
                case TokenKind::KwImport: return "import";
                case TokenKind::KwCpp: return "cpp";
                case TokenKind::KwFrom: return "from";
                case TokenKind::KwAs: return "as";
                case TokenKind::KwState: return "state";
                case TokenKind::KwLet: return "let";
                case TokenKind::KwDef: return "def";
                case TokenKind::KwReturn: return "return";
                case TokenKind::KwIf: return "if";
                case TokenKind::KwElif: return "elif";
                case TokenKind::KwElse: return "else";
                case TokenKind::KwFor: return "for";
                case TokenKind::KwIn: return "in";
                case TokenKind::Identifier: return "identifier";
                case TokenKind::StringLiteral: return "string literal";
                case TokenKind::NumberLiteral: return "number literal";
                case TokenKind::RawText: return "raw text";
                case TokenKind::DirectiveBind: return "bind directive";
                case TokenKind::EventPrefix: return "event prefix";
            }
            return "unknown";
        }
    };

} // namespace Keety

#endif // KEETY_TOKEN_H
