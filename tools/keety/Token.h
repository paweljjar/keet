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
        TagServerOpen, // <server>
        TagServerClose, // </server>

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
        Question, // ?
        Arrow, // ->
        FatArrow, // =>
        Assign, // =
        PlusAssign, // +=
        MinusAssign, // -=
        Ampersand, // &
        Star, // *
        Lifetime, // 'a, 'static, etc.

        // Keywords
        KwImport, // import
        KwCpp, // cpp
        KwFrom, // from
        KwAs, // as
        KwState, // state
        KwLet, // let
        KwConst, // const
        KwDef, // def
        KwReturn, // return
        KwIf, // if
        KwElif, // elif
        KwElse, // else
        KwFor, // for
        KwIn, // in

        // Server block keywords
        KwDatabase, // database
        KwTable, // table
        KwPrimaryKey, // primary_key
        KwAutoIncrement, // auto_increment
        KwUnique, // unique
        KwNotNull, // not_null or required
        KwDefault, // default
        KwBackend, // backend
        KwFrontend, // frontend
        KwGet, // get
        KwPost, // post
        KwPut, // put
        KwDelete, // delete

        // Standalone KeetScript language keywords
        KwFn, // fn
        KwExport, // export
        KwClass, // class
        KwStruct, // struct
        KwPublic, // public
        KwPrivate, // private
        KwProtected, // protected
        KwModule, // module
        KwThis, // this
        KwPrint, // print
        KwErrtype, // errtype
        KwZonk, // zonk
        KwMove, // move
        KwMut, // mut

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
                   kind == TokenKind::KwIn || kind == TokenKind::KwDatabase ||
                   kind == TokenKind::KwTable || kind == TokenKind::KwPrimaryKey ||
                   kind == TokenKind::KwAutoIncrement || kind == TokenKind::KwUnique ||
                   kind == TokenKind::KwNotNull || kind == TokenKind::KwDefault ||
                   kind == TokenKind::KwBackend ||
                   kind == TokenKind::KwFrontend || kind == TokenKind::KwGet ||
                   kind == TokenKind::KwPost || kind == TokenKind::KwPut ||
                   kind == TokenKind::KwDelete || kind == TokenKind::KwFn ||
                   kind == TokenKind::KwExport || kind == TokenKind::KwClass ||
                   kind == TokenKind::KwStruct || kind == TokenKind::KwPublic ||
                   kind == TokenKind::KwPrivate || kind == TokenKind::KwProtected ||
                   kind == TokenKind::KwModule || kind == TokenKind::KwThis ||
                   kind == TokenKind::KwPrint || kind == TokenKind::KwErrtype ||
                   kind == TokenKind::KwZonk || kind == TokenKind::KwMove ||
                   kind == TokenKind::KwMut;
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
                case TokenKind::TagServerOpen: return "<server>";
                case TokenKind::TagServerClose: return "</server>";
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
                case TokenKind::Question: return "?";
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
                case TokenKind::KwConst: return "const";
                case TokenKind::KwDef: return "def";
                case TokenKind::KwReturn: return "return";
                case TokenKind::KwIf: return "if";
                case TokenKind::KwElif: return "elif";
                case TokenKind::KwElse: return "else";
                case TokenKind::KwFor: return "for";
                case TokenKind::KwIn: return "in";
                case TokenKind::KwDatabase: return "database";
                case TokenKind::KwTable: return "table";
                case TokenKind::KwPrimaryKey: return "primary_key";
                case TokenKind::KwAutoIncrement: return "auto_increment";
                case TokenKind::KwUnique: return "unique";
                case TokenKind::KwNotNull: return "not_null";
                case TokenKind::KwDefault: return "default";
                case TokenKind::KwBackend: return "backend";
                case TokenKind::KwFrontend: return "frontend";
                case TokenKind::KwGet: return "get";
                case TokenKind::KwPost: return "post";
                case TokenKind::KwPut: return "put";
                case TokenKind::KwDelete: return "delete";
                case TokenKind::KwFn: return "fn";
                case TokenKind::KwExport: return "export";
                case TokenKind::KwClass: return "class";
                case TokenKind::KwStruct: return "struct";
                case TokenKind::KwPublic: return "public";
                case TokenKind::KwPrivate: return "private";
                case TokenKind::KwProtected: return "protected";
                case TokenKind::KwModule: return "module";
                case TokenKind::KwThis: return "this";
                case TokenKind::KwPrint: return "print";
                case TokenKind::KwErrtype: return "errtype";
                case TokenKind::KwZonk: return "zonk";
                case TokenKind::KwMove: return "move";
                case TokenKind::KwMut: return "mut";
                case TokenKind::Ampersand: return "&";
                case TokenKind::Star: return "*";
                case TokenKind::Lifetime: return "lifetime";
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
