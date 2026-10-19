//
// Created by PAWEL on 19/10/2026.
//

#ifndef KEETY_CODEGEN_H
#define KEETY_CODEGEN_H

#include <memory>
#include <set>
#include <sstream>
#include <string>

#include "AST.h"

namespace Keety {

    struct GeneratedCode { // Holds generated C++20 header and source contents
        std::string headerCode; // Content of the .h header file
        std::string sourceCode; // Content of the .cpp implementation file
    };

    class Codegen { // Generates optimized C++20 code emitting UIEngine::Component subclasses
        public:
            explicit Codegen(const SourceFile& ast, std::string headerFileName = "")
                : _ast(ast), _headerFileName(std::move(headerFileName)) {}

            GeneratedCode generate(); // Produce complete C++20 header and source files

        private:
            std::string _generateHeader();
            std::string _generateSource();

            std::string _mapType(const std::string& typeName) const;
            void _collectInternedStrings(const TemplateNode& node);
            void _generateNodeEmission(std::ostream& os, const TemplateNode& node, const std::string& parentVecName, int& nodeCounter);
            void _generateElementEmission(std::ostream& os, const ElementNode& elem, const std::string& parentVecName, int& nodeCounter);

            const SourceFile& _ast;
            std::string _headerFileName;
            std::set<std::string> _internedTags;
            std::set<std::string> _internedProps;
    };

} // namespace Keety

#endif // KEETY_CODEGEN_H
