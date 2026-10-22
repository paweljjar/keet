//
// Created by PAWEL on 22/10/2026.
// Diagnostic.h - Structured Compiler Diagnostics for KeetScript & keety 🐱
//

#ifndef KEETY_DIAGNOSTIC_H
#define KEETY_DIAGNOSTIC_H

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>

#include "Token.h"

namespace Keety {

    enum class DiagnosticSeverity {
        Error,
        Warning,
        Note,
        Help
    };

    enum class ErrorCode {
        E0501_UseOfMovedValue,
        E0502_BorrowConflict,
        E0503_MultipleMutableBorrows,
        E0504_MutationDuringBorrow,
        E0505_DanglingReference,
        E0506_CannotMoveBorrowed,
        E0500_GeneralSafetyError
    };

    inline std::string errorCodeToString(ErrorCode code) {
        switch (code) {
            case ErrorCode::E0501_UseOfMovedValue: return "E0501";
            case ErrorCode::E0502_BorrowConflict: return "E0502";
            case ErrorCode::E0503_MultipleMutableBorrows: return "E0503";
            case ErrorCode::E0504_MutationDuringBorrow: return "E0504";
            case ErrorCode::E0505_DanglingReference: return "E0505";
            case ErrorCode::E0506_CannotMoveBorrowed: return "E0506";
            case ErrorCode::E0500_GeneralSafetyError: return "E0500";
        }
        return "E0500";
    }

    inline std::ostream& operator<<(std::ostream& os, ErrorCode code) {
        return os << errorCodeToString(code);
    }

    struct DiagnosticSpan {
        SourceLocation loc;
        size_t length = 1;
        std::string label;
    };

    class Diagnostic {
        public:
            ErrorCode code = ErrorCode::E0500_GeneralSafetyError;
            DiagnosticSeverity severity = DiagnosticSeverity::Error;
            std::string message;
            SourceLocation primaryLoc;
            size_t primaryLength = 1;
            std::string primaryLabel;
            std::vector<DiagnosticSpan> secondarySpans;
            std::string help;

            Diagnostic() = default;

            Diagnostic(ErrorCode c, std::string msg, SourceLocation loc, size_t len = 1, std::string label = "")
                : code(c), severity(DiagnosticSeverity::Error), message(std::move(msg)),
                  primaryLoc(std::move(loc)), primaryLength(len), primaryLabel(std::move(label)) {}

            Diagnostic& withSecondary(SourceLocation loc, size_t len, std::string label) {
                secondarySpans.push_back({std::move(loc), len, std::move(label)});
                return *this;
            }

            Diagnostic& withHelp(std::string helpMsg) {
                help = std::move(helpMsg);
                return *this;
            }

            [[nodiscard]] std::string format(std::string_view sourceContent = "") const {
                std::ostringstream os;
                std::string codeStr = errorCodeToString(code);

                // Header line: error[E0501]: use of moved value
                os << "error[" << codeStr << "]: " << message << "\n";
                os << "  --> " << primaryLoc.filename << ":" << primaryLoc.line << ":" << primaryLoc.column << "\n";

                if (!sourceContent.empty() && primaryLoc.line > 0) {
                    // Extract the corresponding line from sourceContent
                    std::string lineContent = _extractLine(sourceContent, primaryLoc.line);
                    std::string lineNumStr = std::to_string(primaryLoc.line);
                    size_t gutterWidth = std::max<size_t>(lineNumStr.size(), 2);

                    os << std::string(gutterWidth + 1, ' ') << "|\n";
                    os << lineNumStr << std::string(gutterWidth - lineNumStr.size() + 1, ' ') << "| " << lineContent << "\n";

                    // Caret pointer line: |   ^^^^ label
                    os << std::string(gutterWidth + 1, ' ') << "| ";
                    size_t col = primaryLoc.column > 0 ? primaryLoc.column - 1 : 0;
                    if (col < lineContent.size()) {
                        os << std::string(col, ' ');
                    }
                    size_t carets = std::max<size_t>(primaryLength, 1);
                    os << std::string(carets, '^');
                    if (!primaryLabel.empty()) {
                        os << " " << primaryLabel;
                    }
                    os << "\n";
                    os << std::string(gutterWidth + 1, ' ') << "|\n";
                }

                if (!help.empty()) {
                    os << "   = help: " << help << "\n";
                }

                return os.str();
            }

        private:
            static std::string _extractLine(std::string_view source, size_t targetLine) {
                size_t currentLine = 1;
                size_t lineStart = 0;
                for (size_t i = 0; i < source.size(); ++i) {
                    if (source[i] == '\n') {
                        if (currentLine == targetLine) {
                            return std::string(source.substr(lineStart, i - lineStart));
                        }
                        currentLine++;
                        lineStart = i + 1;
                    }
                }
                if (currentLine == targetLine && lineStart < source.size()) {
                    return std::string(source.substr(lineStart));
                }
                return "";
            }
    };

    class DiagnosticException : public std::runtime_error {
        public:
            Diagnostic diag;

            explicit DiagnosticException(Diagnostic d, std::string_view sourceContent = "")
                : std::runtime_error(d.format(sourceContent)), diag(std::move(d)) {}

            [[nodiscard]] const Diagnostic& getDiagnostic() const { return diag; }
    };

} // namespace Keety

#endif // KEETY_DIAGNOSTIC_H
